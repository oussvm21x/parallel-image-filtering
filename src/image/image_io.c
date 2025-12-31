#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image_api.h"

/* --- 1. Helper Functions --- */

// Function to get file extension from filepath
static const char* get_file_extension(const char* filepath) {
    const char* dot = strrchr(filepath, '.');
    if (!dot || dot == filepath) return "";
    return dot + 1;
}   

/* --- 2. Generic Image Loading and Saving ---*/
image_t* load_image(const char* filepath) {
    const char* ext = get_file_extension(filepath);
    if (strcmp(ext, "bmp") == 0) {
        return load_bmp(filepath);
    } else if (strcmp(ext, "png") == 0) {
        return load_png(filepath);
    } else {
        fprintf(stderr, "[Image] Unsupported file format: %s\n", ext);
        return NULL;
    }
}

int save_image(const char* filepath, const image_t* img) {
    const char* ext = get_file_extension(filepath);
    if (strcmp(ext, "bmp") == 0) {
        return save_bmp(filepath, img);
    } else if (strcmp(ext, "png") == 0) {
        return save_png(filepath, img);
    } else {
        fprintf(stderr, "[Image] Unsupported file format: %s\n", ext);
        return -1;
    }
}