#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image_api.h"

/* ---1. BMP Header Structure --- */

// We use #pragma pack(1) to force the compiler to store these
// exactly as they appear in memory, with no gaps/padding.

 #pragma pack(push , 1)

 typedef struct {
    uint16_t signature ;    // (  2 bytes ) 'BM'
    uint32_t fileSize ;     // (  4 bytes ) File size in bytes
    uint32_t reserved ;     // (  4 bytes ) Unused = 0
    uint32_t dataOffset ;   // (  4 bytes ) Offset from beginning of file to pixel data
 } BMPHeader ;              // ( 14 bytes)  

 typedef struct {
    uint32_t size ;             // ( 4 bytes ) Header size = 40 
    uint32_t width ;            // ( 4 bytes ) Image width in pixels
    uint32_t height ;           // ( 4 bytes ) Image height in pixels
    uint16_t plannes ;          // ( 2 bytes ) Number of color planes = 1
    uint16_t bitsPerPixel ;     // ( 2 bytes ) Number of bits per pixel 
    uint32_t compression ;      // ( 4 bytes ) Compression type (0 = none)
    uint32_t imageSize ;        // ( 4 bytes ) Compressed size ( 0 if uncompressed )
    uint32_t xPixelsPerM ;      // ( 4 bytes ) Horizontal resolution
    uint32_t yPixelsPerM ;      // ( 4 bytes ) Vertical resolution
    uint32_t colorsUsed ;       // ( 4 bytes ) Number of colors 
    uint32_t importantColors ;  // ( 4 bytes ) Important colors

} BMPInfoHeader ;


/* ---2. Helper functions --- */
void write_bmp_header ( FILE *f , int w , int h , int stride ) {

    BMPHeader header = {0} ;
    BMPInfoHeader infoHeader = {0} ;

    int dataSize = stride * h ;
    int totalSize = sizeof(BMPHeader) + sizeof(BMPInfoHeader) + dataSize ;

    // 1. Fill BMPHeader
    // The rest of the fields are already zero-initialized
    header.signature = 0x4D42 ; // 'BM'
    header.fileSize = totalSize ;
    header.dataOffset = sizeof(BMPHeader) + sizeof(BMPInfoHeader) ;

    // 2. Fill BMPInfoHeader
    // The rest of the fields are already zero-initialized
    infoHeader.size = sizeof(BMPInfoHeader) ;
    infoHeader.width = w ;
    infoHeader.height = h ;
    infoHeader.plannes = 1 ;
    infoHeader.bitsPerPixel = 24 ; // 24 bits = 3 bytes (RGB)
    infoHeader.compression = 0 ; // No compression
    infoHeader.imageSize = dataSize ; // Raw data size
    infoHeader.xPixelsPerM = 2835 ; // 72 DPI
    infoHeader.yPixelsPerM = 2835 ; // 72 DPI
    infoHeader.colorsUsed = 0 ; // All colors
    infoHeader.importantColors = 0 ; // All colors are important

    // 3. Write headers to file
    fwrite ( &header , sizeof(BMPHeader) , 1 , f ) ;
    fwrite ( &infoHeader , sizeof(BMPInfoHeader) , 1 , f ) ;   

}

/* --- 3. Image API --- */

/* --- 3.1 Load image --- */
image_t* load_image(const char* filepath) {
    FILE* f = fopen(filepath, "rb");
    if (!f) {
        perror("[BMP] Error opening file");
        exit(EXIT_FAILURE);
    }

    BMPHeader header;
    BMPInfoHeader infoHeader;

    // 1. Read BMP Header
    if (fread(&header, sizeof(BMPHeader), 1, f) != 1 || 
    fread(&infoHeader, sizeof(BMPInfoHeader), 1, f) != 1) {
        perror("[BMP] Error reading BMP header");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    // 2. Validate BMP format
    if (header.signature != 0x4D42) {
        perror("[BMP] Invalid BMP signature");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    if (infoHeader.bitsPerPixel != 24) {
        perror("[BMP] Unsupported bits per pixel");
        fclose(f);
        exit(EXIT_FAILURE);
    }
    
    // 3. Allocate image
    size_t width = infoHeader.width;
    size_t height = infoHeader.height;
    size_t channels = 3; // RGB

    image_t* img = create_image(width, height, channels);
    if (!img) {
        perror("[BMP] Error allocating image");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    // 4. Read pixel data
    // Move file pointer to pixel data
    fseek(f, header.dataOffset, SEEK_SET);

    // Read pixel data
    fread(img->data, 1, img->row_stride * height, f);

    fclose(f);
    return img;
}

/* --- 3.2 Save image --- */
int save_image(const char* filepath, const image_t* img) {
    FILE* f = fopen(filepath , "wb");
    if (!f) {
        perror("[BMP] Error opening file for writing");
        exit(EXIT_FAILURE);
    }

    // 1. Write BMP header
    write_bmp_header(f, img->width, img->height, img->row_stride);
    // 2. Write pixel data
    fwrite(img->data, 1, img->row_stride * img->height, f);
    fclose(f);
    return 0; 
}