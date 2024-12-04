CC = gcc
CFLAGS = -Wall -Wextra -I./include -I./include/server -I./include/client -I./include/common -I./include/serialization -pthread
LDFLAGS = -pthread
DEBUG_FLAGS = -g -DDEBUG
RELEASE_FLAGS = -O2 -DNDEBUG

# Directories
SRC_DIR = src
TEST_DIR = tests
OBJ_DIR = obj
BIN_DIR = bin
INCLUDE_DIR = include

# Source files
SERVER_SRCS = $(wildcard $(SRC_DIR)/server/*.c)
CLIENT_SRCS = $(wildcard $(SRC_DIR)/client/*.c)
COMMON_SRCS = $(wildcard $(SRC_DIR)/common/*.c)
TEST_SRCS = $(wildcard $(TEST_DIR)/unit/*.c) $(wildcard $(TEST_DIR)/integration/*.c)

# Object files
SERVER_OBJS = $(SERVER_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
CLIENT_OBJS = $(CLIENT_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
COMMON_OBJS = $(COMMON_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TEST_OBJS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(OBJ_DIR)/test_%.o)

# Executables
SERVER_BIN = $(BIN_DIR)/trading_server
CLIENT_BIN = $(BIN_DIR)/trading_client
TEST_BIN = $(BIN_DIR)/run_tests

# Default target
all: dirs release

# Create necessary directories
dirs:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR)/server $(OBJ_DIR)/client $(OBJ_DIR)/common $(OBJ_DIR)/test/unit $(OBJ_DIR)/test/integration

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: dirs $(SERVER_BIN) $(CLIENT_BIN) $(TEST_BIN)

# Release build
release: CFLAGS += $(RELEASE_FLAGS)
release: dirs $(SERVER_BIN) $(CLIENT_BIN)

# Build server
$(SERVER_BIN): $(SERVER_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Build client
$(CLIENT_BIN): $(CLIENT_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Build tests
$(TEST_BIN): $(TEST_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) -lcunit

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile test files
$(OBJ_DIR)/test_%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(TEST_DIR)/mock -c $< -o $@

# Run tests
test: debug $(TEST_BIN)
	./$(TEST_BIN)

# Clean build files
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Install dependencies (Ubuntu/Debian)
deps:
	sudo apt-get update
	sudo apt-get install -y build-essential libcunit1-dev

.PHONY: all dirs debug release test clean deps
