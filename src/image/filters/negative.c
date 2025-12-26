#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "filters.h"


/* ---1. Threads function --- */
void *negative_thread(void *arg) {
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

            for (size_t c = 0; c < img->channels; c++) {
                row_ptr[pixel_index + c] = 255 - row_ptr[pixel_index + c];
            }
        }
    }
    pthread_exit(NULL);
}


/* ---2. Negative Filter with Multithreading --- */
void apply_negative(image_t *img) {
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
        if(pthread_create(&threads[i], NULL, negative_thread, (void *)&thread_args[i]) == -1) {
            perror("[Negative] Failed to create thread");
            exit(EXIT_FAILURE);
        }


    }    // 3. Wait for all threads to finish
    for(int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL) ;
    }
}   
