#include "server/server.h"

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

int accept_client(ServerContext* context) {
    if (!context) return ERROR_INVALID_PARAM;

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_socket = accept(context->server_socket, (struct sockaddr*)&client_addr, &client_len);

    if (client_socket < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No pending connections, this is normal in non-blocking mode
            usleep(10000); // Sleep for 10ms to avoid busy looping
            return ERROR_TIMEOUT;
        } else {
            // Log unexpected errors
            LOG_ERROR("Failed to accept client connection: %s", strerror(errno));
            return ERROR_SOCKET_ACCEPT;
        }
    }

    // Process the new client connection
    LOG_INFO("Accepted new client connection from %s:%d",
             inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Add the client to the context's clients array
    pthread_mutex_lock(&context->clients_mutex);
    if (context->client_count >= context->config.max_clients) {
        LOG_ERROR("Maximum client limit reached, rejecting connection");
        close(client_socket);
        pthread_mutex_unlock(&context->clients_mutex);
        return ERROR_INVALID_STATE;
    }

    ClientConnection* client = &context->clients[context->client_count++];
    client->socket = client_socket;
    client->address = client_addr;
    client->connect_time = time(NULL);
    client->last_heartbeat = time(NULL);
    client->context = context;

    pthread_mutex_unlock(&context->clients_mutex);
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

        client->messages_received++;
        client->last_heartbeat = time(NULL);

        switch (msg.type) {
            case MSG_ORDER_NEW:
            case MSG_ORDER_MODIFY:
            case MSG_ORDER_CANCEL:
                LOG_INFO("Processing order type %d from client %s", msg.type, client->id);
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

    disconnect_client(context, client);
    return NULL;
}

void disconnect_client(ServerContext* context, ClientConnection* client) {
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
    LOG_INFO("Client %s disconnected and cleaned up", client->id);
}
