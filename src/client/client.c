#include "client/client.h"
#include "common/logger.h"

int initialize_client(const char* server_ip, int port) {
    LOG_PERF_START(client_init);
    int client_socket;
    struct sockaddr_in server_addr;
    
    LOG_INFO("Initializing client connection to %s:%d", server_ip, port);
    
    // Create socket
    LOG_DEBUG("Creating client socket");
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        LOG_ERROR("Failed to create socket: %s", strerror(errno));
        return ERROR_SOCKET_CREATE;
    }
    
    // Initialize server address structure
    LOG_DEBUG("Configuring server address");
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    // Convert IP address
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        LOG_ERROR("Invalid server IP address %s: %s", server_ip, strerror(errno));
        close(client_socket);
        return ERROR_SOCKET_CREATE;
    }
    
    LOG_INFO("Attempting connection to server");
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR("Connection failed: %s", strerror(errno));
        close(client_socket);
        return ERROR_SOCKET_CREATE;
    }
    
    LOG_PERF_END(client_init);
    LOG_INFO("Successfully connected to server");
    return client_socket;
}

int send_order(int client_socket, const Order* order) {
    LOG_PERF_START(send_order);
    Message msg;
    char buffer[BUFFER_SIZE];
    
    LOG_INFO("Preparing to send order: ID=%ld, Symbol=%s, Type=%d, Side=%d", 
             order->order_id, order->symbol, order->type, order->side);
    
    // Prepare message
    msg.type = ORDER_NEW;
    msg.data.order = *order;
    
    // Serialize message
    LOG_DEBUG("Serializing order message");
    size_t msg_size = serialize_message(&msg, buffer, BUFFER_SIZE);
    if (msg_size <= 0) {
        LOG_ERROR("Failed to serialize order message");
        return -1;
    }
    
    // Send message
    LOG_DEBUG("Sending %zu bytes", msg_size);
    int result = send(client_socket, buffer, msg_size, 0);
    if (result < 0) {
        LOG_ERROR("Failed to send order: %s", strerror(errno));
    } else {
        LOG_INFO("Order sent successfully (%d bytes)", result);
    }
    
    LOG_PERF_END(send_order);
    return result;
}

int receive_market_data(int client_socket, MarketData* market_data) {
    LOG_PERF_START(receive_market_data);
    char buffer[BUFFER_SIZE];
    Message msg;
    
    // Receive message
    LOG_DEBUG("Waiting for market data");
    ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            LOG_WARN("Server closed connection");
        } else {
            LOG_ERROR("Failed to receive market data: %s", strerror(errno));
        }
        return -1;
    }
    
    LOG_DEBUG("Received %zd bytes of market data", bytes_received);
    
    // Deserialize message
    if (deserialize_message(buffer, bytes_received, &msg) != SUCCESS) {
        LOG_ERROR("Failed to deserialize market data message");
        return -1;
    }
    
    // Check message type
    if (msg.type != MARKET_DATA) {
        LOG_ERROR("Received unexpected message type: %d", msg.type);
        return -1;
    }
    
    // Copy market data
    *market_data = msg.data.market_data;
    LOG_INFO("Received market data for %s: Last=%.2f, Bid=%.2f, Ask=%.2f", 
             market_data->symbol, market_data->last_price, 
             market_data->bid, market_data->ask);
    
    LOG_PERF_END(receive_market_data);
    return SUCCESS;
}
