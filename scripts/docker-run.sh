#!/bin/bash

# Default values
LOG_LEVEL=${2:-2}  # Default to INFO level if not specified
LOG_DIR="/var/log/tradesynth"
CONTAINER_NAME="tradesynth"
IMAGE_NAME="tradesynth"

# Help function
show_help() {
    echo "Usage: ./docker-run.sh <command> [log_level]"
    echo "Commands:"
    echo "  build       - Build Docker image"
    echo "  run         - Run the server container"
    echo "  run-client  - Run a client container"
    echo "  stop        - Stop all containers"
    echo "  clean       - Remove containers and images"
    echo "  logs        - Show container logs"
    echo "  status      - Show container status"
    echo "  test        - Run tests in container"
    echo ""
    echo "Log Levels:"
    echo "  0 - TRACE"
    echo "  1 - DEBUG"
    echo "  2 - INFO (default)"
    echo "  3 - WARN"
    echo "  4 - ERROR"
    echo "  5 - FATAL"
}

# Create log directory if it doesn't exist
create_log_dir() {
    if [ ! -d "$LOG_DIR" ]; then
        echo "Creating log directory: $LOG_DIR"
        sudo mkdir -p "$LOG_DIR"
        sudo chmod 777 "$LOG_DIR"
    fi
}

# Function to check if Docker is running
check_docker() {
    if ! docker info >/dev/null 2>&1; then
        echo "Error: Docker is not running or you don't have permission"
        exit 1
    fi
}

# Main logic
case "$1" in
    "build")
        check_docker
        echo "Building Docker image..."
        docker build -t $IMAGE_NAME:latest .
        ;;
        
    "run")
        check_docker
        create_log_dir
        echo "Starting server container with log level $LOG_LEVEL..."
        docker run -d \
            --name $CONTAINER_NAME-server \
            -p 8080:8080 \
            -e TRADESYNTH_LOG_LEVEL=$LOG_LEVEL \
            -v $LOG_DIR:/var/log/tradesynth \
            $IMAGE_NAME:latest
        ;;
        
    "run-client")
        check_docker
        create_log_dir
        echo "Starting client container with log level $LOG_LEVEL..."
        docker run -d \
            --name $CONTAINER_NAME-client \
            --network host \
            -e TRADESYNTH_LOG_LEVEL=$LOG_LEVEL \
            -v $LOG_DIR:/var/log/tradesynth \
            $IMAGE_NAME:latest ./trading_client
        ;;
        
    "stop")
        check_docker
        echo "Stopping containers..."
        docker stop $CONTAINER_NAME-server $CONTAINER_NAME-client 2>/dev/null
        docker rm $CONTAINER_NAME-server $CONTAINER_NAME-client 2>/dev/null
        ;;
        
    "clean")
        check_docker
        echo "Cleaning up containers and images..."
        docker stop $CONTAINER_NAME-server $CONTAINER_NAME-client 2>/dev/null
        docker rm $CONTAINER_NAME-server $CONTAINER_NAME-client 2>/dev/null
        docker rmi $IMAGE_NAME:latest 2>/dev/null
        ;;
        
    "logs")
        check_docker
        echo "Showing logs..."
        docker logs $CONTAINER_NAME-server
        echo "Client logs:"
        docker logs $CONTAINER_NAME-client
        ;;
        
    "status")
        check_docker
        echo "Container status:"
        docker ps -a | grep $CONTAINER_NAME
        ;;
        
    "test")
        check_docker
        echo "Running tests..."
        docker run --rm \
            -e TRADESYNTH_LOG_LEVEL=$LOG_LEVEL \
            $IMAGE_NAME:latest make test
        ;;
        
    "help"|"--help"|"-h"|"")
        show_help
        ;;
        
    *)
        echo "Unknown command: $1"
        show_help
        exit 1
        ;;
esac
