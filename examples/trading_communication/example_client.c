// examples/trading_communication/example_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../include/client/client.h"
#include "../../include/common/logger.h"
#include "../../include/common/types.h"
#include "../../include/serialization/serialization.h"

int main(int argc, char *argv[]) {
    // Initialize logger
    init_logger("client.log", LOG_DEBUG);
    LOG_INFO("Starting trading example client...");
    
    // Create a sample order
    Order order = {
        .order_id = 12345,
        .type = ORDER_TYPE_LIMIT,
        .side = ORDER_SIDE_BUY,
        .status = ORDER_STATUS_NEW,
        .time_in_force = TIF_DAY,
        .quantity = 100,
        .filled_quantity = 0,
        .remaining_quantity = 100,
        .creation_time = time(NULL),
        .modification_time = time(NULL),
        .expiration_time = time(NULL) + 86400  // Expires in 24 hours
    };
    
    // Set symbol and client ID
    strncpy(order.symbol, "AAPL", MAX_SYMBOL_LENGTH);
    strncpy(order.client_id, "CLIENT001", MAX_CLIENT_ID_LENGTH);
    
    // Set price (e.g., $150.50)
    order.price = double_to_price(150.50);

    // Create message container
    Message msg = {
        .type = MSG_ORDER_NEW,
        .sequence_num = 1,
        .timestamp = time(NULL),
        .data.order = order
    };

    // Connect to server
    ClientContext* client = create_client_context();
    if (!client) {
        LOG_ERROR("Failed to create client context");
        return 1;
    }
    
    if (connect_to_server(client, "localhost", 8080) != 0) {
        LOG_ERROR("Failed to connect to server");
        destroy_client_context(client);
        return 1;
    }

    // Serialize and send the message
    uint8_t buffer[BUFFER_SIZE];
    ssize_t serialized_size = serialize_message(&msg, buffer, BUFFER_SIZE);
    if (serialized_size < 0) {
        LOG_ERROR("Failed to serialize message");
        destroy_client_context(client);
        return 1;
    }

    if (send_data(client, buffer, serialized_size) != serialized_size) {
        LOG_ERROR("Failed to send order");
        destroy_client_context(client);
        return 1;
    }

    LOG_INFO("Sent order: ID=%lu, Symbol=%s, Price=%.2f, Quantity=%u, Side=%s",
             order.order_id, order.symbol, 
             price_to_double(order.price), 
             order.quantity,
             order.side == ORDER_SIDE_BUY ? "BUY" : "SELL");

    // Wait for trade execution response
    uint8_t response_buffer[BUFFER_SIZE];
    ssize_t received = receive_data(client, response_buffer, BUFFER_SIZE);
    if (received > 0) {
        Message response_msg;
        if (deserialize_message(response_buffer, received, &response_msg) == SUCCESS) {
            if (response_msg.type == MSG_TRADE_EXEC) {
                TradeExecution* trade = &response_msg.data.trade;
                LOG_INFO("Received trade execution:");
                LOG_INFO("  Trade ID: %lu", trade->trade_id);
                LOG_INFO("  Order ID: %lu", trade->order_id);
                LOG_INFO("  Symbol: %s", trade->symbol);
                LOG_INFO("  Price: %.2f", price_to_double(trade->price));
                LOG_INFO("  Quantity: %u", trade->quantity);
                LOG_INFO("  Buyer: %s", trade->buyer_id);
                LOG_INFO("  Seller: %s", trade->seller_id);
            }
        }
    }

    // Cleanup
    destroy_client_context(client);
    LOG_INFO("Client shutdown complete");
    return 0;
}
