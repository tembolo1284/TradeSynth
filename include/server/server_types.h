#ifndef TRADESYNTH_SERVER_TYPES_H
#define TRADESYNTH_SERVER_TYPES_H

#include "common/types.h"
#include "common/logger.h"

// Server configuration defaults
#define DEFAULT_PORT 8080
#define DEFAULT_MAX_CLIENTS 100
#define DEFAULT_SOCKET_TIMEOUT 30
#define MAX_PENDING_CONNECTIONS 10
#define ERROR_MAX_CLIENTS -100
#define ERROR_CONFIG_INVALID -101
#define ERROR_SERVER_RUNNING -102

// Server states
typedef enum {
    SERVER_STOPPED = 0,
    SERVER_STARTING,
    SERVER_RUNNING,
    SERVER_STOPPING,
    SERVER_ERROR
} ServerState;

// Server statistics
typedef struct {
    size_t total_connections;
    size_t active_connections;
    size_t messages_processed;
    size_t errors_encountered;
    time_t start_time;
    time_t last_error_time;
    uint64_t bytes_received;
    uint64_t bytes_sent;
} ServerStats;

// Server configuration
typedef struct {
    int port;
    int max_clients;
    int socket_timeout;
    char bind_address[INET_ADDRSTRLEN];
    LogLevel log_level;
    char log_file[256];
} ServerConfig;

// Server context
typedef struct ServerContext {
    int server_socket;
    ServerState state;
    ServerStats stats;
    ServerConfig config;
    pthread_mutex_t stats_mutex;
    pthread_mutex_t clients_mutex;
    ClientConnection* clients;
    int client_count;
} ServerContext;

#endif // TRADESYNTH_SERVER_TYPES_H
