#ifndef TRADESYNTH_LOGGER_H
#define TRADESYNTH_LOGGER_H

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>

typedef enum {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
} LogLevel;

// Errors
#define SUCCESS 0
#define ERROR_INVALID_PARAM -1
#define ERROR_FILE_OPEN -2

// Color codes for different log levels
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// Function declarations
int init_logger(const char* log_file_path, LogLevel min_level);
void log_message(LogLevel level, const char* file, int line, const char* func, const char* format, ...);
void close_logger(void);

// Convenience macros
#define LOG_TRACE(...) log_message(LOG_TRACE, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_DEBUG(...) log_message(LOG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_INFO(...)  log_message(LOG_INFO,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARN(...)  log_message(LOG_WARN,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(...) log_message(LOG_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_FATAL(...) log_message(LOG_FATAL, __FILE__, __LINE__, __func__, __VA_ARGS__)

// Performance logging macros
#define LOG_PERF_START(name) \
    clock_t _start_##name = clock();

#define LOG_PERF_END(name) \
    clock_t _end_##name = clock(); \
    double _duration_##name = ((double)(_end_##name - _start_##name)) / CLOCKS_PER_SEC * 1000; \
    LOG_DEBUG("Performance [%s]: %.2f ms", #name, _duration_##name);

#endif // TRADESYNTH_LOGGER_H
