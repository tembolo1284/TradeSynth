#ifndef TRADESYNTH_UTILS_H
#define TRADESYNTH_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include "types.h"
#include <math.h>

// Time utilities
time_t get_current_timestamp(void);
char* format_timestamp(time_t timestamp, char* buffer, size_t buffer_size);
int64_t get_time_diff_ms(struct timespec* start, struct timespec* end);

// String utilities
char* trim_whitespace(char* str);
int string_to_upper(char* str);
int string_to_lower(char* str);
char* safe_strncpy(char* dest, const char* src, size_t size);

// Number utilities
int64_t safe_atoi64(const char* str);
double safe_atod(const char* str);
int is_valid_price(double price);
int is_valid_quantity(int quantity);

// Memory utilities
void* safe_malloc(size_t size);
void* safe_calloc(size_t nmemb, size_t size);
void safe_free(void** ptr);

// Order utilities
int validate_order_fields(const Order* order);
int is_valid_order_type(OrderType type);
int is_valid_order_side(OrderSide side);
int is_valid_time_in_force(TimeInForce tif);
char* order_type_to_string(OrderType type);
char* order_side_to_string(OrderSide side);
char* order_status_to_string(OrderStatus status);

// Market data utilities
int validate_market_data(const MarketData* data);
int calculate_spread(const MarketData* data, double* spread);
double calculate_vwap(const MarketData* data, const TradeExecution* trades, size_t trade_count);

// Price utilities
int compare_prices(const Price* p1, const Price* p2);
void normalize_price(Price* price);
char* price_to_string(const Price* price, char* buffer, size_t buffer_size);
int parse_price_string(const char* str, Price* price);

// Error handling
const char* error_to_string(ErrorCode code);
void log_error(const char* file, int line, const char* func, ErrorCode code, const char* message);

// Random number generation
double generate_random_price(double min, double max);
int generate_random_quantity(int min, int max);

// Hash functions
uint64_t hash_string(const char* str);
uint64_t hash_order(const Order* order);

#endif // TRADESYNTH_UTILS_H
