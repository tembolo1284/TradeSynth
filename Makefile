CC = gcc
CFLAGS = -Wall -Wextra -I./include -pthread
LDFLAGS = -pthread -lm
DEBUG_FLAGS = -g -DDEBUG 
RELEASE_FLAGS = -O2 -DNDEBUG
TEST_LDFLAGS = -lcriterion

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

# Source files
SERVER_SRCS = $(wildcard $(SERVER_SRC)/*.c)
CLIENT_SRCS = $(wildcard $(CLIENT_SRC)/*.c)
COMMON_SRCS = $(wildcard $(COMMON_SRC)/*.c)
SERIAL_SRCS = $(wildcard $(SERIAL_SRC)/*.c)
TEST_SRCS = $(wildcard $(TEST_DIR)/unit/*.c) $(wildcard $(TEST_DIR)/integration/*.c)

# Example source files
EXAMPLE_DIRS = $(wildcard $(EXAMPLES_DIR)/*)
EXAMPLE_SERVER_SRCS = $(wildcard $(EXAMPLES_DIR)/*/example_server.c)
EXAMPLE_CLIENT_SRCS = $(wildcard $(EXAMPLES_DIR)/*/example_client.c)
EXAMPLE_SERVER_BINS = $(patsubst $(EXAMPLES_DIR)/%/example_server.c,$(BIN_DIR)/%_server,$(EXAMPLE_SERVER_SRCS))
EXAMPLE_CLIENT_BINS = $(patsubst $(EXAMPLES_DIR)/%/example_client.c,$(BIN_DIR)/%_client,$(EXAMPLE_CLIENT_SRCS))
EXAMPLE_BINS = $(EXAMPLE_SERVER_BINS) $(EXAMPLE_CLIENT_BINS)

# Object files
SERVER_OBJS = $(SERVER_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
CLIENT_OBJS = $(CLIENT_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
COMMON_OBJS = $(COMMON_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
SERIAL_OBJS = $(SERIAL_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TEST_OBJS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(OBJ_DIR)/test_%.o)

# Main executables
SERVER_BIN = $(BIN_DIR)/trading_server
CLIENT_BIN = $(BIN_DIR)/trading_client
TEST_BIN = $(BIN_DIR)/run_tests

# Default target
all: dirs release

dirs:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)/server
	@mkdir -p $(OBJ_DIR)/client
	@mkdir -p $(OBJ_DIR)/common
	@mkdir -p $(OBJ_DIR)/serialization
	@mkdir -p $(OBJ_DIR)/test/unit
	@mkdir -p $(OBJ_DIR)/test/integration
	@mkdir -p $(foreach dir,$(EXAMPLE_DIRS),$(OBJ_DIR)/examples/$(notdir $(dir)))

debug: CFLAGS += $(DEBUG_FLAGS)
debug: dirs $(SERVER_BIN) $(CLIENT_BIN) $(TEST_BIN) examples

release: CFLAGS += $(RELEASE_FLAGS)
release: dirs $(SERVER_BIN) $(CLIENT_BIN) examples

$(SERVER_BIN): $(SERVER_OBJS) $(COMMON_OBJS) $(SERIAL_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(CLIENT_BIN): $(CLIENT_OBJS) $(COMMON_OBJS) $(SERIAL_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_BIN): $(TEST_OBJS) $(COMMON_OBJS) $(SERIAL_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(TEST_LDFLAGS)

examples: $(EXAMPLE_BINS)

$(BIN_DIR)/%_server: $(OBJ_DIR)/examples/%/example_server.o $(SERVER_OBJS:$(OBJ_DIR)/server/main.o=) $(COMMON_OBJS) $(SERIAL_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/%_client: $(OBJ_DIR)/examples/%/example_client.o $(CLIENT_OBJS:$(OBJ_DIR)/client/main.o=) $(COMMON_OBJS) $(SERIAL_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/test_%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(TEST_DIR)/mock -c $< -o $@

$(OBJ_DIR)/examples/%/example_server.o: $(EXAMPLES_DIR)/%/example_server.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/examples/%/example_client.o: $(EXAMPLES_DIR)/%/example_client.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

test: debug $(TEST_BIN)
	./$(TEST_BIN)

run-examples: examples
	for example in $(EXAMPLE_SERVER_BINS); do \
		echo "Starting $$example..."; \
		$$example & \
		sleep 1; \
	done
	for example in $(EXAMPLE_CLIENT_BINS); do \
		echo "Running $$example..."; \
		$$example; \
	done
	pkill -f "$(BIN_DIR)/*server"

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

deps:
	sudo apt-get update
	sudo apt-get install -y build-essential libcunit1-dev

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
