#include "ipc_wrapper.h"
#include <stdlib.h>
#include <sys/mman.h>   // for munmap
#include <unistd.h>     // for close
#include <stdio.h>      // for perror
#include <sys/shm.h>    // for shm_unlink
#include <fcntl.h>      // for O_CREAT, O_RDWR
#include <sys/stat.h>   // for S_IRUSR, S_IWUSR


sem_t* ipc_get_semaphore(const char* name, bool is_server) {

    sem_t* sem ;

    if(is_server) {
        // Create the semaphore with initial value 1
        sem = sem_open(name , O_CREAT , 0666 , 1) ;
    }
    else {
        // Open the existing semaphore by client
        sem = sem_open(name , 0) ;
    }

    if(sem == SEM_FAILED) {
        perror("[IPC] sem_open failed") ;
        exit(EXIT_FAILURE) ;
    }

    return sem ;

}



void ipc_close_semaphore(const char* name, sem_t* sem, bool is_server) {

    // Close the semaphore
    if(sem_close(sem) == -1) {
        perror("[IPC] sem_close failed") ;
        exit(EXIT_FAILURE) ;
    }

    // If server, unlink the semaphore
    if(is_server) {
        if(sem_unlink(name) == -1) {
            perror("[IPC] sem_unlink failed") ;
            exit(EXIT_FAILURE) ;    
        }
    }
}
