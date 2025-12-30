#ifndef IMAGE_API_H
#define IMAGE_API_H

#include <unistd.h>    // for pid_t
#include <sys/types.h> // for pid_t on some platforms
#include <stdint.h>    // fixed-width integer types
#include <stddef.h>    // size_t, NULL


/* --- 1. The Generic Image  Structure */

// This structure hides the details of the image header 
// The filter functions will use only this structure to manipulate images
typedef struct {
    uint8_t* data;      // Pointer to the pixel data
    size_t width;       // Image width in pixels
    size_t height;      // Image height in pixels
    size_t channels;    // Number of color channels (e.g., 3 for RGB, 1 for Grayscale)
    size_t row_stride;  // Number of bytes in a row (including padding)
} image_t;


/* --- 2. Image I/O Functions ---*/

/* --- 2.1 Loading and Saving Generic Images --- */

// Load an image from a file on the disk 
image_t* load_image(const char* filepath);

// Save an image to a file on the disk
int save_image(const char* filepath, const image_t* img);



/* --- 2.2 Loading and Saving BMP Images --- */

// Load a BMP image from a file on the disk
image_t* load_bmp(const char* filepath);

// Save a BMP image to a file on the disk
int save_bmp(const char* filepath, const image_t* img);



/* --- 2.3 Loading and Saving JPEG Images --- */

// Load a JPEG image from a file on the disk
image_t* load_jpeg(const char* filepath);

// Save a JPEG image to a file on the disk
int save_jpeg(const char* filepath, const image_t* img);



/* --- 2.4 Loading and Saving PNG Images --- */

// Load a PNG image from a file on the disk
image_t* load_png(const char* filepath);

// Save a PNG image to a file on the disk
int save_png(const char* filepath, const image_t* img);



/* --- 3. Image Memory Management ---*/

// Free the memory allocated for an image
void free_image(image_t* img);

// Create an empty image with specified dimensions and channels
image_t* create_image(size_t width, size_t height, size_t channels);




#endif // IMAGE_API_H