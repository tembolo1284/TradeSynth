#include "client/client.h"

// Receiver thread for incoming messages
void* message_receiver_thread(void* arg) {
    ClientContext* context = (ClientContext*)arg;
    uint8_t buffer[BUFFER_SIZE];
    Message msg;

    while (context->running) {
        ssize_t bytes_received = recv(context->socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                LOG_INFO("Server disconnected");
            } else {
                LOG_ERROR("Error receiving from server: %s", strerror(errno));
            }
            break;
        }

        if (deserialize_message(buffer, bytes_received, &msg) != SUCCESS) {
            LOG_ERROR("Failed to deserialize message");
            continue;
        }

        context->stats.messages_received++;

        // Process received message based on type
        switch (msg.type) {
            case MSG_ORDER_STATUS:
                if (context->callbacks.on_order_status) {
                    context->callbacks.on_order_status(&msg.data.order, context->user_data);
                }
                break;

            case MSG_MARKET_DATA:
                if (context->callbacks.on_market_data) {
                    context->callbacks.on_market_data(&msg.data.market_data, context->user_data);
                }
                break;

            case MSG_TRADE_EXEC:
                if (context->callbacks.on_trade) {
                    context->callbacks.on_trade(&msg.data.trade, context->user_data);
                }
                context->stats.trades_received++;
                break;

            default:
                LOG_WARN("Received unknown message type: %d", msg.type);
        }
    }

    context->state = CLIENT_DISCONNECTED;
    if (context->callbacks.on_disconnect) {
        context->callbacks.on_disconnect(context->user_data);
    }

    return NULL;
}

// Initialize client context
ClientContext* initialize_client(const ClientConfig* config, const ClientCallbacks* callbacks, void* user_data) {
    if (!config) {
        LOG_ERROR("Invalid client configuration");
        return NULL;
    }

    ClientContext* context = (ClientContext*)calloc(1, sizeof(ClientContext));
    if (!context) {
        LOG_ERROR("Failed to allocate client context");
        return NULL;
    }

    context->config = *config;
    if (callbacks) {
        context->callbacks = *callbacks;
    }
    context->user_data = user_data;
    context->state = CLIENT_DISCONNECTED;
    context->running = 1;

    pthread_mutex_init(&context->state_mutex, NULL);
    pthread_mutex_init(&context->stats_mutex, NULL);

    LOG_INFO("Client context initialized successfully");
    return context;
}

// Connect to server
int connect_to_server(ClientContext* context) {
    if (!context) return ERROR_INVALID_PARAM;

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(context->config.server_port);

    // Handle hostname resolution
    if (strcmp(context->config.server_host, "localhost") == 0) {
        if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
            LOG_ERROR("Failed to resolve localhost");
            return ERROR_SOCKET_CONNECT;
        }
    } else {
        if (inet_pton(AF_INET, context->config.server_host, &server_addr.sin_addr) <= 0) {
            LOG_ERROR("Invalid server address: %s", context->config.server_host);
            return ERROR_SOCKET_CONNECT;
        }
    }

    // Create socket
    context->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (context->socket < 0) {
        LOG_ERROR("Failed to create socket: %s", strerror(errno));
        return ERROR_SOCKET_CREATE;
    }

    // Connect to server
    if (connect(context->socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR("Failed to connect to server: %s", strerror(errno));
        close(context->socket);
        return ERROR_SOCKET_CONNECT;
    }

    context->state = CLIENT_CONNECTED;
    context->stats.connect_time = time(NULL);

    // Start receiver thread
    if (pthread_create(&context->receiver_thread, NULL, message_receiver_thread, context) != 0) {
        LOG_ERROR("Failed to create receiver thread: %s", strerror(errno));
        close(context->socket);
        return ERROR_THREAD_CREATE;
    }

    if (context->callbacks.on_connect) {
        context->callbacks.on_connect(context->user_data);
    }

    LOG_INFO("Connected to server %s:%d", context->config.server_host, context->config.server_port);
    return SUCCESS;
}

// Send an order to the server
int send_order(ClientContext* context, const Order* order) {
    if (!context || !order) return ERROR_INVALID_PARAM;
    if (context->state != CLIENT_CONNECTED) return ERROR_INVALID_STATE;

    Message msg = {
        .type = MSG_ORDER_NEW,
        .data.order = *order,
        .timestamp = time(NULL)
    };

    uint8_t buffer[BUFFER_SIZE];
    size_t msg_size = serialize_message(&msg, buffer, BUFFER_SIZE);
    if (msg_size <= 0) {
        LOG_ERROR("Failed to serialize order");
        return ERROR_SERIALIZATION;
    }

    if (send(context->socket, buffer, msg_size, 0) < 0) {
        LOG_ERROR("Failed to send order: %s", strerror(errno));
        return ERROR_SOCKET_CONNECT;
    }

    context->stats.orders_sent++;
    context->stats.messages_sent++;

    LOG_INFO("Sent order: ID=%lu, Symbol=%s", order->order_id, order->symbol);
    return SUCCESS;
}

// Request market data
int request_market_data(ClientContext* context, const char* symbol) {
    if (!context || !symbol) return ERROR_INVALID_PARAM;
    if (context->state != CLIENT_CONNECTED) return ERROR_INVALID_STATE;

    Message msg = {
        .type = MSG_MARKET_DATA,
        .timestamp = time(NULL)
    };
    strncpy(msg.data.market_data.symbol, symbol, MAX_SYMBOL_LENGTH - 1);

    uint8_t buffer[BUFFER_SIZE];
    size_t msg_size = serialize_message(&msg, buffer, BUFFER_SIZE);
    if (msg_size <= 0) {
        LOG_ERROR("Failed to serialize market data request");
        return ERROR_SERIALIZATION;
    }

    if (send(context->socket, buffer, msg_size, 0) < 0) {
        LOG_ERROR("Failed to send market data request: %s", strerror(errno));
        return ERROR_SOCKET_CONNECT;
    }

    context->stats.messages_sent++;
    LOG_INFO("Requested market data for symbol: %s", symbol);
    return SUCCESS;
}

// Disconnect from server
void disconnect_from_server(ClientContext* context) {
    if (!context) return;

    context->running = 0;
    if (context->socket >= 0) {
        close(context->socket);
        context->socket = -1;
    }

    if (context->receiver_thread) {
        pthread_join(context->receiver_thread, NULL);
    }

    context->state = CLIENT_DISCONNECTED;
    LOG_INFO("Disconnected from server");
}

// Clean up client resources
void cleanup_client(ClientContext* context) {
    if (!context) return;

    disconnect_from_server(context);
    pthread_mutex_destroy(&context->state_mutex);
    pthread_mutex_destroy(&context->stats_mutex);
    free(context);

    LOG_INFO("Client resources cleaned up");
}

