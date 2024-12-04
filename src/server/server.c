#include "server/server.h"
#include "common/logger.h"
#include <signal.h>

static volatile sig_atomic_t server_running = 1;

void signal_handler(int signum) {
    LOG_INFO("Received signal %d, initiating shutdown", signum);
    server_running = 0;
}

int initialize_server(int port) {
    LOG_PERF_START(server_init);
    int server_socket;
    struct sockaddr_in server_addr;
    
    // Create socket
    LOG_DEBUG("Creating server socket");
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        LOG_ERROR("Failed to create socket: %s", strerror(errno));
        return ERROR_SOCKET_CREATE;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        LOG_ERROR("Failed to set socket options: %s", strerror(errno));
        close(server_socket);
        return ERROR_SOCKET_CREATE;
    }
    
    // Initialize server address structure
    LOG_DEBUG("Configuring server address on port %d", port);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR("Failed to bind socket: %s", strerror(errno));
        close(server_socket);
        return ERROR_SOCKET_BIND;
    }
    
    // Listen for connections
    LOG_DEBUG("Setting up listener with maximum %d pending connections", MAX_CLIENTS);
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        LOG_ERROR("Failed to listen on socket: %s", strerror(errno));
        close(server_socket);
        return ERROR_SOCKET_LISTEN;
    }
    
    LOG_PERF_END(server_init);
    LOG_INFO("Server initialized successfully on port %d", port);
    return server_socket;
}

void* handle_client(void* arg) {
    ClientConnection* client = (ClientConnection*)arg;
    char buffer[BUFFER_SIZE];
    Message msg;
    char client_ip[INET_ADDRSTRLEN];
    
    inet_ntop(AF_INET, &(client->address.sin_addr), client_ip, INET_ADDRSTRLEN);
    LOG_INFO("New client connection from %s:%d", client_ip, ntohs(client->address.sin_port));
    
    // Generate unique client ID
    snprintf(client->id, sizeof(client->id), "CLIENT_%s_%d", client_ip, ntohs(client->address.sin_port));
    LOG_DEBUG("Assigned client ID: %s", client->id);
    
    while (server_running) {
        LOG_PERF_START(message_processing);
        
        // Receive message
        ssize_t bytes_received = recv(client->socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                LOG_INFO("Client %s disconnected gracefully", client->id);
            } else {
                LOG_ERROR("Error receiving from client %s: %s", client->id, strerror(errno));
            }
            break;
        }
        
        LOG_DEBUG("Received %zd bytes from client %s", bytes_received, client->id);
        
        // Deserialize message
        if (deserialize_message(buffer, bytes_received, &msg) != SUCCESS) {
            LOG_ERROR("Failed to deserialize message from client %s", client->id);
            continue;
        }
        
        // Process message based on type
        LOG_DEBUG("Processing message type %d from client %s", msg.type, client->id);
        switch (msg.type) {
            case ORDER_NEW:
                LOG_INFO("New order received from client %s: OrderID=%ld, Symbol=%s", 
                        client->id, msg.data.order.order_id, msg.data.order.symbol);
                process_order(&msg.data.order);
                break;
                
            case ORDER_MODIFY:
                LOG_INFO("Order modification from client %s: OrderID=%ld", 
                        client->id, msg.data.order.order_id);
                process_order(&msg.data.order);
                break;
                
            case ORDER_CANCEL:
                LOG_INFO("Order cancellation from client %s: OrderID=%ld", 
                        client->id, msg.data.order.order_id);
                process_order(&msg.data.order);
                break;
                
            case MARKET_DATA:
                LOG_DEBUG("Broadcasting market data update for %s", msg.data.market_data.symbol);
                broadcast_market_data(&msg.data.market_data);
                break;
                
            case TRADE_EXECUTION:
                LOG_INFO("Trade execution for OrderID=%ld: Price=%.2f, Quantity=%d", 
                        msg.data.trade.order_id, msg.data.trade.price, msg.data.trade.quantity);
                process_trade_execution(&msg.data.trade);
                break;
                
            default:
                LOG_WARN("Unknown message type %d from client %s", msg.type, client->id);
        }
        
        LOG_PERF_END(message_processing);
    }
    
    // Clean up client connection
    LOG_INFO("Cleaning up connection for client %s", client->id);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].id, client->id) == 0) {
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            LOG_DEBUG("Removed client %s from active clients list. Active clients: %d", 
                     client->id, client_count);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    close(client->socket);
    free(client);
    LOG_INFO("Client %s connection cleaned up successfully", client->id);
    return NULL;
}
