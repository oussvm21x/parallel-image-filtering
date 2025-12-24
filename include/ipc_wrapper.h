#ifndef IPC_WRAPPER_H
#define IPC_WRAPPER_H

#include <unistd.h>    // for pid_t
#include <stddef.h>    // size_t, NULL
#include <semaphore.h> // for sem_t
#include <stdbool.h>   // for bool

/* ---1. Shared Memory ---*/

/*
* A Wrapper function to create or get a shared memory segment.
* Parameters:
*   - name: Name of the shared memory segment.
*   - size: Size of the shared memory segment.
*   - is_server: If true, create the shared memory segment; if false, just get it.
* Returns:
*   - Pointer to the shared memory segment, or NULL on failure.
*/

void* ipc_get_shared_memory(const char* name, size_t size , bool is_server) ; 


/*
* A Wrapper function to close and optionally unlink a shared memory segment.
* Parameters:
*   - name: Name of the shared memory segment.
*   - addr: Pointer to the shared memory segment.
*   - size: Size of the shared memory segment.
*   - is_server: If true, unlink the shared memory segment by calling shm_unlink.
* Returns:
*   - void
*/
void ipc_close_shared_memory(const char* name, void* addr, size_t size , bool is_server) ;


/* ---2. Semaphores ---*/

/*
* A Wrapper function to create or get a named semaphore.
* Parameters:
*   - name: Name of the semaphore.
*   - is_server: If true, create the semaphore and initialize it; if false, just get it.
* Returns:
*   - Pointer to the semaphore, or NULL on failure.
*/  
sem_t* ipc_get_semaphore(const char* name, bool is_server) ;


/*
* A Wrapper function to close and optionally unlink a named semaphore.
* Parameters:
*   - name: Name of the semaphore.
*   - sem: Pointer to the semaphore.
*   - is_server: If true, unlink the semaphore by calling sem_unlink.
* Returns:
*   - void
*/
void ipc_close_semaphore(const char* name, sem_t* sem, bool is_server) ;


/* ---3. FIFOs */

/* 
* A Wrapper function to create a FIFO (named pipe) .
* Technically , it's a thin wrapper over mkfifo system call.
* Parameters:
*   - path: Path of the FIFO to create.
* Returns:
*   - 0 on success, -1 on failure.
*/
int ipc_create_fifo(const char* path) ;

#endif // IPC_WRAPPER_H