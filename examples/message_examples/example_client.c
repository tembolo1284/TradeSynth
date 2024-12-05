// examples/message_examples/example_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../include/client/client.h"
#include "../../include/common/logger.h"
#include "../../include/common/types.h"
#include "../../include/serialization/serialization.h"

// Helper function to send a message and receive response
int send_and_receive(ClientContext* client, Message* msg) {
    uint8_t send_buffer[BUFFER_SIZE];
    uint8_t recv_buffer[BUFFER_SIZE];
    Message response;

    ssize_t serialized_size = serialize_message(msg, send_buffer, BUFFER_SIZE);
    if (serialized_size < 0) {
        LOG_ERROR("Failed to serialize message");
        return ERROR_SERIALIZATION;
    }

    if (send_data(client, send_buffer, serialized_size) != serialized_size) {
        LOG_ERROR("Failed to send message");
        return ERROR_SOCKET_CONNECT;
    }

    LOG_INFO("Sent message type %d, sequence %lu", msg->type, msg->sequence_num);

    // Wait for response
    ssize_t received = receive_data(client, recv_buffer, BUFFER_SIZE);
    if (received > 0) {
        if (deserialize_message(recv_buffer, received, &response) == SUCCESS) {
            LOG_INFO("Received response type %d, sequence %lu", 
                    response.type, response.sequence_num);
            return SUCCESS;
        }
    }

    return ERROR_SOCKET_CONNECT;
}

// Create and send two order messages
void send_orders(ClientContext* client) {
    // First order - Market Buy
    Order buy_order = {
        .order_id = 1001,
        .type = ORDER_TYPE_MARKET,
        .side = ORDER_SIDE_BUY,
        .status = ORDER_STATUS_NEW,
        .time_in_force = TIF_DAY,
        .quantity = 100,
        .filled_quantity = 0,
        .remaining_quantity = 100,
        .creation_time = time(NULL),
        .modification_time = time(NULL),
        .expiration_time = time(NULL) + 86400
    };
    strncpy(buy_order.symbol, "AAPL", MAX_SYMBOL_LENGTH);
    strncpy(buy_order.client_id, "CLIENT001", MAX_CLIENT_ID_LENGTH);
    buy_order.price = double_to_price(150.50);

    Message buy_msg = {
        .type = MSG_ORDER_NEW,
        .sequence_num = 1,
        .timestamp = time(NULL),
        .data.order = buy_order
    };

    // Second order - Limit Sell
    Order sell_order = {
        .order_id = 1002,
        .type = ORDER_TYPE_LIMIT,
        .side = ORDER_SIDE_SELL,
        .status = ORDER_STATUS_NEW,
        .time_in_force = TIF_GTC,
        .quantity = 200,
        .filled_quantity = 0,
        .remaining_quantity = 200,
        .creation_time = time(NULL),
        .modification_time = time(NULL),
        .expiration_time = time(NULL) + 86400 * 30
    };
    strncpy(sell_order.symbol, "MSFT", MAX_SYMBOL_LENGTH);
    strncpy(sell_order.client_id, "CLIENT001", MAX_CLIENT_ID_LENGTH);
    sell_order.price = double_to_price(280.75);

    Message sell_msg = {
        .type = MSG_ORDER_NEW,
        .sequence_num = 2,
        .timestamp = time(NULL),
        .data.order = sell_order
    };

    send_and_receive(client, &buy_msg);
    send_and_receive(client, &sell_msg);
}

// Create and send two market data messages
void send_market_data(ClientContext* client) {
    // First market data update
    MarketData data1 = {
        .last_price = double_to_price(150.50),
        .bid = double_to_price(150.45),
        .ask = double_to_price(150.55),
        .last_size = 100,
        .bid_size = 500,
        .ask_size = 700,
        .volume = 1000000,
        .num_trades = 1250,
        .timestamp = time(NULL)
    };
    strncpy(data1.symbol, "AAPL", MAX_SYMBOL_LENGTH);

    Message mkt_msg1 = {
        .type = MSG_MARKET_DATA,
        .sequence_num = 3,
        .timestamp = time(NULL),
        .data.market_data = data1
    };

    // Second market data update
    MarketData data2 = {
        .last_price = double_to_price(280.75),
        .bid = double_to_price(280.70),
        .ask = double_to_price(280.80),
        .last_size = 50,
        .bid_size = 300,
        .ask_size = 400,
        .volume = 750000,
        .num_trades = 980,
        .timestamp = time(NULL)
    };
    strncpy(data2.symbol, "MSFT", MAX_SYMBOL_LENGTH);

    Message mkt_msg2 = {
        .type = MSG_MARKET_DATA,
        .sequence_num = 4,
        .timestamp = time(NULL),
        .data.market_data = data2
    };

    send_and_receive(client, &mkt_msg1);
    send_and_receive(client, &mkt_msg2);
}

// Create and send two trade execution messages
void send_trade_executions(ClientContext* client) {
    // First trade execution
    TradeExecution trade1 = {
        .trade_id = 5001,
        .order_id = 1001,
        .price = double_to_price(150.50),
        .quantity = 100,
        .timestamp = time(NULL)
    };
    strncpy(trade1.symbol, "AAPL", MAX_SYMBOL_LENGTH);
    strncpy(trade1.buyer_id, "CLIENT001", MAX_CLIENT_ID_LENGTH);
    strncpy(trade1.seller_id, "MARKET", MAX_CLIENT_ID_LENGTH);

    Message trade_msg1 = {
        .type = MSG_TRADE_EXEC,
        .sequence_num = 5,
        .timestamp = time(NULL),
        .data.trade = trade1
    };

    // Second trade execution
    TradeExecution trade2 = {
        .trade_id = 5002,
        .order_id = 1002,
        .price = double_to_price(280.75),
        .quantity = 200,
        .timestamp = time(NULL)
    };
    strncpy(trade2.symbol, "MSFT", MAX_SYMBOL_LENGTH);
    strncpy(trade2.buyer_id, "MARKET", MAX_CLIENT_ID_LENGTH);
    strncpy(trade2.seller_id, "CLIENT001", MAX_CLIENT_ID_LENGTH);

    Message trade_msg2 = {
        .type = MSG_TRADE_EXEC,
        .sequence_num = 6,
        .timestamp = time(NULL),
        .data.trade = trade2
    };

    send_and_receive(client, &trade_msg1);
    send_and_receive(client, &trade_msg2);
}

// Send heartbeat messages
void send_heartbeats(ClientContext* client) {
    // First heartbeat
    Message heartbeat1 = {
        .type = MSG_HEARTBEAT,
        .sequence_num = 7,
        .timestamp = time(NULL)
    };

    // Second heartbeat
    Message heartbeat2 = {
        .type = MSG_HEARTBEAT,
        .sequence_num = 8,
        .timestamp = time(NULL)
    };

    send_and_receive(client, &heartbeat1);
    send_and_receive(client, &heartbeat2);
}

int main(void) {
    init_logger("client.log", LOG_DEBUG);
    LOG_INFO("Starting example client...");

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

    LOG_INFO("Connected to server, sending messages...");

    // Send two of each message type
    send_orders(client);
    send_market_data(client);
    send_trade_executions(client);
    send_heartbeats(client);

    // Cleanup
    destroy_client_context(client);
    LOG_INFO("Client shutdown complete");
    return 0;
}