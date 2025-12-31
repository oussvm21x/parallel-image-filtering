#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image_api.h"
#include <arpa/inet.h> // Required for htonl/ntohl (Converts numbers between Host & Network Byte Order)
#include <zlib.h>      // Standard library for the DEFLATE compression algorithm

#pragma pack(1)

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
        return -1;
    }

    // 2. Write PNG signature
    fwrite(PNG_SIG, 1, sizeof(PNG_SIG), f);

    // 3. Write IHDR chunk

    // 3.1 Prepare IHDR data
    IHDR ihdr;
    ihdr.width = htonl(img->width);
    ihdr.height = htonl(img->height);
    ihdr.bit_depth = 8; // Standard value , 8 bits per channel
    if (img->channels == 3) {
        ihdr.color_type = 2; // RGB
    } else if (img->channels == 4) {
        ihdr.color_type = 6; // RGBA 
    } else {
        ihdr.color_type = 0; // Grayscale
    }
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
        return -1;
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
        return -1;
    }

    // Compress using zlib's compress function
    if (compress(compressed_data, &compressed_len, raw_data, raw_len) != Z_OK) {
        perror("[PNG] Error compressing image data");
        free(raw_data);
        free(compressed_data);
        fclose(f);
        return -1;
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
        return NULL;
    }

    // 2. Read and validate PNG signature
    uint8_t signature[8];
    fread(signature, 1, sizeof(signature), f);
    if (memcmp(signature, PNG_SIG, sizeof(PNG_SIG)) != 0) {
        perror("[PNG] Invalid PNG signature");
        fclose(f);
        return NULL;
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
            // Skip CRC
            fseek(f, 4, SEEK_CUR);
        }
        // PLTE chunk
        else if (strcmp(type, "PLTE") == 0) {
            int num_colors = len / 3;
            if(num_colors > 256) num_colors = 256;
            for (int i=0 ; i < num_colors ; i++) {
                fread(palette[i], 1, 3, f);
            }
            // Skip CRC
            fseek(f, 4, SEEK_CUR);
        }
        
        // IDAT chunk 
        else if (strcmp(type , "IDAT") == 0) {

            // Reallocate buffer to hold compressed data
            uint8_t *new_buffer = realloc(compressed_data, compressed_size + len);
            if (!new_buffer) {
                perror("[PNG] Error reallocating memory for compressed data");
                free(compressed_data);
                fclose(f);
                return NULL;
            }

            compressed_data = new_buffer;

            // Read chunk data into buffer
            if(fread(compressed_data + compressed_size, 1, len, f) != len) {
                perror("[PNG] Error reading IDAT chunk data");
                free(compressed_data);
                fclose(f);
                return NULL;
            }
            compressed_size += len;

            // Skip CRC
            fseek(f, 4, SEEK_CUR);
        }

        // IEND chunk
        else if (strcmp(type , "IEND") == 0) {
            // End of PNG file
            // Skip CRC
            fseek(f, 4, SEEK_CUR);
            break;
        }
        else {
            // Unknown chunk , skip it
            fseek(f, len + 4, SEEK_CUR); // Skip data + CRC
        }

    }
    fclose(f);

    // Validate that we have necessary data
    if (width == 0 || height == 0 || compressed_data == NULL) {
        perror("[PNG] Incomplete PNG data");
        free(compressed_data);
        return NULL;
    }

    // 5. Decompress image data using zlib
    // Determine channels based on color type
    int channels = 0;
    if (color_type == 2) {
        channels = 3; // RGB
    } else if (color_type == 3) {
        channels = 1; // Indexed color
    } 
    else if(color_type == 6) {
        channels = 4; // RGBA
    }
    else if (color_type == 0) {
        channels = 1; // Grayscale
    } else {
        perror("[PNG] Unsupported color type");
        free(compressed_data);
        return NULL;
    }

    // Calculate expected raw data size
    // Bytes per row = (channels * width) + 1 (filter byte)
    // Raw data size = ( bytes per row + 1 )* height
    size_t row_bytes= (channels * width) ;
    size_t raw_size = (row_bytes + 1) * height;
    uint8_t *raw_data = (uint8_t *)malloc(raw_size);
    if (!raw_data) {
        perror("[PNG] Error allocating memory for raw image data");
        free(compressed_data);
        return NULL;
    }

    // Decompress
    // res returns Z_OK on success
    int res = uncompress(raw_data, &raw_size, compressed_data, compressed_size);
    free(compressed_data);

    if (res != Z_OK) {
        perror("[PNG] Error decompressing image data");
        free(raw_data);
        return NULL;
    }

    // 6. Create image_t structure
    size_t out_channels = (color_type == 3) ? 3 : channels; // Expand indexed to RGB
    image_t *img = create_image(width, height, out_channels);
    if (!img) {
        perror("[PNG] Error creating image structure");
        free(raw_data);
        return NULL;
    }
    
    // 7. Reconstruction (Unfilter)
    // we need two buffers because unfiltring often relies on the row above 
    // to reconstruct the current row
    uint8_t *prev_row = (uint8_t *)calloc(row_bytes, 1); // Initialize to zero
    uint8_t *curr_row = (uint8_t *)malloc(row_bytes);
    if (!prev_row || !curr_row) {
        perror("[PNG] Error allocating memory for row buffers");
        free_image(img);
        free(prev_row);
        free(curr_row);
        free(raw_data);
        return NULL;
    }

    // 7.1 Process each row
    for (size_t y=0 ; y < height ; y++) {
        // 1. Locate the raw scanline
        // In the raw data, each scanline starts with a filter byte
        uint8_t *raw_scanline = raw_data + y * (row_bytes + 1);
        uint8_t filter_type = raw_scanline[0];
        uint8_t *data = raw_scanline + 1;

        // 2. itterate over each byte in the scanline
        for (size_t x=0 ; x < row_bytes ; x++) {
            // Identify neighboring bytes
            // Neighbors are bytes to the left and above the current byte
            uint8_t left = (x >= (size_t)channels) ? curr_row[x - channels] : 0;
            uint8_t above = prev_row[x];
            uint8_t upper_left = (x >= (size_t)channels) ? prev_row[x - channels] : 0;

            // The filtred value 
            uint8_t filtred = data[x];
            uint8_t recon = 0;

            // 3. Apply the inverse filter based on filter type
            switch (filter_type) {
                case 0: // None
                    // Data is raw , no filtering applied
                    recon = filtred;
                    break;
                case 1: // Sub
                    // Sub filter: recon = filtred + left
                    recon = filtred + left;
                    break;
                case 2: // Up
                    // Up filter: recon = filtred + above
                    recon = filtred + above;
                    break;
                case 3: // Average
                    // Average filter: recon = filtred + floor((left + above) / 2
                    recon = filtred + ((left + above) / 2);
                    break;
                case 4: // Paeth
                    // Paeth filter
                    // Predictive filter using a linear function of the three neighboring bytes
                    // recon = filtred + PaethPredictor(left, above, upper_left)
                    {
                        int p = left + above - upper_left;
                        int pa = abs(p - left);
                        int pb = abs(p - above);
                        int pc = abs(p - upper_left);
                        uint8_t paeth;
                        if (pa <= pb && pa <= pc) {
                            paeth = left;
                        } else if (pb <= pc) {
                            paeth = above;
                        } else {
                            paeth = upper_left;
                        }
                        recon = filtred + paeth;
                    }
                    break;
                default:
                    perror("[PNG] Unsupported filter type");
                    free_image(img);
                    free(prev_row);
                    free(curr_row);
                    free(raw_data);
                    return NULL;
            }
            curr_row[x] = recon;

        }

        // 4. Handle color types
        // Case 1 : Indexed color (color_type == 3)
        if (color_type == 3) {
            for (size_t x=0 ; x < width ; x++) {
                uint8_t index = curr_row[x];
                img->data[y * img->row_stride + x * 3 + 0] = palette[index][0]; // R
                img->data[y * img->row_stride + x * 3 + 1] = palette[index][1]; // G
                img->data[y * img->row_stride + x * 3 + 2] = palette[index][2]; // B
            }
        }
        // Case 2 : Direct color (RGB or Grayscale)
        else {
            memcpy(img->data + y * img->row_stride, curr_row, row_bytes);
        }

        // 5. Swap current and previous row buffers
        memcpy(prev_row, curr_row, row_bytes);

    }

    // 8. Cleanup
    free(raw_data);
    free(prev_row);
    free(curr_row);
    printf("[PNG] Image loaded successfully: %s\n", filepath);
    return img; 


}

