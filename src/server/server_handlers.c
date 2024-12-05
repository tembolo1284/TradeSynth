#include <stdio.h>
#include <string.h>
#include "server/server_handlers.h"
#include "common/logger.h"
#include "serialization/serialization.h"

static int find_client_socket(ServerContext* context, const char* client_id);
static uint32_t get_client_position(ServerContext* context, const char* client_id, const char* symbol);
static MarketData* get_market_data(ServerContext* context, const char* symbol);

int handle_message(ServerContext* context, int client_socket, const Message* msg) {
    LOG_INFO("Handling message type: %d", msg->type);
    
    switch (msg->type) {
        case MSG_HEARTBEAT:
            return handle_heartbeat(context, client_socket, msg);
        case MSG_ORDER_NEW:
        case MSG_ORDER_CANCEL:
        case MSG_ORDER_MODIFY:
        case MSG_ORDER_STATUS:
            return handle_order_message(context, client_socket, msg);
        case MSG_MARKET_DATA:
            return handle_market_data(context, client_socket, msg);
        case MSG_TRADE_EXEC:
            return handle_trade_exec(context, client_socket, msg);
        case MSG_ERROR:
            return handle_error(context, client_socket, msg);
        default:
            LOG_ERROR("Unknown message type: %d", msg->type);
            return ERROR_INVALID_MESSAGE;
    }
}

int handle_heartbeat(ServerContext* context __attribute__((unused)), 
                    int client_socket,
                    const Message* msg) {
    LOG_DEBUG("Received heartbeat, sequence: %lu", msg->sequence_num);
    
    Message response = {
        .type = MSG_HEARTBEAT,
        .sequence_num = msg->sequence_num + 1,
        .timestamp = time(NULL)
    };
    
    return send_response_message(client_socket, &response);
}

int handle_order_message(ServerContext* context,
                        int client_socket __attribute__((unused)),
                        const Message* msg) {
    const Order* order = &msg->data.order;
    LOG_INFO("Processing order:");
    LOG_INFO("  Message Type: %d", msg->type);
    LOG_INFO("  Order ID: %lu", order->order_id);
    LOG_INFO("  Symbol: %s", order->symbol);
    LOG_INFO("  Client ID: %s", order->client_id);
    LOG_INFO("  Type: %d", order->type);
    LOG_INFO("  Side: %d", order->side);
    LOG_INFO("  Price: %.6f", price_to_double(order->price));
    LOG_INFO("  Quantity: %u", order->quantity);
    
    return process_order(context, order);
}

int process_order(ServerContext* context, const Order* order) {
    // Create a working copy of the order
    Order processed_order = *order;
    
    // Validate order parameters
    if (processed_order.quantity == 0 || processed_order.quantity > 1000000) {
        LOG_ERROR("Invalid order quantity: %u", processed_order.quantity);
        return ERROR_INVALID_ORDER;
    }

    // Position limit checks
    uint32_t total_position = get_client_position(context,
                                               processed_order.client_id,
                                               processed_order.symbol);
    if (processed_order.side == ORDER_SIDE_BUY &&
        total_position + processed_order.quantity > 1000000) { // Example limit
        LOG_ERROR("Position limit exceeded for %s", processed_order.client_id);
        return ERROR_INVALID_ORDER;
    }

    // Price improvement for limit orders
    if (processed_order.type == ORDER_TYPE_LIMIT) {
        MarketData* market_data = get_market_data(context, processed_order.symbol);
        if (market_data) {
            if (processed_order.side == ORDER_SIDE_BUY &&
                price_to_double(processed_order.price) < price_to_double(market_data->ask)) {
                processed_order.price = double_to_price(
                    price_to_double(processed_order.price) + 0.01);
            }
        }
    }

    // Update status and send message
    processed_order.status = ORDER_STATUS_NEW;
    processed_order.modification_time = time(NULL);
    
    Message response = {
        .type = MSG_ORDER_STATUS,
        .sequence_num = atomic_fetch_add(&context->sequence_num, 1),
        .timestamp = time(NULL),
        .data.order = processed_order
    };

    return send_response_message(find_client_socket(context, processed_order.client_id),
                              &response);
}

