#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>      
#include <sys/stat.h>
#include <string.h>

#include "protocol.h"
#include "image_api.h"
#include "filters.h"    


void worker_core(filter_request_t req) {
    printf("[Worker %d] Processing request for PID %d on file %s with filter %d\n",
           getpid(), req.pid, req.chemin, req.filtre);

    // 1. Load the image 
    image_t *img = load_image(req.chemin);
    if (img == NULL) {
        perror("[Worker] Failed to load image");
        exit(EXIT_FAILURE);
    }

    // 2.Apply filters (Multitheading)
    printf("[Worker %d] Applying filter %d\n", getpid(), req.filtre);
    switch (req.filtre) {
        case FILTER_GRAYSCALE:
            apply_grayscale(img);
            break;
    
        case FILTER_NEGATIVE : 
            apply_negative(img) ;
            break;
        case FILTER_SEPPIA :
            apply_seppia(img) ;
            break ;
    }

    // 3. Open response FIFO (Pipe) 
    char fifo_name[256];
    snprintf(fifo_name, sizeof(fifo_name), FIFO_RESPONSE_TEMPLATE, req.pid);
    int fd = open(fifo_name, O_WRONLY);
    if (fd == -1) {
        perror("[Worker] Failed to open response FIFO");
        free_image(img);
        exit(EXIT_FAILURE);
    }

    // 4. Send Header (width, height, channels)
    write(fd, &img->width, sizeof(size_t));
    write(fd, &img->height, sizeof(size_t));
    write(fd, &img->channels, sizeof(size_t));  

    // 5. Send Data ( Including Padding )
    size_t data_size = img->row_stride * img->height;
    ssize_t sent_bytes = write(fd, img->data, data_size);
    if (sent_bytes != (ssize_t)data_size) {
        perror("[Worker] Failed to send complete image data");
        exit(EXIT_FAILURE);
    }
    printf("[Worker %d] Sent %zd bytes of image data\n", getpid(), sent_bytes);
    // 6. Cleanup
    close(fd);
    free_image(img);

}