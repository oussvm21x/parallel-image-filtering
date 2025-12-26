#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "filters.h"

/* ---1. Threads function --- */
void *seppia_thread(void *arg) {
    thread_data_t *data = (thread_data_t *) arg ;
    image_t *img = data->img ;
    printf("[Thread %d] Processing rows %zu to %zu\n", data->thread_id, data->start_row, data->end_row);
    for (size_t y = data->start_row; y < data->end_row; y++) {
        // Calculate the pointer to the start of the row
        // we use row_stride to account for padding
        uint8_t *row_ptr = img->data + y * img->row_stride;
        // Process each pixel in the row
        // we use width as a limit, ignoring padding bytes
        for (size_t x = 0; x < img->width; x++) {
            size_t pixel_index = x * img->channels;
            uint8_t *b = &row_ptr[pixel_index + 0];
            uint8_t *g = &row_ptr[pixel_index + 1];
            uint8_t *r = &row_ptr[pixel_index + 2];

            float raw_r = *r ;
            float raw_g = *g ;
            float raw_b = *b ;


            float tr = (0.393 * raw_r + 0.769 * raw_g + 0.189 * raw_b);
            float tg = (0.349 * raw_r + 0.686 * raw_g + 0.168 * raw_b);
            float tb = (0.272 * raw_r + 0.534 * raw_g + 0.131 * raw_b);
            if(tr > 255) tr = 255;
            if(tg > 255) tg = 255;
            if(tb > 255) tb = 255;
            *r = (uint8_t)tr;
            *g = (uint8_t)tg;
            *b = (uint8_t)tb;
        }
    }
    pthread_exit(NULL);
}   


/* ---2. Sepia Filter with Multithreading --- */
void apply_seppia(image_t *img) {
    pthread_t threads[NUM_THREADS] ;
    thread_data_t thread_args[NUM_THREADS] ;
    // 1. Calculate rows per thread
    size_t rows_per_thread = img->height / NUM_THREADS ;
    size_t remaining_rows = img->height % NUM_THREADS ;
    // 2. Create threads
    for(int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].img = img ;
        thread_args[i].start_row = i * rows_per_thread ;
        thread_args[i].end_row = (i + 1) * rows_per_thread ;
        thread_args[i].thread_id = i ;
        // Last thread takes the remaining rows
        if (i == NUM_THREADS - 1) {
            thread_args[i].end_row += remaining_rows ;
        }
        if(pthread_create(&threads[i], NULL, seppia_thread, (void *)&thread_args[i]) == -1) {
            perror("[Sepia] Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }    // 3. Wait for all threads to finish
    for(int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL) ;
    }
}