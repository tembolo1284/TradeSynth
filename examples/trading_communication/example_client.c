// examples/trading_communication/example_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../include/client/client.h"
#include "../../include/common/logger.h"
#include "../../include/common/types.h"
#include "../../include/serialization/serialization.h"

void on_connect(void* user_data __attribute__((unused))) {
    LOG_INFO("Connected to server");
}

void on_trade(const TradeExecution* trade, void* user_data __attribute__((unused))) {
    LOG_INFO("Received trade execution:");
    LOG_INFO("  Trade ID: %lu", trade->trade_id);
    LOG_INFO("  Order ID: %lu", trade->order_id);
    LOG_INFO("  Symbol: %s", trade->symbol);
    LOG_INFO("  Price: %.2f", price_to_double(trade->price));
    LOG_INFO("  Quantity: %u", trade->quantity);
    LOG_INFO("  Buyer: %s", trade->buyer_id);
    LOG_INFO("  Seller: %s", trade->seller_id);
}

int main(void) {
    init_logger("client.log", LOG_DEBUG);
    LOG_INFO("Starting trading example client...");
    
    ClientConfig config = {
        .server_port = 8080,
        .socket_timeout = DEFAULT_SOCKET_TIMEOUT,
        .reconnect_attempts = DEFAULT_RECONNECT_ATTEMPTS
    };
    strncpy(config.server_host, "localhost", sizeof(config.server_host));
    strncpy(config.client_id, "CLIENT001", MAX_CLIENT_ID_LENGTH);

    ClientCallbacks callbacks = {
        .on_connect = on_connect,
        .on_trade = on_trade
    };

    ClientContext* client = initialize_client(&config, &callbacks, NULL);
    if (!client) {
        LOG_ERROR("Failed to create client context");
        return 1;
    }

    if (connect_to_server(client) != SUCCESS) {
        LOG_ERROR("Failed to connect to server");
        cleanup_client(client);
        return 1;
    }

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
        .expiration_time = time(NULL) + 86400
    };
    
    strncpy(order.symbol, "AAPL", MAX_SYMBOL_LENGTH);
    strncpy(order.client_id, "CLIENT001", MAX_CLIENT_ID_LENGTH);
    order.price = double_to_price(150.50);

    if (send_order(client, &order) != SUCCESS) {
        LOG_ERROR("Failed to send order");
        cleanup_client(client);
        return 1;
    }

    LOG_INFO("Sent order: ID=%lu, Symbol=%s, Price=%.2f, Quantity=%u, Side=%s",
             order.order_id, order.symbol, 
             price_to_double(order.price), 
             order.quantity,
             order.side == ORDER_SIDE_BUY ? "BUY" : "SELL");

    // Sleep to allow time for response
    sleep(1);

    cleanup_client(client);
    LOG_INFO("Client shutdown complete");
    return 0;
}
