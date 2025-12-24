#include "ipc_wrapper.h"
#include <stdlib.h>
#include <sys/mman.h>   // for munmap
#include <unistd.h>     // for close
#include <stdio.h>      // for perror
#include <errno.h>      // for errno
#include <sys/types.h>
#include <sys/stat.h>  // <--- Required for mkfifo

/* ---1. Implemntation of  FIFOs ( Tubes ) */

int ipc_create_fifo(const char* path) {

    if(mkfifo(path , 0666) == -1) {
        // If the FIFO already exists, it's not an error for our use case
        if (errno != EEXIST) {
            perror("[IPC] mkfifo failed") ;
            return -1 ;
        }
    } 
    return 0 ;

}