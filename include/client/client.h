#ifndef TRADESYNTH_CLIENT_H
#define TRADESYNTH_CLIENT_H

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

// Client configuration
#define DEFAULT_RECONNECT_ATTEMPTS 3
#define RECONNECT_DELAY_MS 1000
#define HEARTBEAT_INTERVAL_MS 5000
#define RESPONSE_TIMEOUT_MS 5000

// Client states
typedef enum {
    CLIENT_DISCONNECTED = 0,
    CLIENT_CONNECTING,
    CLIENT_CONNECTED,
    CLIENT_AUTHENTICATING,
    CLIENT_READY,
    CLIENT_ERROR
} ClientState;

// Client statistics
typedef struct {
    uint64_t messages_sent;
    uint64_t messages_received;
    uint64_t orders_sent;
    uint64_t trades_received;
    uint64_t errors_encountered;
    time_t connect_time;
    time_t last_heartbeat;
} ClientStats;

// Client configuration structure
typedef struct {
    char server_host[256];
    int server_port;
    int reconnect_attempts;
    int socket_timeout;
    LogLevel log_level;
    char log_file[256];
    char client_id[MAX_CLIENT_ID_LENGTH];
} ClientConfig;

// Client callback functions
typedef struct {
    void (*on_connect)(void* user_data);
    void (*on_disconnect)(void* user_data);
    void (*on_market_data)(const MarketData* data, void* user_data);
    void (*on_order_status)(const Order* order, void* user_data);
    void (*on_trade)(const TradeExecution* trade, void* user_data);
    void (*on_error)(ErrorCode error, const char* message, void* user_data);
} ClientCallbacks;

// Client context structure
typedef struct {
    int socket;
    ClientState state;
    ClientStats stats;
    ClientConfig config;
    ClientCallbacks callbacks;
    void* user_data;
    pthread_t receiver_thread;
    pthread_mutex_t state_mutex;
    pthread_mutex_t stats_mutex;
    volatile sig_atomic_t running;
} ClientContext;

// Function declarations

// Client lifecycle management
ClientContext* initialize_client(const ClientConfig* config, const ClientCallbacks* callbacks, void* user_data);
int connect_to_server(ClientContext* context);
void disconnect_from_server(ClientContext* context);
void cleanup_client(ClientContext* context);

// Message sending
int send_order(ClientContext* context, const Order* order);
int cancel_order(ClientContext* context, uint64_t order_id);
int modify_order(ClientContext* context, const Order* order);
int request_market_data(ClientContext* context, const char* symbol);

// Message receiving
void* message_receiver_thread(void* arg);
int process_received_message(ClientContext* context, const Message* msg);

// Utility functions
ClientState get_client_state(const ClientContext* context);
ClientStats get_client_stats(const ClientContext* context);
const char* client_state_to_string(ClientState state);
int validate_client_config(const ClientConfig* config);
int validate_callbacks(const ClientCallbacks* callbacks);

// Error handling
const char* get_client_error(int error_code);
void update_client_state(ClientContext* context, ClientState new_state);
void update_client_stats(ClientContext* context, const char* metric);

// Heartbeat management
int send_heartbeat(ClientContext* context);
int handle_heartbeat_response(ClientContext* context);

#endif // TRADESYNTH_CLIENT_H
