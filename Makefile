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
EXAMPLES_DIR = examples

# Source directories
SERVER_SRC = $(SRC_DIR)/server
CLIENT_SRC = $(SRC_DIR)/client
COMMON_SRC = $(SRC_DIR)/common
SERIAL_SRC = $(SRC_DIR)/serialization
EXAMPLES_SRC = $(EXAMPLES_DIR)/trading_communication

# Source files
SERVER_SRCS = $(wildcard $(SERVER_SRC)/*.c)
CLIENT_SRCS = $(wildcard $(CLIENT_SRC)/*.c)
COMMON_SRCS = $(wildcard $(COMMON_SRC)/*.c)
SERIAL_SRCS = $(wildcard $(SERIAL_SRC)/*.c)
TEST_SRCS = $(wildcard $(TEST_DIR)/unit/*.c) $(wildcard $(TEST_DIR)/integration/*.c)
EXAMPLES_SRCS = $(wildcard $(EXAMPLES_SRC)/*.c)

# Object files
SERVER_OBJS = $(SERVER_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
CLIENT_OBJS = $(CLIENT_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
COMMON_OBJS = $(COMMON_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
SERIAL_OBJS = $(SERIAL_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TEST_OBJS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(OBJ_DIR)/test_%.o)
EXAMPLES_OBJS = $(EXAMPLES_SRCS:$(EXAMPLES_DIR)/%.c=$(OBJ_DIR)/examples/%.o)

# Executables
SERVER_BIN = $(BIN_DIR)/trading_server
CLIENT_BIN = $(BIN_DIR)/trading_client
TEST_BIN = $(BIN_DIR)/run_tests
EXAMPLE_SERVER_BIN = $(BIN_DIR)/example_server
EXAMPLE_CLIENT_BIN = $(BIN_DIR)/example_client

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
		$(OBJ_DIR)/test/integration \
		$(OBJ_DIR)/examples/trading_communication

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: dirs $(SERVER_BIN) $(CLIENT_BIN) $(TEST_BIN) examples

# Release build
release: CFLAGS += $(RELEASE_FLAGS)
release: dirs $(SERVER_BIN) $(CLIENT_BIN) examples

# Build server
$(SERVER_BIN): $(SERVER_OBJS) $(COMMON_OBJS) $(SERIAL_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Build client
$(CLIENT_BIN): $(CLIENT_OBJS) $(COMMON_OBJS) $(SERIAL_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Build tests
$(TEST_BIN): $(TEST_OBJS) $(COMMON_OBJS) $(SERIAL_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) -lcunit

# Build examples
examples: $(EXAMPLE_SERVER_BIN) $(EXAMPLE_CLIENT_BIN)

$(EXAMPLE_SERVER_BIN): $(OBJ_DIR)/examples/trading_communication/example_server.o $(COMMON_OBJS) $(SERIAL_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(EXAMPLE_CLIENT_BIN): $(OBJ_DIR)/examples/trading_communication/example_client.o $(COMMON_OBJS) $(SERIAL_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile test files
$(OBJ_DIR)/test_%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(TEST_DIR)/mock -c $< -o $@

# Compile example files
$(OBJ_DIR)/examples/%.o: $(EXAMPLES_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Run tests
test: debug $(TEST_BIN)
	./$(TEST_BIN)

# Run examples
run-examples: examples
	@echo "Starting example server..."
	./$(EXAMPLE_SERVER_BIN) &
	@sleep 1
	@echo "Starting example client..."
	./$(EXAMPLE_CLIENT_BIN)
	@pkill -f $(EXAMPLE_SERVER_BIN)

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
	@echo "  examples     - Build example programs"
	@echo "  run-examples - Build and run example programs"
	@echo "  clean        - Remove build artifacts"
	@echo "  deps         - Install system dependencies"
	@echo "  dirs         - Create necessary build directories"
	@echo "  help         - Show this help message"

.PHONY: all dirs debug release test clean deps help examples run-examples
