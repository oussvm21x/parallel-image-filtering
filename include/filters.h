#ifndef FILTERS_H
#define FILTERS_H

#include "image_api.h"
#include <pthread.h>

/* --- 1. Filter Types ---*/
#define FILTER_GRAYSCALE 1
#define FILTER_NEGATIVE 2



// Concurrency Configuration
// We use 4 threads
#define NUM_THREADS 4

// Thread Arguments Structure
// Each thread needs to know: "Which image?" and "Which rows do I process?"
typedef struct {
    image_t *img;       // Shared pointer to the image
    size_t start_row;   // Starting Y coordinate
    size_t end_row;     // Ending Y coordinate (exclusive)
    int thread_id;      // For debugging logging
} thread_data_t;

// Filter Prototypes
void apply_grayscale(image_t *img);

#endif