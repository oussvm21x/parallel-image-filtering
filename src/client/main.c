#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>      
#include <sys/types.h>
#include <sys/stat.h>
#include <protocol.h>
#include <ipc_wrapper.h>
#include <image_api.h>

void print_usage(const char *prog_name) {
    printf("Usage: %s <image_path> <filter_id>\n", prog_name);
    printf("Filters:\n");
    printf("  1 - Grayscale\n");
    printf("  2 - Negative\n");
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

    printf("[Client %d] Starting...\n", getpid());

    // 2. Setup IPC mechanisms (shared memory and semaphore)
    // The client only needs to get access to existing IPC mechanisms
    // is_server = false (0)
    filter_request_t* shm_ptr = ipc_get_shared_memory(SHM_NAME, sizeof(filter_request_t), 0) ;
    sem_t * sem = ipc_get_semaphore("/sem_requests", 0) ;

    if (!shm_ptr || !sem) {
        fprintf(stderr, "[Client %d] Fatal: Could not access IPC mechanisms\n", getpid());
        return EXIT_FAILURE ;
    }

    // 3. Prepare the fifo 
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
    int retries = 0 ; 
    while(1) {
        // Lock the semaphore
        sem_wait(sem) ;
        // Check if the server is busy
        if (shm_ptr->pid == 0) {
            // Prepare the request
            shm_ptr->pid = getpid() ;
            strncpy(shm_ptr->chemin, image_path, MAX_PATH_LENGTH - 1) ;
            shm_ptr->chemin[MAX_PATH_LENGTH - 1] = '\0' ; // Ensure null-termination
            shm_ptr->filtre = filter_id ;   

            // Unlock the semaphore
            sem_post(sem) ;
            break;
        }
        // Unlock the semaphore
        sem_post(sem) ;

        // Server is busy, wait and retry
        usleep(10000) ; // 10 ms
        retries++ ;
        if (retries % 100 == 0) {
            printf("[Client %d] Waiting to send request... (retries: %d)\n", getpid(), retries);
        }
    }
    printf("[Client %d] Sent request to server.\n", getpid());

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
    snprintf(output_path, sizeof(output_path), "output_%d.png", getpid());
    if (save_image(output_path, img) != 0) {
        fprintf(stderr, "[Client %d] Error saving image to %s\n", getpid(), output_path);
    }
    else {
        printf("[Client] Success! Image saved to %s\n", output_path);
    }

    free_image(img) ; 
    close(fifo_fd) ; 
    unlink(fifo_path) ;

    //close IPC mechanisms
    ipc_close_shared_memory(SHM_NAME, shm_ptr, sizeof(filter_request_t), 0  ) ;
    ipc_close_semaphore("/sem_requests", sem, 0 ) ;


    return 0 ;
}