#include "server/server.h"
#include <getopt.h>

static void print_usage(const char* program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -p, --port PORT       Server port (default: %d)\n", DEFAULT_PORT);
    printf("  -c, --clients MAX     Maximum clients (default: %d)\n", DEFAULT_MAX_CLIENTS);
    printf("  -t, --timeout SECS    Socket timeout (default: %d)\n", DEFAULT_SOCKET_TIMEOUT);
    printf("  -l, --log-level LVL   Log level (0-5, default: 2)\n");
    printf("  -f, --log-file FILE   Log file path\n");
    printf("  -h, --help            Show this help message\n");
}

int main(int argc, char *argv[]) {
    ServerConfig config = {
        .port = DEFAULT_PORT,
        .max_clients = DEFAULT_MAX_CLIENTS,
        .socket_timeout = DEFAULT_SOCKET_TIMEOUT,
        .log_level = LOG_INFO
    };
    strncpy(config.bind_address, "0.0.0.0", sizeof(config.bind_address));
    strncpy(config.log_file, "./server.log", sizeof(config.log_file));

    // Parse command line options
    static struct option long_options[] = {
        {"port",      required_argument, 0, 'p'},
        {"clients",   required_argument, 0, 'c'},
        {"timeout",   required_argument, 0, 't'},
        {"log-level", required_argument, 0, 'l'},
        {"log-file",  required_argument, 0, 'f'},
        {"help",      no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "p:c:t:l:f:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                config.port = atoi(optarg);
                break;
            case 'c':
                config.max_clients = atoi(optarg);
                break;
            case 't':
                config.socket_timeout = atoi(optarg);
                break;
            case 'l':
                config.log_level = atoi(optarg);
                break;
            case 'f':
                strncpy(config.log_file, optarg, sizeof(config.log_file) - 1);
                break;
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Initialize logging
    init_logger(config.log_file, config.log_level);
    LOG_INFO("TradeSynth Server Starting...");

    // Initialize server
    ServerContext* context = initialize_server_context(&config);
    if (!context) {
        LOG_ERROR("Failed to initialize server context");
        return EXIT_FAILURE;
    }

    // Start server
    int result = start_server(context);
    
    // Cleanup
    cleanup_server(context);
    close_logger();
    
    return result == SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}