int handle_market_data(ServerContext* context,
                      int client_socket __attribute__((unused)),
                      const Message* msg) {
    const MarketData* mkt_data = &msg->data.market_data;
    LOG_INFO("Received market data:");
    LOG_INFO("  Symbol: %s", mkt_data->symbol);
    LOG_INFO("  Last Price: %.6f", price_to_double(mkt_data->last_price));
    LOG_INFO("  Bid: %.6f", price_to_double(mkt_data->bid));
    LOG_INFO("  Ask: %.6f", price_to_double(mkt_data->ask));
    LOG_INFO("  Volume: %lu", mkt_data->volume);
    
    return broadcast_market_data(context, mkt_data);
}

int broadcast_market_data(ServerContext* context, const MarketData* market_data) {
    Message msg = {
        .type = MSG_MARKET_DATA,
        .sequence_num = atomic_fetch_add(&context->sequence_num, 1),
        .timestamp = time(NULL),
        .data.market_data = *market_data
    };
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (context->clients[i].socket > 0) {
            send_response_message(context->clients[i].socket, &msg);
        }
    }
    
    return SUCCESS;
}

int handle_trade_exec(ServerContext* context,
                     int client_socket __attribute__((unused)),
                     const Message* msg) {
    const TradeExecution* trade = &msg->data.trade;
    LOG_INFO("Processing trade execution:");
    LOG_INFO("  Trade ID: %lu", trade->trade_id);
    LOG_INFO("  Order ID: %lu", trade->order_id);
    LOG_INFO("  Symbol: %s", trade->symbol);
    LOG_INFO("  Price: %.6f", price_to_double(trade->price));
    LOG_INFO("  Quantity: %u", trade->quantity);
    LOG_INFO("  Buyer: %s", trade->buyer_id);
    LOG_INFO("  Seller: %s", trade->seller_id);
    
    return process_trade_execution(context, trade);
}

int process_trade_execution(ServerContext* context, const TradeExecution* trade) {
    Message msg = {
        .type = MSG_TRADE_EXEC,
        .sequence_num = atomic_fetch_add(&context->sequence_num, 1),
        .timestamp = time(NULL),
        .data.trade = *trade
    };
    
    int buyer_socket = find_client_socket(context, trade->buyer_id);
    int seller_socket = find_client_socket(context, trade->seller_id);
    
    if (buyer_socket > 0) {
        send_response_message(buyer_socket, &msg);
    }
    if (seller_socket > 0) {
        send_response_message(seller_socket, &msg);
    }
    
    return SUCCESS;
}

int handle_error(ServerContext* context __attribute__((unused)),
                int client_socket __attribute__((unused)),
                const Message* msg) {
    LOG_ERROR("Received error message: [%d] %s",
              msg->data.error.code,
              msg->data.error.message);
    return SUCCESS;
}

int send_response_message(int client_socket, const Message* response) {
    uint8_t buffer[BUFFER_SIZE];
    ssize_t serialized_size = serialize_message(response, buffer, BUFFER_SIZE);
    
    if (serialized_size < 0) {
        LOG_ERROR("Failed to serialize response message");
        return ERROR_SERIALIZATION;
    }
    
    if (send(client_socket, buffer, serialized_size, 0) != serialized_size) {
        LOG_ERROR("Failed to send response message");
        return ERROR_SOCKET_CONNECT;
    }
    
    return SUCCESS;
}

static int find_client_socket(ServerContext* context, const char* client_id) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (context->clients[i].socket > 0 &&
            strcmp(context->clients[i].id, client_id) == 0) {
            return context->clients[i].socket;
        }
    }
    return -1;
}

static uint32_t get_client_position(ServerContext* context __attribute__((unused)),
                                  const char* client_id __attribute__((unused)),
                                  const char* symbol __attribute__((unused))) {
    // Placeholder implementation
    return 0;
}

static MarketData* get_market_data(ServerContext* context __attribute__((unused)),
                                 const char* symbol __attribute__((unused))) {
    // Placeholder implementation
    static MarketData dummy_data;
    return &dummy_data;
}

uint64_t generate_order_id(ServerContext* context) {
    return atomic_fetch_add(&context->sequence_num, 1);
}

uint64_t generate_trade_id(ServerContext* context) {
    return atomic_fetch_add(&context->sequence_num, 1);
}
