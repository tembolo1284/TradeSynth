#include "common/logger.h"

static FILE* log_file = NULL;
static LogLevel minimum_level = LOG_INFO;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

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

void init_logger(const char* log_file_path, LogLevel min_level) {
    minimum_level = min_level;
    
    if (log_file_path) {
        log_file = fopen(log_file_path, "a");
        if (!log_file) {
            fprintf(stderr, "Failed to open log file: %s\n", log_file_path);
            return;
        }
    }
    
    LOG_INFO("Logger initialized with minimum level: %s", level_strings[min_level]);
}

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
    if (log_file) {
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

void close_logger(void) {
    if (log_file) {
        LOG_INFO("Closing logger");
        fclose(log_file);
        log_file = NULL;
    }
}
