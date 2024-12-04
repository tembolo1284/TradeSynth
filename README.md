# TradeSynth

A high-performance, multithreaded trading system implementation in C that handles real-time market data processing and order management through a robust client-server architecture.

## Features

- **High-Performance Networking**: Efficient client-server communication using multithreading and socket programming
- **Market Data Processing**: Real-time handling of market data updates and order flow
- **Robust Serialization**: Custom serialization protocol for complex financial data structures
- **Thread-Safe Operations**: Concurrent handling of multiple client connections
- **Comprehensive Logging**: Multi-level logging system with performance tracking
- **Containerized Deployment**: Docker support for easy deployment and scaling

## Quick Start

```bash
# Clone the repository
git clone <project_url>
cd tradesynth

# Make scripts executable
chmod +x scripts/docker-run.sh

# Build the Docker image
./scripts/docker-run.sh build

# Start the server (with default INFO level logging)
./scripts/docker-run.sh run

# Start a client (with DEBUG level logging)
./scripts/docker-run.sh run-client 1

# Check status
./scripts/docker-run.sh status

# View logs
./scripts/docker-run.sh logs

# Stop all containers
./scripts/docker-run.sh stop

# Clean up everything
./scripts/docker-run.sh clean
```

## Docker Script Usage

The `docker-run.sh` script provides a convenient way to manage the TradeSynth containers:

```bash
Usage: ./docker-run.sh <command> [log_level]

Commands:
  build       - Build Docker image
  run         - Run the server container
  run-client  - Run a client container
  stop        - Stop all containers
  clean       - Remove containers and images
  logs        - Show container logs
  status      - Show container status
  test        - Run tests in container

Log Levels:
  0 - TRACE
  1 - DEBUG
  2 - INFO (default)
  3 - WARN
  4 - ERROR
  5 - FATAL
```

## Prerequisites

### Local Development
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libcunit1-dev \
    git

# CentOS/RHEL
sudo yum update
sudo yum groupinstall "Development Tools"
sudo yum install cmake cunit-devel
```

### Docker Development
- Docker Engine 20.10+
- Docker Compose v2.0+

## Logging System

### Log Levels

TradeSynth implements a comprehensive logging system with six levels of granularity:

1. **TRACE** (`LOG_TRACE`): Most detailed level, for in-depth debugging
   - Function entry/exit points
   - Variable values during computation
   - Memory allocations/deallocations

2. **DEBUG** (`LOG_DEBUG`): Development-time debugging information
   - Detailed flow control
   - State changes
   - Important variable values

3. **INFO** (`LOG_INFO`): General operational information
   - Application startup/shutdown
   - Connection establishment/termination
   - Order processing status

4. **WARN** (`LOG_WARN`): Warning messages for concerning but non-critical issues
   - System resource running low
   - Retryable failures
   - Deprecated feature usage

5. **ERROR** (`LOG_ERROR`): Error messages for serious issues
   - Failed operations
   - Connection losses
   - Data corruption

6. **FATAL** (`LOG_FATAL`): Critical errors requiring immediate shutdown
   - Unrecoverable system states
   - Critical resource exhaustion
   - Security breaches

### Configuring Log Levels

#### Using docker-run.sh
```bash
# Run server with TRACE level logging
./scripts/docker-run.sh run 0

# Run client with DEBUG level logging
./scripts/docker-run.sh run-client 1
```

#### Environment Variables
```bash
# Set minimum log level (TRACE=0, DEBUG=1, INFO=2, WARN=3, ERROR=4, FATAL=5)
export TRADESYNTH_LOG_LEVEL=2  # Set to INFO level

# Set log file path
export TRADESYNTH_LOG_FILE="/var/log/tradesynth/tradesynth.log"
```

#### Runtime Configuration
```c
// In your application code
init_logger("/var/log/tradesynth/tradesynth.log", LOG_INFO);
```

### Log Format

Each log entry includes:
- Timestamp
- Log Level
- Source File and Line Number
- Function Name
- Message

Example:
```
2024-12-04 15:30:45 [INFO] (server.c:145 - handle_client) New client connection from 192.168.1.100:54321
```

### Performance Logging

The system includes performance tracking macros:
```c
LOG_PERF_START(operation_name);
// ... operation code ...
LOG_PERF_END(operation_name);  // Automatically logs duration
```

## Building for Local Development

1. Create build directories:
```bash
make dirs
```

2. Build the project:
```bash
# Debug build with tests
make debug

# Release build
make release
```

3. Run tests:
```bash
make test
```

## Project Structure

```plaintext
tradesynth/
├── include/          # Header files organized by component
├── src/             # Source files
├── tests/           # Unit and integration tests
├── scripts/         # Utility and deployment scripts
├── deployment/      # Deployment configurations
└── docs/           # Documentation
