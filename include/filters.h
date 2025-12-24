#ifndef FILTERS_H
#define FILTERS_H

#include "image_api.h"

// Filter ID Constants (Must match protocol.h logic if defined there, 
// or simply use these ID definitions)
#define FILTER_GRAYSCALE 1
#define FILTER_NEGATIVE  2

// Filter Function Prototypes
void apply_grayscale(image_t *img);
void apply_negative(image_t *img); // Future use

#endif