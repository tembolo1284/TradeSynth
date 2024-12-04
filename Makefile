CC = gcc
CFLAGS = -Wall -Wextra -I./include -pthread
LDFLAGS = -pthread -lm
DEBUG_FLAGS = -g -DDEBUG
RELEASE_FLAGS = -O2 -DNDEBUG

# Directories
SRC_DIR = src
TEST_DIR = tests
OBJ_DIR = obj
BIN_DIR = bin

# Source directories
SERVER_SRC = $(SRC_DIR)/server
CLIENT_SRC = $(SRC_DIR)/client
COMMON_SRC = $(SRC_DIR)/common
SERIAL_SRC = $(SRC_DIR)/serialization

# Source files
SERVER_SRCS = $(wildcard $(SERVER_SRC)/*.c)
CLIENT_SRCS = $(wildcard $(CLIENT_SRC)/*.c)
COMMON_SRCS = $(wildcard $(COMMON_SRC)/*.c)
SERIAL_SRCS = $(wildcard $(SERIAL_SRC)/*.c)
TEST_SRCS = $(wildcard $(TEST_DIR)/unit/*.c) $(wildcard $(TEST_DIR)/integration/*.c)

# Object files
SERVER_OBJS = $(SERVER_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
CLIENT_OBJS = $(CLIENT_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
COMMON_OBJS = $(COMMON_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
SERIAL_OBJS = $(SERIAL_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TEST_OBJS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(OBJ_DIR)/test_%.o)

# Executables
SERVER_BIN = $(BIN_DIR)/trading_server
CLIENT_BIN = $(BIN_DIR)/trading_client
TEST_BIN = $(BIN_DIR)/run_tests

# Default target
all: dirs release

# Create necessary directories
dirs:
	@mkdir -p $(BIN_DIR) \
		$(OBJ_DIR)/server \
		$(OBJ_DIR)/client \
		$(OBJ_DIR)/common \
		$(OBJ_DIR)/serialization \
		$(OBJ_DIR)/test/unit \
		$(OBJ_DIR)/test/integration

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: dirs $(SERVER_BIN) $(CLIENT_BIN) $(TEST_BIN)

# Release build
release: CFLAGS += $(RELEASE_FLAGS)
release: dirs $(SERVER_BIN) $(CLIENT_BIN)

# Build server
$(SERVER_BIN): $(SERVER_OBJS) $(COMMON_OBJS) $(SERIAL_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Build client
$(CLIENT_BIN): $(CLIENT_OBJS) $(COMMON_OBJS) $(SERIAL_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Build tests
$(TEST_BIN): $(TEST_OBJS) $(COMMON_OBJS) $(SERIAL_OBJS)
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

# Install dependencies
deps:
	sudo apt-get update
	sudo apt-get install -y build-essential libcunit1-dev

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Default target, same as 'release'"
	@echo "  debug        - Build debug version with symbols"
	@echo "  release      - Build optimized release version"
	@echo "  test         - Build and run tests"
	@echo "  clean        - Remove build artifacts"
	@echo "  deps         - Install system dependencies"
	@echo "  dirs         - Create necessary build directories"
	@echo "  help         - Show this help message"

.PHONY: all dirs debug release test clean deps help
