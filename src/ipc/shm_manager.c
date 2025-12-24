#include "ipc_wrapper.h"
#include <fcntl.h>      // for O_CREAT, O_RDWR
#include <sys/mman.h>   // for shm_open, mmap, PROT_READ, PROT_WRITE, MAP_SHARED
#include <sys/stat.h>   // for S_IRUSR, S_IWUSR
#include <unistd.h>     // for ftruncate, close
#include <stdlib.h>     // for EXIT_FAILURE, exit



/* ---1. Implementation of shared memory management functions */

void* ipc_get_shared_memory(const char* name, size_t size , bool is_server) {

    int fd ; 
    int flags ; 

    // Determine flags based on whether it's server or client
    if(is_server) {
        flags = O_CREAT | O_RDWR ;
    }
    else {
        flags = O_RDWR ;
    }

    // Open or create the shared memory segment
    fd = shm_open(name , flags , 0666) ; 
    if(fd == -1) {
        return NULL ; 
    }

    // If server, set the size of the shared memory segment
    if(is_server) {
        if(ftruncate(fd , size) == -1) {
            perror("[IPC] ftruncate failed") ; 
            exit(EXIT_FAILURE) ;
        }
    }

    // Map the shared memory segment into the process's address space
    void* addr = mmap(NULL , size , PROT_READ | PROT_WRITE , MAP_SHARED , fd , 0) ;

    if(addr == MAP_FAILED) {
        perror("[IPC] mmap failed") ;
        exit(EXIT_FAILURE) ;
    } 

    // Close the file descriptor as it's no longer needed after mmap
    close(fd) ;

    return addr ;

}


void ipc_close_shared_memory(const char* name, void* addr, size_t size , bool is_server) {

    // Unmap the shared memory segment
    if(munmap(addr , size) == -1) {
        perror("[IPC] munmap failed") ;
        exit(EXIT_FAILURE) ;
    }   

    // If server, unlink the shared memory segment
    if(is_server) {
        if(shm_unlink(name) == -1) {
            perror("[IPC] shm_unlink failed") ;
            exit(EXIT_FAILURE) ;
        }
    }

}
