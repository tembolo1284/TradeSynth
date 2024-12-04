#include "server/server.h"

static volatile sig_atomic_t server_running = 1;

void signal_handler(int signum) {
    LOG_INFO("Received signal %d, initiating shutdown", signum);
    server_running = 0;
}

int validate_server_config(const ServerConfig* config) {
    if (!config) return 0;
    
    if (config->port <= 0 || config->port > 65535) {
        LOG_ERROR("Invalid port number: %d", config->port);
        return 0;
    }
    
    if (config->max_clients <= 0 || config->max_clients > MAX_CLIENTS) {
        LOG_ERROR("Invalid max clients: %d", config->max_clients);
        return 0;
    }
    
    if (config->socket_timeout < 0) {
        LOG_ERROR("Invalid socket timeout: %d", config->socket_timeout);
        return 0;
    }
    
    return 1;
}

int setup_socket(ServerContext* context) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        LOG_ERROR("Failed to create socket: %s", strerror(errno));
        return ERROR_SOCKET_CREATE;
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        LOG_ERROR("Failed to set socket options: %s", strerror(errno));
        close(server_socket);
        return ERROR_SOCKET_CREATE;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(context->config.port),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR("Failed to bind socket: %s", strerror(errno));
        close(server_socket);
        return ERROR_SOCKET_BIND;
    }

    if (listen(server_socket, MAX_PENDING_CONNECTIONS) < 0) {
        LOG_ERROR("Failed to listen on socket: %s", strerror(errno));
        close(server_socket);
        return ERROR_SOCKET_LISTEN;
    }

    LOG_INFO("Server socket setup successfully on port %d", context->config.port);
    return server_socket;
}

ServerContext* initialize_server_context(const ServerConfig* config) {
    if (!validate_server_config(config)) {
        LOG_ERROR("Invalid server configuration");
        return NULL;
    }

    ServerContext* context = (ServerContext*)calloc(1, sizeof(ServerContext));
    if (!context) {
        LOG_ERROR("Failed to allocate server context");
        return NULL;
    }

    context->config = *config;
    context->state = SERVER_STOPPED;
    context->stats.start_time = time(NULL);
    
    pthread_mutex_init(&context->stats_mutex, NULL);
    pthread_mutex_init(&context->clients_mutex, NULL);

    context->clients = (ClientConnection*)calloc(config->max_clients, sizeof(ClientConnection));
    if (!context->clients) {
        LOG_ERROR("Failed to allocate client connection array");
        free(context);
        return NULL;
    }

    LOG_INFO("Server context initialized successfully");
    return context;
}

int start_server(ServerContext* context) {
    if (!context) {
        LOG_ERROR("Null server context");
        return ERROR_INVALID_PARAM;
    }

    LOG_INFO("Starting server on port %d", context->config.port);
    
    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    context->server_socket = setup_socket(context);
    if (context->server_socket < 0) {
        LOG_ERROR("Failed to set up server socket");
        return context->server_socket;
    }

    context->state = SERVER_RUNNING;
    LOG_INFO("Server started successfully");

    while (server_running) {
        int client_socket = accept_client(context);
        if (client_socket < 0) {
            if (server_running) {
                LOG_ERROR("Failed to accept client connection");
            }
            continue;
        }
    }

    stop_server(context);
    return SUCCESS;
}

void* handle_client(void* arg) {
    ClientConnection* client = (ClientConnection*)arg;
    char buffer[BUFFER_SIZE];
    Message msg;
    ServerContext* context = (ServerContext*)client->context;

    LOG_INFO("Handling new client connection: %s", client->id);

    while (server_running) {
        ssize_t bytes_received = recv(client->socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                LOG_INFO("Client %s disconnected", client->id);
            } else {
                LOG_ERROR("Error receiving from client %s: %s", 
                         client->id, strerror(errno));
            }
            break;
        }

        if (deserialize_message((const uint8_t*)buffer, bytes_received, &msg) != SUCCESS) {
            LOG_ERROR("Failed to deserialize message from client %s", client->id);
            continue;
        }

        switch (msg.type) {
            case MSG_ORDER_NEW:
                LOG_INFO("New order from client %s", client->id);
                process_order(context, &msg.data.order);
                break;

            case MSG_ORDER_MODIFY:
                LOG_INFO("Order modification from client %s", client->id);
                process_order(context, &msg.data.order);
                break;

            case MSG_ORDER_CANCEL:
                LOG_INFO("Order cancellation from client %s", client->id);
                process_order(context, &msg.data.order);
                break;

            case MSG_MARKET_DATA:
                LOG_DEBUG("Market data update for %s", msg.data.market_data.symbol);
                broadcast_market_data(context, &msg.data.market_data);
                break;

            case MSG_TRADE_EXEC:
                LOG_INFO("Trade execution for %s", msg.data.trade.symbol);
                process_trade_execution(context, &msg.data.trade);
                break;

            default:
                LOG_WARN("Unknown message type %d from client %s", msg.type, client->id);
        }
    }

    pthread_mutex_lock(&context->clients_mutex);
    for (int i = 0; i < context->client_count; i++) {
        if (strcmp(context->clients[i].id, client->id) == 0) {
            if (i < context->client_count - 1) {
                memmove(&context->clients[i], &context->clients[i + 1], 
                        (context->client_count - i - 1) * sizeof(ClientConnection));
            }
            context->client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&context->clients_mutex);

    close(client->socket);
    free(client);
    return NULL;
}

