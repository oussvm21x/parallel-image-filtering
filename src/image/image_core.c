#include <stdio.h>
#include <stdlib.h>
#include "image_api.h"

/* --- TEMPORARY STUBS TO ALLOW COMPILATION --- */

image_t* create_image(size_t w, size_t h, size_t channels) {
    // Return a dummy pointer just so the client doesn't crash immediately
    image_t *img = malloc(sizeof(image_t));
    img->width = w;
    img->height = h;
    img->channels = channels;
    img->row_stride = w * channels; // Simple packing assumption
    img->data = malloc(w * h * channels);
    return img;
}

void free_image(image_t *img) {
    if (img) {
        if (img->data) free(img->data);
        free(img);
    }
}

int save_image(const char *path, const image_t *img) {
    printf("[STUB] Would save image %zux%zu to '%s' now.\n", img->width, img->height, path);
    return 0; // Success
}

image_t* load_image(const char *path) {
    printf("[STUB] Would load image from '%s' now.\n", path);
    return NULL;
}