# parallel-image-filtering
 
project_root/
├── Makefile                # Smart build system (compiles modules separately)
├── README.md
│
├── include/                # PUBLIC HEADER FILES (The API)
│   ├── protocol.h          # Shared structs (Requests, Protocol Defines)
│   ├── image_api.h         # Public interface for Image Processing (Load/Save/Filter)
│   ├── ipc_wrapper.h       # Public interface for System Resources (SHM, Semaphores)
│   └── utils.h             # Logging, Error handling macros, Common types
│
├── src/
│   ├── client/             # CLIENT MODULE
│   │   └── main.c          # Client entry point (CLI parsing, Request sending)
│   │
│   ├── server/             # SERVER MODULE
│   │   ├── main.c          # Server entry point (Signal handling, Main loop)
│   │   └── process_mgr.c   # Logic to fork and manage worker lifecycles
│   │
│   ├── worker/             # WORKER MODULE
│   │   ├── worker_core.c   # The worker's main logic flow
│   │   └── thread_pool.c   # Logic to split work and manage threads
│   │
│   ├── image/              # IMAGE ENGINE (Business Logic Layer)
│   │   ├── image_core.c    # Generic image alloc/free/copy
│   │   ├── formats/        # EXTENSIBLE FORMAT DRIVERS
│   │   │   └── bmp.c       # BMP specific parsing
│   │   │   /* .png or .jpg drivers go here later */
│   │   └── filters/        # EXTENSIBLE FILTER ALGORITHMS
│   │       ├── grayscale.c
│   │       /* sobel.c, blur.c go here later */
│   │
│   └── ipc/                # INFRASTRUCTURE LAYER (System Wrappers)
│       ├── shm_manager.c   # Wraps shm_open, mmap, unmap
│       ├── fifo_manager.c  # Wraps mkfifo, open, read/write
│       └── sync.c          # Wraps sem_wait, sem_post
│
└── tests/                  # UNIT TESTS (Critical for Pro dev)
    ├── test_bmp_load.c
    └── test_shm_init.c