int process_order(ServerContext* context, const Order* order) {
    LOG_PERF_START(process_order);
    
    if (!context || !order) {
        LOG_ERROR("Invalid parameters in process_order");
        return ERROR_INVALID_PARAM;
    }
    
    LOG_INFO("Processing order ID=%lu for symbol %s", order->order_id, order->symbol);
    
    // Add actual order processing logic here
    // This is just a placeholder that acknowledges the order
    Message response = {
        .type = MSG_ORDER_STATUS,
        .data.order = *order
    };
    
    response.data.order.status = ORDER_STATUS_NEW;
    
    LOG_PERF_END(process_order);
    return SUCCESS;
}

int broadcast_market_data(ServerContext* context, const MarketData* market_data) {
    LOG_PERF_START(broadcast_market_data);
    
    if (!context || !market_data) {
        LOG_ERROR("Invalid parameters in broadcast_market_data");
        return ERROR_INVALID_PARAM;
    }
    
    Message msg = {
        .type = MSG_MARKET_DATA,
        .data.market_data = *market_data
    };
    
    uint8_t buffer[BUFFER_SIZE];
    size_t msg_size = serialize_message(&msg, buffer, BUFFER_SIZE);
    if (msg_size <= 0) {
        LOG_ERROR("Failed to serialize market data message");
        return ERROR_SERIALIZATION;
    }
    
    pthread_mutex_lock(&context->clients_mutex);
    
    for (int i = 0; i < context->client_count; i++) {
        if (send(context->clients[i].socket, buffer, msg_size, 0) < 0) {
            LOG_ERROR("Failed to send market data to client %s: %s", 
                     context->clients[i].id, strerror(errno));
        }
    }
    
    pthread_mutex_unlock(&context->clients_mutex);
    
    LOG_PERF_END(broadcast_market_data);
    return SUCCESS;
}

int process_trade_execution(ServerContext* context, const TradeExecution* trade) {
    LOG_PERF_START(process_trade);
    
    if (!context || !trade) {
        LOG_ERROR("Invalid parameters in process_trade_execution");
        return ERROR_INVALID_PARAM;
    }
    
    LOG_INFO("Processing trade ID=%lu for order ID=%lu", 
             trade->trade_id, trade->order_id);
    
    // Update relevant orders and notify clients
    // This is a placeholder implementation
    Message msg = {
        .type = MSG_TRADE_EXEC,
        .data.trade = *trade
    };
    
    uint8_t buffer[BUFFER_SIZE];
    size_t msg_size = serialize_message(&msg, buffer, BUFFER_SIZE);
    if (msg_size <= 0) {
        LOG_ERROR("Failed to serialize trade execution message");
        return ERROR_SERIALIZATION;
    }
    
    // Notify involved parties
    pthread_mutex_lock(&context->clients_mutex);
    for (int i = 0; i < context->client_count; i++) {
        if (strcmp(context->clients[i].id, trade->buyer_id) == 0 ||
            strcmp(context->clients[i].id, trade->seller_id) == 0) {
            if (send(context->clients[i].socket, buffer, msg_size, 0) < 0) {
                LOG_ERROR("Failed to send trade execution to client %s: %s", 
                         context->clients[i].id, strerror(errno));
            }
        }
    }
    pthread_mutex_unlock(&context->clients_mutex);
    
    LOG_PERF_END(process_trade);
    return SUCCESS;
}

void stop_server(ServerContext* context) {
    if (!context) return;

    LOG_INFO("Stopping server");
    context->state = SERVER_STOPPING;

    // Close server socket
    if (context->server_socket >= 0) {
        close(context->server_socket);
        context->server_socket = -1;
    }

    // Disconnect all clients
    pthread_mutex_lock(&context->clients_mutex);
    for (int i = 0; i < context->client_count; i++) {
        close(context->clients[i].socket);
    }
    context->client_count = 0;
    pthread_mutex_unlock(&context->clients_mutex);

    context->state = SERVER_STOPPED;
    LOG_INFO("Server stopped");
}

void cleanup_server(ServerContext* context) {
    if (!context) return;

    stop_server(context);
    
    pthread_mutex_destroy(&context->stats_mutex);
    pthread_mutex_destroy(&context->clients_mutex);
    
    free(context->clients);
    free(context);
    
    LOG_INFO("Server resources cleaned up");
}
