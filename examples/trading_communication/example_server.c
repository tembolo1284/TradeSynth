// examples/trading_communication/example_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "../../include/server/server.h"

void example_handle_order(const Order* order, int client_socket) {
    LOG_INFO("Received order:");
    LOG_INFO("  Order ID: %lu", order->order_id);
    LOG_INFO("  Symbol: %s", order->symbol);
    LOG_INFO("  Price: %.2f", price_to_double(order->price));
    LOG_INFO("  Quantity: %u", order->quantity);
    LOG_INFO("  Side: %s", order->side == ORDER_SIDE_BUY ? "BUY" : "SELL");
    LOG_INFO("  Type: %d", order->type);
    LOG_INFO("  Client ID: %s", order->client_id);

    // Create a simulated trade execution
    Message response = {
        .type = MSG_TRADE_EXEC,
        .sequence_num = 1,
        .timestamp = time(NULL)
    };

    TradeExecution* trade = &response.data.trade;
    trade->trade_id = 98765;
    trade->order_id = order->order_id;
    strncpy(trade->symbol, order->symbol, MAX_SYMBOL_LENGTH);
    trade->price = order->price;
    trade->quantity = order->quantity;
    trade->timestamp = time(NULL);
    strncpy(trade->buyer_id, order->client_id, MAX_CLIENT_ID_LENGTH);
    strncpy(trade->seller_id, "MARKET", MAX_CLIENT_ID_LENGTH);

    // Serialize and send the trade execution
    uint8_t buffer[BUFFER_SIZE];
    ssize_t serialized_size = serialize_message(&response, buffer, BUFFER_SIZE);
    if (serialized_size > 0) {
        send(client_socket, buffer, serialized_size, 0);
        LOG_INFO("Sent trade execution confirmation");
    }
}

void example_handle_client_message(ServerContext* context, int client_socket, const uint8_t* data, size_t size) {
    Message msg;
    if (deserialize_message(data, size, &msg) == SUCCESS) {
        switch (msg.type) {
            case MSG_ORDER_NEW:
                // First process using the server's handler
                if (process_order(context, &msg.data.order) == SUCCESS) {
                    // Then handle our example-specific logic
                    example_handle_order(&msg.data.order, client_socket);
                }
                break;
            default:
                LOG_WARN("Unhandled message type: %d", msg.type);
                break;
        }
    } else {
        LOG_ERROR("Failed to deserialize message");
    }
}

// Custom client handler for the example
void* example_client_handler(void* arg) {
    ClientConnection* client = (ClientConnection*)arg;
    uint8_t buffer[BUFFER_SIZE];
    
    LOG_INFO("New client connection thread started");
    
    ssize_t received = recv(client->socket, buffer, BUFFER_SIZE, 0);
    if (received > 0) {
        example_handle_client_message(client->context, client->socket, buffer, received);
    }
    
    close(client->socket);
    free(client);
    return NULL;
}

int main(void) {
    init_logger("server.log", LOG_DEBUG);
    LOG_INFO("Starting trading example server...");
    
    // Initialize server configuration
    ServerConfig config = {
        .port = 8080,
        .max_clients = MAX_CLIENTS,
        .client_handler = example_client_handler
    };
    
    // Initialize server context
    ServerContext* server = initialize_server_context(&config);
    if (!server) {
        LOG_ERROR("Failed to initialize server context");
        return 1;
    }
    
    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Start the server
    if (start_server(server) != SUCCESS) {
        LOG_ERROR("Failed to start server");
        cleanup_server(server);
        return 1;
    }
    
    LOG_INFO("Server listening on port %d", config.port);
    
    // Main server loop
    while (server_running) {
        sleep(1);  // Prevent busy waiting
    }
    
    // Cleanup
    stop_server(server);
    cleanup_server(server);
    LOG_INFO("Server shutdown complete");
    return 0;
}
