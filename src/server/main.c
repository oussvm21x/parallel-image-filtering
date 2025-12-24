#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include "protocol.h" 
#include "ipc_wrapper.h"


/* ---1. Global Vars ---*/
filter_request_t* global_shm_ptr = NULL ; 
sem_t  * global_sem = NULL ;
int server_running = 1 ;


/* --- Temp func declaration ---*/
void worker_main(filter_request_t request) ;

/* ---2. Signal Handler ---*/
// Handler for SIGINT 
void handle_sigint(int sig) {
    (void)sig; 
    printf("Server shutting down...\n");
    server_running = 0 ;
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

    // 1. Setup signal handlers
    signal(SIGINT, handle_sigint);
    signal(SIGCHLD, handle_sigchld);

    printf("Server is starting...\n");

    // 2. Setup IPC mechanisms (shared memory and semaphore)

    // 2.1 Shared Memory

    global_shm_ptr = ipc_get_shared_memory(SHM_NAME, sizeof(filter_request_t), 1) ;
    if (!global_shm_ptr) {
        fprintf(stderr, "[Server] Fatal: Could not create Shared Memory\n");
        return 1;
    }

    // 2.2 Semaphore
    global_sem = ipc_get_semaphore("/sem_requests", 1) ;
    if (!global_sem) {
        fprintf(stderr, "[Server] Fatal: Could not create Semaphore\n");

        // Cleanup shared memory since semaphore creation failed 
        ipc_close_shared_memory(SHM_NAME, global_shm_ptr, sizeof(filter_request_t), 1) ;
        return 1;
    }

    // Initialize shared memory to zero to avoid garbage values
    memset(global_shm_ptr, 0, sizeof(filter_request_t)) ;
    printf("Server is ready to accept requests...\n");

    // 3. Main loop to handle requests
    while (server_running) {

        // Lock the semaphore to wait for a new request
        sem_wait(global_sem) ;

        // Check is there are pending requests
        // if pid is 0, no pending requests 
        // else, there is a pending request from client with pid (global_shm_ptr->pid)
        if (global_shm_ptr->pid != 0) {
            // Copy the request locally 
            // to avoid issues with concurrent access 
            // this make the server more robust and faster
            filter_request_t local_request  = *global_shm_ptr ;

            // Reset the shared memory pid to 0 to indicate request is being processed
            global_shm_ptr->pid = 0 ;

            // Unlock the semaphore
            sem_post(global_sem) ;

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
                    worker_main(local_request) ;
                    exit(EXIT_SUCCESS) ;
                
                default :
                    // Parent process (server)      
                    printf("[Server] Spawned worker process with PID %d for client PID %d\n", pid, local_request.pid);
                    break ;
            }

        }
        else {
            // No pending requests, just unlock the semaphore
            sem_post(global_sem) ;
            // Sleep briefly to avoid busy waiting
            usleep(100000) ; // 100 ms
        }
    }

    // 4. Cleanup IPC mechanisms
    ipc_close_semaphore("/sem_requests", global_sem, 1) ;
    ipc_close_shared_memory(SHM_NAME, global_shm_ptr, sizeof(filter_request_t), 1) ;

    printf("Server has shut down gracefully.\n");
    return 0 ;

}


/* Temporary worker function */
void worker_main(filter_request_t request) {
    printf("[Worker %d] Processing request for image: %s with filter %d\n", getpid(), request.chemin, request.filtre);
    // Simulate processing time
    sleep(2) ;
    printf("[Worker %d] Finished processing request for image: %s\n", getpid(), request.chemin);
}


