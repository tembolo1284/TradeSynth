#include "common/utils.h"
#include "common/logger.h"
#include <errno.h>
#include <ctype.h>
#include <math.h>

// Time utilities
time_t get_current_timestamp(void) {
    return time(NULL);
}

char* format_timestamp(time_t timestamp, char* buffer, size_t buffer_size) {
    struct tm* tm_info = localtime(&timestamp);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tm_info);
    return buffer;
}

int64_t get_time_diff_ms(struct timespec* start, struct timespec* end) {
    return (end->tv_sec - start->tv_sec) * 1000 + 
           (end->tv_nsec - start->tv_nsec) / 1000000;
}

// String utilities
char* trim_whitespace(char* str) {
    if (!str) return NULL;
    
    // Trim leading space
    while(isspace((unsigned char)*str)) str++;
    
    if(*str == 0) return str;
    
    // Trim trailing space
    char* end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    
    return str;
}

int string_to_upper(char* str) {
    if (!str) return ERROR_INVALID_PARAM;
    
    for(int i = 0; str[i]; i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
    return SUCCESS;
}

int string_to_lower(char* str) {
    if (!str) return ERROR_INVALID_PARAM;
    
    for(int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
    return SUCCESS;
}

char* safe_strncpy(char* dest, const char* src, size_t size) {
    if (!dest || !src || size == 0) return NULL;
    
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
    return dest;
}

// Number utilities
int64_t safe_atoi64(const char* str) {
    if (!str) return 0;
    char* endptr;
    errno = 0;
    int64_t val = strtoll(str, &endptr, 10);
    return (errno == 0 && *endptr == '\0') ? val : 0;
}

double safe_atod(const char* str) {
    if (!str) return 0.0;
    char* endptr;
    errno = 0;
    double val = strtod(str, &endptr);
    return (errno == 0 && *endptr == '\0') ? val : 0.0;
}

int is_valid_price(double price) {
    return price > 0.0 && price < 1000000.0 && !isnan(price) && !isinf(price);
}

int is_valid_quantity(int quantity) {
    return quantity > 0 && quantity <= 1000000;
}

// Memory utilities
void* safe_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        LOG_ERROR("Memory allocation failed for size %zu", size);
        return NULL;
    }
    return ptr;
}

void* safe_calloc(size_t nmemb, size_t size) {
    void* ptr = calloc(nmemb, size);
    if (!ptr) {
        LOG_ERROR("Memory allocation failed for %zu elements of size %zu", nmemb, size);
        return NULL;
    }
    return ptr;
}

void safe_free(void** ptr) {
    if (ptr && *ptr) {
        free(*ptr);
        *ptr = NULL;
    }
}

// Order validation functions
int validate_order_fields(const Order* order) {
    if (!order) return ERROR_INVALID_PARAM;
    
    if (strlen(order->symbol) == 0 || strlen(order->symbol) >= MAX_SYMBOL_LENGTH) {
        LOG_ERROR("Invalid symbol length for order ID=%lu", order->order_id);
        return ERROR_INVALID_ORDER;
    }
    
    if (order->quantity <= 0) {
        LOG_ERROR("Invalid quantity %u for order ID=%lu", order->quantity, order->order_id);
        return ERROR_INVALID_ORDER;
    }
    
    if (!is_valid_order_type(order->type)) {
        LOG_ERROR("Invalid order type %d for order ID=%lu", order->type, order->order_id);
        return ERROR_INVALID_ORDER;
    }
    
    if (!is_valid_order_side(order->side)) {
        LOG_ERROR("Invalid order side %d for order ID=%lu", order->side, order->order_id);
        return ERROR_INVALID_ORDER;
    }
    
    return SUCCESS;
}

int is_valid_order_type(OrderType type) {
    return type >= ORDER_TYPE_MARKET && type <= ORDER_TYPE_STOP_LIMIT;
}

int is_valid_order_side(OrderSide side) {
    return side == ORDER_SIDE_BUY || side == ORDER_SIDE_SELL;
}

int is_valid_time_in_force(TimeInForce tif) {
    return tif >= TIF_DAY && tif <= TIF_GTC;
}

char* order_type_to_string(OrderType type) {
    switch(type) {
        case ORDER_TYPE_MARKET: return "MARKET";
        case ORDER_TYPE_LIMIT: return "LIMIT";
        case ORDER_TYPE_STOP: return "STOP";
        case ORDER_TYPE_STOP_LIMIT: return "STOP_LIMIT";
        default: return "UNKNOWN";
    }
}

char* order_side_to_string(OrderSide side) {
    switch(side) {
        case ORDER_SIDE_BUY: return "BUY";
        case ORDER_SIDE_SELL: return "SELL";
        default: return "UNKNOWN";
    }
}

char* order_status_to_string(OrderStatus status) {
    switch(status) {
        case ORDER_STATUS_NEW: return "NEW";
        case ORDER_STATUS_PARTIAL: return "PARTIAL";
        case ORDER_STATUS_FILLED: return "FILLED";
        case ORDER_STATUS_CANCELLED: return "CANCELLED";
        case ORDER_STATUS_REJECTED: return "REJECTED";
        default: return "UNKNOWN";
    }
}

// Market data validation
int validate_market_data(const MarketData* data) {
    if (!data) return ERROR_INVALID_PARAM;
    
    if (strlen(data->symbol) == 0 || strlen(data->symbol) >= MAX_SYMBOL_LENGTH) {
        LOG_ERROR("Invalid symbol in market data");
        return ERROR_MARKET_DATA;
    }
    
    if (price_to_double(data->bid) >= price_to_double(data->ask)) {
        LOG_ERROR("Invalid bid/ask spread for %s", data->symbol);
        return ERROR_MARKET_DATA;
    }
    
    return SUCCESS;
}

// Random number generation
uint64_t generate_order_id(void) {
    static uint64_t next_order_id = 1;
    return __sync_fetch_and_add(&next_order_id, 1);
}

uint64_t generate_trade_id(void) {
    static uint64_t next_trade_id = 1;
    return __sync_fetch_and_add(&next_trade_id, 1);
}

double generate_random_price(double min, double max) {
    return min + (((double)rand() / RAND_MAX) * (max - min));
}

int generate_random_quantity(int min, int max) {
    return min + (rand() % (max - min + 1));
}

// Hash functions
uint64_t hash_string(const char* str) {
    uint64_t hash = 5381;
    int c;
    
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    
    return hash;
}

uint64_t hash_order(const Order* order) {
    if (!order) return 0;
    
    uint64_t hash = hash_string(order->symbol);
    hash = (hash << 5) + hash + order->order_id;
    hash = (hash << 5) + hash + (uint64_t)order->type;
    hash = (hash << 5) + hash + (uint64_t)order->side;
    hash = (hash << 5) + hash + (uint64_t)order->quantity;
    
    return hash;
}

// Price utilities
int compare_prices(const Price* p1, const Price* p2) {
    if (!p1 || !p2) return 0;
    
    double price1 = price_to_double(*p1);
    double price2 = price_to_double(*p2);
    
    if (price1 < price2) return -1;
    if (price1 > price2) return 1;
    return 0;
}

void normalize_price(Price* price) {
    if (!price) return;
    
    while (price->mantissa % 10 == 0 && price->exponent < 0) {
        price->mantissa /= 10;
        price->exponent++;
    }
}

char* price_to_string(const Price* price, char* buffer, size_t buffer_size) {
    if (!price || !buffer || buffer_size == 0) return NULL;
    
    double value = price_to_double(*price);
    snprintf(buffer, buffer_size, "%.6f", value);
    return buffer;
}
