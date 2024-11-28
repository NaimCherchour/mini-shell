# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Iheaders 
LDFLAGS = -lreadline

# Directories
SRC_DIR = src
LOCALS_DIR = $(SRC_DIR)/locals
HEADERS_DIR = headers
BIN_DIR = bin
OBJ_DIR = obj
TESTS_DIR = tests

# Output Executable
TARGET = fsh

# Source Files
MAIN_SRC = $(SRC_DIR)/main.c
LOCALS_SRCS = $(wildcard $(LOCALS_DIR)/*.c)
OTHER_SRCS = $(filter-out $(MAIN_SRC), $(wildcard $(SRC_DIR)/*.c))

# Object Files
LOCALS_BINS = $(patsubst $(LOCALS_DIR)/%.c,$(BIN_DIR)/%,$(LOCALS_SRCS))
OTHER_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(OTHER_SRCS))

# Default Target
all: $(LOCALS_BINS) $(TARGET)

# Build standalone binaries in bin/
$(BIN_DIR)/%: $(LOCALS_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Build fsh executable
$(TARGET): $(OTHER_OBJS) $(MAIN_SRC)
	$(CC) $(CFLAGS) -o $@ $(MAIN_SRC) $(OTHER_OBJS) $(LDFLAGS)

# Compile src/*.c (except main.c) to obj/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create directories if they don't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean up build artifacts
clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR) $(TARGET)

# Print debug information
debug:
	@echo "LOCALS_BINS: $(LOCALS_BINS)"
	@echo "OTHER_OBJS: $(OTHER_OBJS)"

test:
	@for file in $(TESTS_DIR)/*.sh; do \
		echo "Running test: $$file"; \
		$$file; \
	done

help:
	@echo "Usage: make [all|clean|debug|help]"