#ifndef TRADESYNTH_SERVER_H
#define TRADESYNTH_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common/types.h"
#include "common/logger.h"
#include "serialization/serialization.h"

// Server configuration
#define DEFAULT_PORT 8080
#define DEFAULT_MAX_CLIENTS 100
#define DEFAULT_SOCKET_TIMEOUT 30
#define MAX_PENDING_CONNECTIONS 10

// Additional error codes
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

// Server configuration structure
typedef struct {
    int port;
    int max_clients;
    int socket_timeout;
    char bind_address[INET_ADDRSTRLEN];
    LogLevel log_level;
    char log_file[256];
} ServerConfig;

// Server context structure
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

// Function declarations

// Server lifecycle management
ServerContext* initialize_server_context(const ServerConfig* config);
int start_server(ServerContext* context);
void stop_server(ServerContext* context);
void cleanup_server(ServerContext* context);

// Client handling
void* handle_client(void* client_data);
int accept_client(ServerContext* context);
void disconnect_client(ServerContext* context, int client_index);

// Message processing
int process_client_message(ServerContext* context, ClientConnection* client, const Message* msg);
int broadcast_message(ServerContext* context, const Message* msg, int exclude_client);

// Order management
int process_order(ServerContext* context, const Order* order);
int process_order_cancel(ServerContext* context, long order_id);
int process_order_modify(ServerContext* context, const Order* order);

// Market data handling
int process_market_data(ServerContext* context, const MarketData* market_data);
int broadcast_market_data(ServerContext* context, const MarketData* market_data);

// Trade execution
int process_trade_execution(ServerContext* context, const TradeExecution* trade);
int notify_trade_parties(ServerContext* context, const TradeExecution* trade);

// Statistics and monitoring
void update_server_stats(ServerContext* context, const char* metric);
ServerStats get_server_stats(const ServerContext* context);
const char* server_state_to_string(ServerState state);

// Error handling
const char* get_server_error(int error_code);
void log_server_error(ServerContext* context, const char* message);

// Socket and networking
int setup_socket(ServerContext* context);
void cleanup_socket(int socket_fd);

// Utility functions
int validate_server_config(const ServerConfig* config);

#endif // TRADESYNTH_SERVER_H
