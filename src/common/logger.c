#include "common/logger.h"
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

// Global state
static FILE* log_file = NULL;
static LogLevel minimum_level = LOG_INFO;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Log level strings and colors
static const char* level_strings[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static const char* level_colors[] = {
    ANSI_COLOR_BLUE,    // TRACE
    ANSI_COLOR_CYAN,    // DEBUG
    ANSI_COLOR_GREEN,   // INFO
    ANSI_COLOR_YELLOW,  // WARN
    ANSI_COLOR_RED,     // ERROR
    ANSI_COLOR_MAGENTA  // FATAL
};

/**
 * Initialize the logger.
 * 
 * @param log_file_path Path to the log file. If NULL, logs are written to stderr.
 * @param min_level Minimum log level to capture.
 * @return int SUCCESS (0) on success, error code otherwise.
 */
int init_logger(const char* log_file_path, LogLevel min_level) {
    // Validate log level
    if (min_level < LOG_TRACE || min_level > LOG_FATAL) {
        fprintf(stderr, "Invalid log level: %d\n", min_level);
        return ERROR_INVALID_PARAM;
    }

    minimum_level = min_level;

    // Open log file if a path is provided
    if (log_file_path) {
        log_file = fopen(log_file_path, "a");
        if (!log_file) {
            fprintf(stderr, "Failed to open log file: %s\n", log_file_path);
            return ERROR_FILE_OPEN;
        }
    } else {
        // Use stderr if no file path is provided
        log_file = stderr;
    }

    // Log initialization success
    LOG_INFO("Logger initialized with minimum level: %s", level_strings[min_level]);
    return SUCCESS;
}

/**
 * Log a message with a specified log level.
 * 
 * @param level Log level of the message.
 * @param file Source file where the log is called.
 * @param line Line number in the source file.
 * @param func Function name where the log is called.
 * @param format Format string for the log message.
 */
void log_message(LogLevel level, const char* file, int line, const char* func, const char* format, ...) {
    if (level < minimum_level) return;

    time_t now;
    time(&now);
    struct tm* local_time = localtime(&now);

    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local_time);

    pthread_mutex_lock(&log_mutex);

    // Write to stdout with colors
    printf("%s%s [%-5s] (%s:%d - %s) ",
           level_colors[level],
           timestamp,
           level_strings[level],
           file,
           line,
           func);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf(ANSI_COLOR_RESET "\n");

    // Write to log file if opened
    if (log_file && log_file != stderr) {
        fprintf(log_file, "%s [%-5s] (%s:%d - %s) ",
                timestamp,
                level_strings[level],
                file,
                line,
                func);

        va_list file_args;
        va_start(file_args, format);
        vfprintf(log_file, format, file_args);
        va_end(file_args);

        fprintf(log_file, "\n");
        fflush(log_file);
    }

    pthread_mutex_unlock(&log_mutex);

    // Exit on fatal errors
    if (level == LOG_FATAL) {
        exit(EXIT_FAILURE);
    }
}

/**
 * Close the logger and release resources.
 */
void close_logger(void) {
    if (log_file && log_file != stderr) {
        LOG_INFO("Closing logger");
        fclose(log_file);
        log_file = NULL;
    }
}

