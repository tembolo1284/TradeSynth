#include "client/client.h"
#include <getopt.h>

static void print_usage(const char* program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -h, --host HOST       Server host (default: localhost)\n");
    printf("  -p, --port PORT       Server port (default: %d)\n", DEFAULT_PORT);
    printf("  -t, --timeout SECS    Socket timeout (default: %d)\n", DEFAULT_SOCKET_TIMEOUT);
    printf("  -l, --log-level LVL   Log level (0-5, default: 2)\n");
    printf("  -f, --log-file FILE   Log file path\n");
    printf("  --help                Show this help message\n");
}

// Callback functions
static void on_connect(void* user_data) {
    (void)user_data;
    LOG_INFO("Connected to server");
}

static void on_disconnect(void* user_data) {
    (void)user_data;
    LOG_INFO("Disconnected from server");
}

static void on_market_data(const MarketData* data, void* user_data) {
    (void)user_data;
    LOG_INFO("Market data for %s: Bid=%.2f, Ask=%.2f",
             data->symbol,
             price_to_double(data->bid),
             price_to_double(data->ask));
}

static void on_order_status(const Order* order, void* user_data) {
    (void)user_data;
    LOG_INFO("Order status update: ID=%lu, Status=%d",
             order->order_id, order->status);
}

static void on_trade(const TradeExecution* trade, void* user_data) {
    (void)user_data;
    LOG_INFO("Trade executed: ID=%lu, Price=%.2f, Quantity=%d",
             trade->trade_id,
             price_to_double(trade->price),
             trade->quantity);
}

int main(int argc, char *argv[]) {
    // Initialize default client configuration
    ClientConfig config = {
        .server_port = DEFAULT_PORT,
        .socket_timeout = DEFAULT_SOCKET_TIMEOUT,
        .reconnect_attempts = DEFAULT_RECONNECT_ATTEMPTS,
        .log_level = LOG_INFO
    };
    strncpy(config.server_host, "localhost", sizeof(config.server_host));
    strncpy(config.log_file, "./client.log", sizeof(config.log_file));

    // Parse command-line options
    static struct option long_options[] = {
        {"host",      required_argument, 0, 'h'},
        {"port",      required_argument, 0, 'p'},
        {"timeout",   required_argument, 0, 't'},
        {"log-level", required_argument, 0, 'l'},
        {"log-file",  required_argument, 0, 'f'},
        {"help",      no_argument,       0, '?'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "h:p:t:l:f:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                strncpy(config.server_host, optarg, sizeof(config.server_host) - 1);
                break;
            case 'p':
                config.server_port = atoi(optarg);
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
            case '?':
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Initialize logger
    if (init_logger(config.log_file, config.log_level) != SUCCESS) {
        fprintf(stderr, "Failed to initialize logger\n");
        return EXIT_FAILURE;
    }
    LOG_INFO("TradeSynth Client Starting...");

    // Set up callbacks
    ClientCallbacks callbacks = {
        .on_connect = on_connect,
        .on_disconnect = on_disconnect,
        .on_market_data = on_market_data,
        .on_order_status = on_order_status,
        .on_trade = on_trade
    };

    // Initialize client context
    ClientContext* context = initialize_client(&config, &callbacks, NULL);
    if (!context) {
        LOG_ERROR("Failed to initialize client context");
        close_logger();
        return EXIT_FAILURE;
    }

    // Attempt to connect to the server
    int result = connect_to_server(context);
    if (result != SUCCESS) {
        LOG_ERROR("Failed to connect to server");
        cleanup_client(context);
        close_logger();
        return EXIT_FAILURE;
    }

    // Main user interaction loop
    char symbol[MAX_SYMBOL_LENGTH];
    printf("Enter symbol to subscribe (or 'quit' to exit): ");
    while (fgets(symbol, sizeof(symbol), stdin)) {
        symbol[strcspn(symbol, "\n")] = 0;  // Remove trailing newline

        if (strcmp(symbol, "quit") == 0) {
            LOG_INFO("User requested to exit");
            break;
        }

        if (strlen(symbol) == 0) {
            LOG_WARN("Symbol cannot be empty");
            printf("Enter a valid symbol (or 'quit' to exit): ");
            continue;
        }

        // Request market data
        result = request_market_data(context, symbol);
        if (result != SUCCESS) {
            LOG_ERROR("Failed to request market data for symbol: %s", symbol);
        }

        printf("Enter symbol to subscribe (or 'quit' to exit): ");
    }

    // Cleanup and exit
    cleanup_client(context);
    close_logger();
    LOG_INFO("Client terminated successfully");
    return EXIT_SUCCESS;
}

