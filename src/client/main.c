#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>      
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <signal.h>
#include "protocol.h"
#include "ipc_wrapper.h"
#include "image_api.h"


/* --- 1. Helpers --- */
// Function to print usage instructions
void print_usage(const char *prog_name) {
    printf("Usage: %s <image_path> <filter_id>\n", prog_name);
    printf("Filters:\n");
    printf("  1 - Grayscale\n");
    printf("  2 - Negative\n");
    printf("  3 - Sepia tone\n");
}

// Signal handler for SIGINT to clean up resources
void handle_sigint(int sig) {
    (void)sig;
    // Check if the error come from the user(Ctrl+C) or the workers
    if(sig == SIGTERM){
        printf("[Client] Received SIGTERM, exiting...\n");
        exit(EXIT_FAILURE);
    }
    printf("[Client] Client Received SIGINT, cleaning up...\n");
    if(strlen(FIFO_RESPONSE_TEMPLATE) > 0) {
        char fifo_path[256];
        snprintf(fifo_path, sizeof(fifo_path), FIFO_RESPONSE_TEMPLATE, getpid());
        unlink(fifo_path);
    }
    exit(EXIT_FAILURE);
}

int main( int argc , char* argv[]) {

    // 1. Parse command-line arguments

    if(argc < 3) {
        perror("[Client] Error: Not enough arguments.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE ;   
    }

    const char* image_path = argv[1];
    int filter_id = atoi(argv[2]);

    // 2. Register signlas handler 
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    printf("[Client %d] Starting...\n", getpid());

    // 3. Setup IPC mechanisms (shared memory and semaphore)
    // 3.1 Connect to shared memory 

    // Map only the header part of the shared memory
    // to read table size 
    request_table_t * shm_ptr = ipc_get_shared_memory(SHM_NAME, sizeof(request_table_t), 0) ;
    if(!shm_ptr){
        perror("[Client] Error: Could not access Shared Memory");
        exit(EXIT_FAILURE);
    }
    int table_size = shm_ptr->size ;
    // Unmap the header part
    ipc_close_shared_memory(SHM_NAME, shm_ptr, sizeof(request_table_t), 0 ) ;

    // Now map the full shared memory
    size_t shm_size = sizeof(request_table_t) + table_size * sizeof(filter_request_t) ;
    shm_ptr = ipc_get_shared_memory(SHM_NAME, shm_size, 0) ;
    if(!shm_ptr){
        perror("[Client] Error: Could not access Shared Memory");
        exit(EXIT_FAILURE);
    }

    // 3.2 Connect to semaphore
    sem_t *sem_mutex = ipc_get_semaphore(SEM_MUTEX_NAME, 0, 0) ;
    if (!sem_mutex) {
        perror("[Client] Error: Could not access Mutex Semaphore") ;
        goto cleanup ;
    }

    sem_t *sem_empty = ipc_get_semaphore(SEM_EMPTY_NAME, 0, 0) ;
    if (!sem_empty) {
        perror("[Client] Error: Could not access Empty Semaphore") ;
        goto cleanup ;
    }

    sem_t *sem_full = ipc_get_semaphore(SEM_FULL_NAME, 0, 0) ;
    if (!sem_full) {
        perror("[Client] Error: Could not access Full Semaphore") ;
        goto cleanup ;
    }

    // 4. Prepare the fifo 
    char fifo_path[256];
    snprintf(fifo_path, sizeof(fifo_path), FIFO_RESPONSE_TEMPLATE, getpid());

    // Delete any existing FIFO with the same name
    unlink(fifo_path);

    if (ipc_create_fifo(fifo_path) == -1) {
        perror("[Client] Error creating FIFO for response");
        return EXIT_FAILURE ;
    }
    printf("[Client %d] Created FIFO at %s\n", getpid(), fifo_path);

    
    // 4. Send the request
    // 4.1 Wait for an empty slot
    if (sem_wait(sem_empty) == -1) {
        perror("[Client] sem_wait(sem_empty) failed") ;
        goto cleanup ;
    }

    // 4.2 Lock the mutex to access shared memory
    if (sem_wait(sem_mutex) == -1) {
        perror("[Client] sem_wait(sem_mutex) failed") ;
        // Release the empty semaphore before exiting
        sem_post(sem_empty) ;
        goto cleanup ;
    }
    // 4.3 Write the request to the circular buffer
    int index = shm_ptr->in ;
    shm_ptr->buffer[index].pid = getpid() ;
    strncpy(shm_ptr->buffer[index].chemin, image_path, MAX_PATH_LENGTH - 1) ;
    shm_ptr->buffer[index].chemin[MAX_PATH_LENGTH - 1] = '\0' ; // Ensure null-termination
    shm_ptr->buffer[index].filtre = filter_id ;
    // Initialize parameters to zero
    for(int i = 0; i < 5; i++) {
        shm_ptr->buffer[index].parametres[i] = 0 ;
    }

    // Update the in index
    shm_ptr->in = (shm_ptr->in + 1) % shm_ptr->size ;

    // 4.4 Unlock the mutex
    sem_post(sem_mutex) ;

    // 4.5 Signal that there is a new full slot
    sem_post(sem_full) ;
    printf("[Client %d] Sent request for image %s with filter %d\n", getpid(), image_path, filter_id);
    
    // 5. Wait for the response
    int fifo_fd = open(fifo_path, O_RDONLY);
    if (fifo_fd == -1) {
        perror("[Client] Error opening FIFO for reading");
        unlink(fifo_path);
        exit(EXIT_FAILURE);
    }

    // 6. Read the processed image from the FIFO
    // We assume workers send three values: width, height, and channels followed by pixel data
    size_t width, height, channels;
    if( read(fifo_fd, &width, sizeof(size_t)) != sizeof(size_t) ||
        read(fifo_fd, &height, sizeof(size_t)) != sizeof(size_t) ||
        read(fifo_fd, &channels, sizeof(size_t)) != sizeof(size_t)) {
        perror("[Client] Error reading image dimensions from FIFO");
        close(fifo_fd);
        unlink(fifo_path);
        exit(EXIT_FAILURE);
    }   

    printf("[Client %d] Received image dimensions: %zu x %zu x %zu\n", getpid(), width, height, channels);
    

    // 7. Receive pixel data
    image_t * img = create_image(width, height, channels);
    if (!img) {
        fprintf(stderr, "[Client %d] Error allocating memory for image\n", getpid());
        close(fifo_fd);
        unlink(fifo_path);
        exit(EXIT_FAILURE);
    }

    size_t expected_bytes = width * height * channels;
    size_t total_read = 0;
    while (total_read < expected_bytes) {
        ssize_t bytes_read = read(fifo_fd, img->data + total_read, expected_bytes - total_read);
        if (bytes_read <= 0) {
            perror("[Client] Error reading pixel data from FIFO");
            free_image(img);
            close(fifo_fd);
            unlink(fifo_path);
            exit(EXIT_FAILURE);
        }
        total_read += bytes_read;
    }

    if(total_read != expected_bytes) {
        fprintf(stderr, "[Client %d] Incomplete image data received\n", getpid());
        free_image(img);
        close(fifo_fd);
        unlink(fifo_path);
        exit(EXIT_FAILURE);
    }

    // 8. Save the received image to disk
    char output_path[300];

    // 8.1 Get the extension from the input image path
    const char* ext = strrchr(image_path, '.');
    if (!ext) {
        perror("[Client] Error: Input image path does not have an extension");
        free_image(img);
        close(fifo_fd);
        unlink(fifo_path);
        exit(EXIT_FAILURE);
    }



    snprintf(output_path, sizeof(output_path), "output_%d%s", getpid(), ext);
    if (save_image(output_path, img) != 0) {
        fprintf(stderr, "[Client %d] Error saving image to %s\n", getpid(), output_path);
    }
    else {
        printf("[Client] Success! Image saved to %s\n", output_path);
    }

    // 9. Cleanup
    free_image(img) ; 
    close(fifo_fd) ; 
cleanup:


    unlink(fifo_path) ;

    //close IPC mechanisms
    ipc_close_semaphore(SEM_MUTEX_NAME, sem_mutex, 0);
    ipc_close_semaphore(SEM_EMPTY_NAME, sem_empty, 0);
    ipc_close_semaphore(SEM_FULL_NAME, sem_full, 0);
    ipc_close_shared_memory(SHM_NAME, shm_ptr, shm_size, 0);


    return 0 ;
}