
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <unistd.h>    // for pid_t
#include <sys/types.h> // for pid_t on some platforms
#include <stdint.h>    // fixed-width integer types
#include <stddef.h>    // size_t, NULL

/* --- 1. System Constants ---*/

#define SHM_NAME "/shm_requests" // Shared memory name
#define MAX_PATH_LENGTH 256      // Maximum length for file paths
#define FIFO_RESPONSE_TEMPLATE "/tmp/fifo_rep_%d" // Template for response FIFO names 




/* --- 2. Filter Types ---*/
#define FILTER_GRAYSCALE 1
#define FILTER_NEGATIVE 2
#define FILTER_BLUR 3


/* --- 3. Request Structure ---*/
// Given by the teacher
typedef struct filter_request {
pid_t pid ;
char chemin [256];
int filtre ;
int parametres [5];
} filter_request_t ;



#endif // PROTOCOL_H