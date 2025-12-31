#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image_api.h"
#include <arpa/inet.h> // Required for htonl/ntohl (Converts numbers between Host & Network Byte Order)
#include <zlib.h>      // Standard library for the DEFLATE compression algorithm


/* ---1. Data Structures --- */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t bit_depth;
    uint8_t color_type;
    uint8_t compression_method;
    uint8_t filter_method;
    uint8_t interlace_method;
} IHDR  ; 

/* --- 1. PNG Signature  --- */
static const uint8_t PNG_SIG[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

/* --- 2. Helpers ---*/

/* --- 2.1 write chunk */
// Function to write PNG chunks
// A chunk is : length (4 bytes) + type (4 bytes) + data (length bytes) + CRC (4 bytes)

void write_chunk(FILE *f , const char *type , uint8_t *data , uint32_t len) {

    // PNG uses Big Endian format for multi-byte integers
    // Convert length to big-endian format
    uint32_t len_be = htonl(len);

    /* ---1. Write length --- */
    fwrite(&len_be, sizeof(len_be), 1, f); 
    /* ---2. Write type --- */
    fwrite(type, 1, 4, f);
    /* ---3. Write data --- */
    if (len > 0 && data != NULL) {
        fwrite(data, 1, len, f);        
    }

    /* ---4. Write CRC --- */
    // CRC ensure the integrity of the chunk type and data 
    // It means it ensure that the chunk wasn't corrupted 
    // Calculate CRC over type and data

    // 4.1 Initialize CRC value to zero 
    uint32_t crc = crc32(0L, Z_NULL, 0); 

    // 4.2 Update CRC with chunk type
    crc = crc32(crc, (const Bytef *)type, 4);

    // 4.3 Update CRC with chunk data
    if (len > 0 && data != NULL) {
        crc = crc32(crc, data, len);
    }
    // 4.4 Convert CRC to big-endian format
    uint32_t crc_be = htonl(crc);

    // 4.5 Write CRC to file
    fwrite(&crc_be, sizeof(crc_be), 1, f);

}

/* --- 2.2 Read u32 --- */
// Function to read a 4-byte unsigned integer from file in big-endian format
// and convert it to host byte order (little-endian)
uint32_t read_u32(FILE *f) {
    uint32_t be_value;
    fread(&be_value, sizeof(be_value), 1, f);
    return ntohl(be_value); // Convert from big-endian to host byte order
}

/* --- 3. PNG Loading and Saving Functions --- */

/* --- 3.1 Save Image --- */
int save_png(const char *filepath , const image_t *img) {
    // 1. Open file for writing in binary mode
    FILE *f = fopen(filepath , "wb");
    if (!f) {
        perror("[PNG] Error opening file for writing");
        exit(EXIT_FAILURE);
    }

    // 2. Write PNG signature
    fwrite(PNG_SIG, 1, sizeof(PNG_SIG), f);

    // 3. Write IHDR chunk

    // 3.1 Prepare IHDR data
    IHDR ihdr;
    ihdr.width = htonl(img->width);
    ihdr.height = htonl(img->height);
    ihdr.bit_depth = 8; // Standard value , 8 bits per channel
    ihdr.color_type = (img->channels == 3) ? 2 : 0; // 2 = Truecolor (RGB), 0 = Grayscale
    ihdr.compression_method = 0; // Standard DEFLATE compression
    ihdr.filter_method = 0;      // Standard adaptive filtering
    ihdr.interlace_method = 0; // No interlacing

    // 3.2 Write IHDR chunk to file
    write_chunk(f, "IHDR", (uint8_t *)&ihdr, sizeof(IHDR));

    // 4. Write IDAT chunk
    // 4.1 Prepare raw image data with filter bytes
    // PNG requires a filter byte at the start of each row
    // The byte tells the decoder how to predict pixel values
    // Here we use filter type 0 (None) for simplicity
    // So we insert 0x00 at the start of each row
    size_t row_size = img->channels * img->width;
    size_t raw_len = (row_size + 1) * img->height; // +1 for filter byte per row
    uint8_t *raw_data = (uint8_t *)malloc(raw_len);
    if (!raw_data) {
        perror("[PNG] Error allocating memory for raw image data");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    for (size_t y=0 ; y < img->height ; y++) {
        // pointer to the start of the row in raw_data
        uint8_t *dest_row = raw_data + y * (row_size + 1); 

        // pointer to the actual pixel data in img->data
        uint8_t *src_row = img->data + y * img->row_stride;
        dest_row[0] = 0x00; // Filter type 0 (None)
        
        // Copy pixel data
        memcpy(dest_row + 1, src_row, row_size); 
    }

    // 4.2 Compress raw image data using zlib 
    // Allocate buffer for compressed data
    // It should be slightly larger than raw data to handle zlib overhead
    unsigned long compressed_len = raw_len + 1024 ; 
    uint8_t *compressed_data = (uint8_t *)malloc(compressed_len);
    if (!compressed_data) {
        perror("[PNG] Error allocating memory for compressed data");
        free(raw_data);
        fclose(f);
        exit(EXIT_FAILURE);
    }

    // Compress using zlib's compress function
    if (compress(compressed_data, &compressed_len, raw_data, raw_len) != Z_OK) {
        perror("[PNG] Error compressing image data");
        free(raw_data);
        free(compressed_data);
        fclose(f);
        exit(EXIT_FAILURE);
    }

    // free raw image data as it's no longer needed
    free(raw_data);

    // 4.3 Write IDAT chunk to file
    write_chunk(f, "IDAT", compressed_data, compressed_len);
    // free compressed data as it's no longer needed
    free(compressed_data);


    // 5. Write IEND chunk to signify end of PNG file
    write_chunk(f, "IEND", NULL, 0);

    // 6. Close file
    fclose(f);
    printf("[PNG] Image saved successfully: %s\n", filepath);
    return 0;
}
    

/* --- 3.2 Load Image --- */
image_t* load_png(const char *filepath) {

    // 1. Open file for reading in binary mode
    FILE *f = fopen(filepath , "rb");
    if (!f) {
        perror("[PNG] Error opening file");
        exit(EXIT_FAILURE);
    }

    // 2. Read and validate PNG signature
    uint8_t signature[8];
    fread(signature, 1, sizeof(signature), f);
    if (memcmp(signature, PNG_SIG, sizeof(PNG_SIG)) != 0) {
        perror("[PNG] Invalid PNG signature");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    // 3. State variables for parsing 
    uint8_t *compressed_data = NULL;
    size_t compressed_size = 0;
    u_int32_t width = 0, height = 0;
    uint8_t color_type = 0; // 2 = RGB , 3 : Indexed , 0 : Grayscale

    // Palette storage : max 256 colors , each color is 3 bytes (RGB)
    // used only if color_type == 3
    uint8_t palette[256][3];
    memset(palette, 0, sizeof(palette));

    // 4. Chunk parsing loop
    // Read chunks until IEND is encountered
    while(!feof(f)){
        // 4.1 Read chunk length
        uint32_t len = read_u32(f);
        // 4.2 Read chunk type
        char type[5] = {0};
        if(fread(type, 1, 4, f) != 4) {
            perror("[PNG] Error reading chunk type");
            break; // EOF or read error
        }
        // 4.3 Process chunk based on type
        // IHDR chunk
        if (strcmp(type, "IHDR") == 0) {
            IHDR ihdr;
            fread(&ihdr, 1, sizeof(IHDR), f);
            width = ntohl(ihdr.width);
            height = ntohl(ihdr.height);
            color_type = ihdr.color_type;
            


    }

}
}