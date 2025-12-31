
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
#define SEM_MUTEX_NAME "/sem_mutex" // Semaphore name for mutual exclusion
#define SEM_EMPTY_NAME "/sem_empty" // Semaphore name for empty slots
#define SEM_FULL_NAME "/sem_full"   // Semaphore name for full slots


/* --- 2. Request Structure ---*/
// Given by the teacher
typedef struct filter_request {
pid_t pid ;
char chemin [256];
int filtre ;
int parametres [5];
} filter_request_t ;


/* --- 3. Shared Memory Structure ---*/
typedef struct request_table_t {
    size_t in;    // Index for the next write
    size_t out;   // Index for the next read
    size_t size;  // Size of the buffer
    filter_request_t buffer[]; // Circular buffer for requests
} request_table_t ;


#endif // PROTOCOL_H