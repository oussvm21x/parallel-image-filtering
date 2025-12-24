#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image_api.h"



/* --- 1. Image Memory Management ---*/

image_t* create_image(size_t width , size_t height , size_t channels) {

    // 1. Allocate memory 
    image_t* img = (image_t*)malloc(sizeof(image_t));
    if (img == NULL) {
        perror("[Image] Failed to allocate memory for image structure");
        exit(EXIT_FAILURE);
    }

    // 2. Initialize image properties 
    // This is general for any image type
    img->width = width;
    img->height = height;
    img->channels = channels;

    // 3. Calculate row stride
    // This is for bmp format 
    // In BMP, rows must be a multiple of 4 bytes.
    // Formula: (width * channels + 3) & ~3
    // This ensures we can save directly to BMP without re-packing later.
    // For other formats (e.g., PNG, JPEG), row_stride they do not require this padding.
    // However , they don't mind either, so we use the same approach for all formats for simplicity.

    size_t raw_row_size = width * channels;
    img->row_stride = (raw_row_size + 3) & ~3; 

    // 4. Allocate pixel data with row stride consideration
    img->data = (uint8_t*)malloc(img->row_stride * height);
    if (img->data == NULL) {
        perror("[Image] Failed to allocate memory for pixel data");
        free(img);
        exit(EXIT_FAILURE);
    }
    // 5. Initialize pixel data to zero (black/transparent)
    memset(img->data, 0, img->row_stride * height);
    return img;
    
}

void free_image(image_t* img) {
    if (img) {
        if (img->data) {
            free(img->data);
            img->data = NULL; 
        }
        free(img);
    }
}   

