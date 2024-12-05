#ifndef TRADESYNTH_SERVER_TYPES_H
#define TRADESYNTH_SERVER_TYPES_H

#include <stdatomic.h>
#include <pthread.h>
#include <netinet/in.h>
#include "common/types.h"
#include "common/logger.h"

// Server configuration defaults
#define DEFAULT_PORT 8080
#define DEFAULT_MAX_CLIENTS 100
#define DEFAULT_SOCKET_TIMEOUT 30
#define MAX_PENDING_CONNECTIONS 10
#define MAX_SYMBOLS 1000
#define MAX_ORDERS_PER_SYMBOL 10000

// Server error codes
#define ERROR_MAX_CLIENTS -100
#define ERROR_CONFIG_INVALID -101
#define ERROR_SERVER_RUNNING -102
#define ERROR_ORDERBOOK_FULL -103
#define ERROR_SYMBOL_NOT_FOUND -104
#define ERROR_POSITION_LIMIT -105

// Forward declarations
typedef struct ServerContext ServerContext;

// Order book structures
typedef struct OrderBookEntry {
   Order order;
   struct OrderBookEntry* next;
   struct OrderBookEntry* prev;
   time_t entry_time;
} OrderBookEntry;

typedef struct OrderBook {
   char symbol[MAX_SYMBOL_LENGTH];
   OrderBookEntry* bids;
   OrderBookEntry* asks;
   uint32_t bid_count;
   uint32_t ask_count;
   Price best_bid;
   Price best_ask;
   uint64_t total_volume;
   pthread_rwlock_t lock;
} OrderBook;

// Client position tracking
typedef struct ClientPosition {
   char client_id[MAX_CLIENT_ID_LENGTH];
   char symbol[MAX_SYMBOL_LENGTH];
   int64_t position;
   Price average_price;
   Price realized_pnl;
   Price unrealized_pnl;
   uint64_t total_volume;
   uint32_t open_orders;
} ClientPosition;

// Client connection
typedef struct ClientConnection {
   // Socket and network info
   int socket;
   struct sockaddr_in address;
   char id[MAX_CLIENT_ID_LENGTH];
   
   // Connection status
   volatile int active;
   pthread_t thread;
   ServerContext* context;
   
   // Timing info
   time_t connect_time;
   time_t last_heartbeat;
   
   // Message counters
   atomic_uint_least64_t messages_sent;
   atomic_uint_least64_t messages_received;
   
   // Connection state
   pthread_mutex_t lock;
   
   // Client specific data
   ClientPosition* positions;
   uint32_t num_positions;
} ClientConnection;

// Server states
typedef enum {
   SERVER_STATE_INIT,
   SERVER_STARTING,
   SERVER_RUNNING,
   SERVER_STOPPING,
   SERVER_STOPPED,
   SERVER_ERROR
} ServerState;

// Server statistics
typedef struct {
   atomic_size_t total_connections;
   atomic_size_t active_connections;
   atomic_size_t messages_processed;
   atomic_size_t errors_encountered;
   atomic_uint_least64_t bytes_received;
   atomic_uint_least64_t bytes_sent;
   time_t start_time;
   time_t last_error_time;
} ServerStats;

// Server configuration
typedef struct {
   int port;
   int max_clients;
   int socket_timeout;
   char bind_address[INET_ADDRSTRLEN];
   LogLevel log_level;
   char log_file[256];
   uint32_t max_symbols;
   uint32_t max_orders_per_symbol;
   uint32_t position_limit;
   void* (*client_handler)(void*);
} ServerConfig;

// Server context
struct ServerContext {
   // Core server info
   int server_socket;
   ServerState state;
   atomic_uint_least64_t sequence_num;
   
   // Configuration
   ServerConfig config;
   
   // Client management
   ClientConnection* clients;
   atomic_int client_count;
   
   // Statistics
   ServerStats stats;
   
   // Thread safety
   pthread_mutex_t stats_mutex;
   pthread_mutex_t clients_mutex;
   pthread_t accept_thread;
   
   // Market data
   MarketData* market_data_cache;
   pthread_rwlock_t market_data_lock;
   
   // Order management 
   OrderBook* order_books;
   uint32_t symbol_count;
   pthread_rwlock_t order_book_lock;
   
   // Risk management
   ClientPosition* positions;
   pthread_rwlock_t position_lock;
};

#endif // TRADESYNTH_SERVER_TYPES_H
