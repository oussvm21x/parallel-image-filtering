# ==========================================
#   Parallel Image Filtering Project
# ==========================================

# Compiler and Flags
CC = gcc
# -pthread: Enable threading support
# -Iinclude: Look for headers in the 'include' folder
# -g: Add debug symbols (useful for gdb/valgrind)
CFLAGS = -Wall -Wextra -pthread -Iinclude -g
LDFLAGS = -lrt -pthread -lm

# Directories
SRC_DIR = src
OBJ_DIR = obj

# --- Source Discovery (Automatic) ---
# IPC: shm_manager.c, fifo_manager.c, sync.c
IPC_SRCS = $(wildcard $(SRC_DIR)/ipc/*.c)

# Image Engine: core, bmp driver, filters (grayscale, etc.)
IMG_SRCS = $(wildcard $(SRC_DIR)/image/*.c) \
           $(wildcard $(SRC_DIR)/image/formats/*.c) \
           $(wildcard $(SRC_DIR)/image/filters/*.c)

# Worker Logic: worker_core.c
WORKER_SRCS = $(wildcard $(SRC_DIR)/worker/*.c)

# Main Entry Points
SERVER_MAIN = $(SRC_DIR)/server/main.c
CLIENT_MAIN = $(SRC_DIR)/client/main.c

# Helper Modules for Server (e.g., process manager)
SERVER_HELPER_SRCS = $(wildcard $(SRC_DIR)/server/*.c)
# Filter out main.c from helpers to avoid duplicate main definition if you had others
SERVER_HELPERS = $(filter-out $(SERVER_MAIN), $(SERVER_HELPER_SRCS))

# --- Object Generation ---
IPC_OBJS    = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(IPC_SRCS))
IMG_OBJS    = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(IMG_SRCS))
WORKER_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(WORKER_SRCS))
SERVER_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SERVER_MAIN) $(SERVER_HELPERS))
CLIENT_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(CLIENT_MAIN))

# --- Targets ---

all: directories server client

# 1. Server Executable
# Links: Server Logic + IPC + Worker Logic + Image Engine
server: $(SERVER_OBJS) $(IPC_OBJS) $(WORKER_OBJS) $(IMG_OBJS)
	@echo "[LINK] Creating Server..."
	$(CC) -o $@ $^ $(LDFLAGS)

# 2. Client Executable
# Links: Client Logic + IPC + Image Engine (for saving results)
client: $(CLIENT_OBJS) $(IPC_OBJS) $(IMG_OBJS)
	@echo "[LINK] Creating Client..."
	$(CC) -o $@ $^ $(LDFLAGS)

# 3. Generic Compile Rule
# Compiles any .c file into a .o file, preserving directory structure
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Create object directory
directories:
	@mkdir -p $(OBJ_DIR)

# Cleanup
clean:
	@echo "[CLEAN] Removing objects and executables..."
	rm -rf $(OBJ_DIR) server client
	rm -f processed_*.bmp
	rm -f /tmp/fifo_rep_*

# Phony targets (not real files)
.PHONY: all clean directories