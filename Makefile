CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
LDFLAGS = 

SRC_DIR = src
OBJ_DIR = build
INCLUDE_DIR = include
EXAMPLE_DIR = examples

# Source files
SRCS_C = $(wildcard $(SRC_DIR)/*.c)
SRCS_ASM = $(wildcard $(SRC_DIR)/*.S)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS_C)) \
       $(patsubst $(SRC_DIR)/%.S, $(OBJ_DIR)/%.o, $(SRCS_ASM))

# Targets
TARGET_LIB = libgthread.a
EXAMPLES = basic_threads stride_test stack_test mutex_test sleep_test io_test http_server matrix_mul runner

all: $(TARGET_LIB) $(EXAMPLES)

$(TARGET_LIB): $(OBJS)
	ar rcs $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.S | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Example Build Rules
basic_threads: $(EXAMPLE_DIR)/basic_threads.c $(TARGET_LIB)
	$(CC) $(CFLAGS) $< -o build/$@ -L. -lgthread

stride_test: $(EXAMPLE_DIR)/stride_test.c $(TARGET_LIB)
	$(CC) $(CFLAGS) $< -o build/$@ -L. -lgthread

stack_test: $(EXAMPLE_DIR)/stack_test.c $(TARGET_LIB)
	$(CC) $(CFLAGS) $< -o build/$@ -L. -lgthread

mutex_test: $(EXAMPLE_DIR)/mutex_test.c $(SRC_DIR)/sync.c $(TARGET_LIB)
	$(CC) $(CFLAGS) $< $(SRC_DIR)/sync.c -o build/$@ -L. -lgthread

sleep_test: $(EXAMPLE_DIR)/sleep_test.c $(TARGET_LIB)
	$(CC) $(CFLAGS) $< -o build/$@ -L. -lgthread

io_test: $(EXAMPLE_DIR)/io_test.c $(TARGET_LIB)
	$(CC) $(CFLAGS) $< -o build/$@ -L. -lgthread

http_server: $(EXAMPLE_DIR)/http_server.c $(TARGET_LIB)
	$(CC) $(CFLAGS) $< -o build/$@ -L. -lgthread

matrix_mul: $(EXAMPLE_DIR)/matrix_mul.c $(TARGET_LIB)
	$(CC) $(CFLAGS) $< -o build/$@ -L. -lgthread

runner: $(EXAMPLE_DIR)/runner.c
	$(CC) $(CFLAGS) $< -o build/$@

clean:
	rm -rf $(OBJ_DIR) $(TARGET_LIB)

.PHONY: all clean
