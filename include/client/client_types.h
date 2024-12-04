#ifndef TRADESYNTH_CLIENT_TYPES_H
#define TRADESYNTH_CLIENT_TYPES_H

#include "common/types.h"

// Client configuration defaults
#define DEFAULT_PORT 8080
#define DEFAULT_MAX_CLIENTS 100
#define DEFAULT_SOCKET_TIMEOUT 30
#define DEFAULT_RECONNECT_ATTEMPTS 3
#define RECONNECT_DELAY_MS 1000
#define HEARTBEAT_INTERVAL_MS 5000
#define RESPONSE_TIMEOUT_MS 5000

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

// Client configuration
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

// Client context
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

#endif // TRADESYNTH_CLIENT_TYPES_H
