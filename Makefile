# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -pthread -Iinclude -g
LDFLAGS = -lrt -pthread -lm

# Directories
SRC_DIR = src
OBJ_DIR = obj

# Sources
# Find all .c files in specific directories
IPC_SRCS = $(wildcard $(SRC_DIR)/ipc/*.c)
# Note: This picks up your new image_stubs.c automatically
IMG_SRCS = $(wildcard $(SRC_DIR)/image/*.c) $(wildcard $(SRC_DIR)/image/formats/*.c) $(wildcard $(SRC_DIR)/image/filters/*.c)
WORKER_SRCS = $(wildcard $(SRC_DIR)/worker/*.c)
SERVER_SRCS = $(SRC_DIR)/server/main.c $(SRC_DIR)/server/process_mgr.c
CLIENT_SRCS = $(SRC_DIR)/client/main.c

# Object Files
# Convert src/xxx/file.c -> obj/xxx/file.o
IPC_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(IPC_SRCS))
IMG_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(IMG_SRCS))
WORKER_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(WORKER_SRCS))
SERVER_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SERVER_SRCS))
CLIENT_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(CLIENT_SRCS))

# Targets
all: directories server client

# Link Server (Needs IPC, Image Logic, and Worker logic)
server: $(SERVER_OBJS) $(IPC_OBJS) $(WORKER_OBJS) $(IMG_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Link Client (Needs IPC logic and Image Logic for saving)
client: $(CLIENT_OBJS) $(IPC_OBJS) $(IMG_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile generic rule (Handles nested directories)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Create directories
directories:
	@mkdir -p $(OBJ_DIR)

# Clean
clean:
	rm -rf $(OBJ_DIR) server client
	rm -f /tmp/fifo_rep_*

.PHONY: all clean directories