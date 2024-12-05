#ifndef TRADESYNTH_TYPES_H
#define TRADESYNTH_TYPES_H

#include <time.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <math.h>
#include <pthread.h>

// Constants
#define MAX_SYMBOL_LENGTH 16
#define MAX_CLIENT_ID_LENGTH 32
#define MAX_ERROR_MSG_LENGTH 256
#define BUFFER_SIZE 4096
#define MAX_CLIENTS 100

// Error codes
typedef enum {
    SUCCESS = 0,
    ERROR_SOCKET_CREATE = -1,
    ERROR_SOCKET_BIND = -2,
    ERROR_SOCKET_LISTEN = -3,
    ERROR_SOCKET_ACCEPT = -4,
    ERROR_SOCKET_CONNECT = -5,
    ERROR_THREAD_CREATE = -6,
    ERROR_MEMORY_ALLOC = -7,
    ERROR_INVALID_PARAM = -8,
    ERROR_INVALID_STATE = -9,
    ERROR_TIMEOUT = -10,
    ERROR_SERIALIZATION = -11,
    ERROR_DESERIALIZATION = -12,
    ERROR_INVALID_MESSAGE = -13,
    ERROR_INVALID_ORDER = -14,
    ERROR_ORDER_NOT_FOUND = -15,
    ERROR_MARKET_DATA = -16
} ErrorCode;

// Message types
typedef enum {
    MSG_NONE = 0,
    MSG_HEARTBEAT = 1,
    MSG_ORDER_NEW = 2,
    MSG_ORDER_CANCEL = 3,
    MSG_ORDER_MODIFY = 4,
    MSG_ORDER_STATUS = 5,
    MSG_MARKET_DATA = 6,
    MSG_TRADE_EXEC = 7,
    MSG_ERROR = 8
} MessageType;

// Order types
typedef enum {
    ORDER_TYPE_MARKET = 1,
    ORDER_TYPE_LIMIT = 2,
    ORDER_TYPE_STOP = 3,
    ORDER_TYPE_STOP_LIMIT = 4
} OrderType;

// Order side
typedef enum {
    ORDER_SIDE_BUY = 1,
    ORDER_SIDE_SELL = 2
} OrderSide;

// Order status
typedef enum {
    ORDER_STATUS_NEW = 1,
    ORDER_STATUS_PARTIAL = 2,
    ORDER_STATUS_FILLED = 3,
    ORDER_STATUS_CANCELLED = 4,
    ORDER_STATUS_REJECTED = 5
} OrderStatus;

// Time in force
typedef enum {
    TIF_DAY = 1,
    TIF_IOC = 2,  // Immediate or Cancel
    TIF_FOK = 3,  // Fill or Kill
    TIF_GTC = 4   // Good Till Cancel
} TimeInForce;

// Price structure
typedef struct {
    int64_t mantissa;
    int32_t exponent;
} Price;

// Order structure
typedef struct {
    uint64_t order_id;
    char symbol[MAX_SYMBOL_LENGTH];
    char client_id[MAX_CLIENT_ID_LENGTH];
    OrderType type;
    OrderSide side;
    OrderStatus status;
    TimeInForce time_in_force;
    Price price;
    uint32_t quantity;
    uint32_t filled_quantity;
    uint32_t remaining_quantity;
    time_t creation_time;
    time_t modification_time;
    time_t expiration_time;
} Order;

// Market Data structure
typedef struct {
    char symbol[MAX_SYMBOL_LENGTH];
    Price last_price;
    Price bid;
    Price ask;
    uint32_t last_size;
    uint32_t bid_size;
    uint32_t ask_size;
    uint64_t volume;
    uint32_t num_trades;
    time_t timestamp;
} MarketData;

// Trade execution structure
typedef struct {
    uint64_t trade_id;
    uint64_t order_id;
    char symbol[MAX_SYMBOL_LENGTH];
    Price price;
    uint32_t quantity;
    time_t timestamp;
    char buyer_id[MAX_CLIENT_ID_LENGTH];
    char seller_id[MAX_CLIENT_ID_LENGTH];
} TradeExecution;

// Message structure
typedef struct {
    MessageType type;
    uint64_t sequence_num;
    time_t timestamp;
    union {
        Order order;
        MarketData market_data;
        TradeExecution trade;
        struct {
            ErrorCode code;
            char message[MAX_ERROR_MSG_LENGTH];
        } error;
    } data;
} Message;

// Forward declaration
struct ServerContext;

// Price manipulation functions
static inline Price create_price(int64_t mantissa, int32_t exponent) {
    Price p = {mantissa, exponent};
    return p;
}

static inline double price_to_double(Price p) {
    return (double)p.mantissa * pow(10, p.exponent);
}

static inline Price double_to_price(double value) {
    Price p;
    p.exponent = -6;  // 6 decimal places precision
    p.mantissa = (int64_t)(value * pow(10, -p.exponent));
    return p;
}

#endif // TRADESYNTH_TYPES_H
