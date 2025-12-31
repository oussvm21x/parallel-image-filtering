#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include "protocol.h" 
#include "ipc_wrapper.h"


/* ---1. Global Vars ---*/
request_table_t * global_shm_ptr = NULL ;

sem_t *sem_mutex = NULL ;
sem_t *sem_empty = NULL ;
sem_t *sem_full = NULL ;

size_t shm_size = 0 ;

int server_running = 1 ;


/* --- 2. Forward Declaration ---*/
// We tell the compiler: "Trust me, this function exists in another file (worker_core.c)"
void worker_core(filter_request_t req) ; 

/* ---2. Signal Handler ---*/
// Handler for SIGINT 
void handle_sigint(int sig) {
    (void)sig; 
    printf("Server shutting down...\n");
    server_running = 0 ;
    // Unblock the sem_wait(sem_full) in the main loop
    if (sem_full != NULL) {
        sem_post(sem_full);
    }
}

// Hanlder for SIGCHLD
void handle_sigchld(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        // Reap all terminated child processes
    }
}

// ---3. Main Server Loop ---//

int main(int argc, char* argv[]) {

    // 1. Parse command line arguments
    if(argc < 2){
        perror("[Server] Usage: ./server <table_size> ") ;
        exit(EXIT_FAILURE) ;
    }

    int table_size = atoi(argv[1]) ;
    if(table_size <= 0){
        table_size = 10 ; // default size
    }

    // 2. Setup signal handlers
    signal(SIGINT, handle_sigint);
    signal(SIGCHLD, handle_sigchld);

    printf("[Server] Server is starting...\n");
    // 2. Setup IPC mechanisms (shared memory and semaphore)

    // 2.1 Shared Memory
    
    shm_size = sizeof(request_table_t) + table_size * sizeof(filter_request_t) ;
    global_shm_ptr = ipc_get_shared_memory(SHM_NAME, shm_size, 1) ;
    if (!global_shm_ptr) {
        fprintf(stderr, "[Server] Fatal: Could not create Shared Memory\n");
        exit(EXIT_FAILURE) ;
    }

    // 2.2 Initialize the request table
    // Its about initializing the circular buffer indexes and size
    // Initialize Header (Clear only the header struct size)
    memset(global_shm_ptr, 0, sizeof(request_table_t));
    
    global_shm_ptr->in = 0 ;
    global_shm_ptr->out = 0 ;
    global_shm_ptr->size = table_size ;

    // 3.2 Semaphore
    // Mutex semaphore to protect access to the shared memory
    sem_mutex = ipc_get_semaphore(SEM_MUTEX_NAME, 1, 1) ;
    if (!sem_mutex) {
        fprintf(stderr, "[Server] Fatal: Could not create Mutex Semaphore\n");
        exit(EXIT_FAILURE) ;
    }

    // Semaphore to count empty slots in the request table
    sem_empty = ipc_get_semaphore(SEM_EMPTY_NAME, table_size, 1) ;
    if (!sem_empty) {
        fprintf(stderr, "[Server] Fatal: Could not create Empty Semaphore\n");
        exit(EXIT_FAILURE) ;
    }

    // Semaphore to count full slots in the request table
    sem_full = ipc_get_semaphore(SEM_FULL_NAME, 0, 1) ;
    if (!sem_full) {
        fprintf(stderr, "[Server] Fatal: Could not create Full Semaphore\n");
        exit(EXIT_FAILURE) ;
    }

    printf("[Server] Server is ready to accept requests...\n");

    // 3. Main loop to handle requests
    while (server_running) {

        // Wait for item available 
        if (sem_wait(sem_full) == -1) {
            if (server_running == 0) {
                break; // Exit loop if server is shutting down
            }
            perror("[Server] sem_wait(sem_full) failed") ;
            continue ;
        }

        // Check if we were woken up due to shutdown signal
        if (server_running == 0) {
            break;
        }

        // Lock the mutex to access shared memory
        if (sem_wait(sem_mutex) == -1) {
            if (server_running == 0) {
                break; // Exit loop if server is shutting down
            }
            perror("[Server] sem_wait(sem_mutex) failed") ;
            // Release the full semaphore to avoid deadlock
            sem_post(sem_full) ;
            continue ;
        }

        // Read the request from the circular buffer
        filter_request_t local_request = global_shm_ptr->buffer[global_shm_ptr->out] ;
        global_shm_ptr->out = (global_shm_ptr->out + 1) % global_shm_ptr->size ;

        // Unlock the mutex
        sem_post(sem_mutex) ;

        // Signal that there is a new empty slot
        sem_post(sem_empty) ;

        printf("[Server] Received request from PID %d\n", local_request.pid);
        printf("         Image Path: %s\n", local_request.chemin);
        printf("         Filter Type: %d\n", local_request.filtre); 

        // Fork a worker process to handle the request
        pid_t pid = fork() ;
        switch(pid) {
            case -1 : 
                perror("[Server] Fork failed") ;
                exit(EXIT_FAILURE) ;
            case 0  :
                // Child process (worker)
                worker_core(local_request) ;
                exit(EXIT_SUCCESS) ;
            
            default :
                // Parent process (server)      
                printf("[Server] Spawned worker process with PID %d for client PID %d\n", pid, local_request.pid);
                break ;
        }

    }

    // 4. Cleanup IPC mechanisms
    printf("[Server] Cleaning up IPC resources...\n");
    ipc_close_semaphore(SEM_MUTEX_NAME, sem_mutex, 1) ;
    ipc_close_semaphore(SEM_EMPTY_NAME, sem_empty, 1) ;
    ipc_close_semaphore(SEM_FULL_NAME, sem_full, 1) ;
    ipc_close_shared_memory(SHM_NAME, global_shm_ptr, shm_size, 1) ;
    printf("[Server] Server has shut down gracefully.\n");
    return 0 ;

}


