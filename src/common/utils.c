#include "common/utils.h"
#include "common/logger.h"

void process_order(const Order* order) {
    LOG_PERF_START(process_order);
    
    LOG_INFO("Processing order: ID=%ld, Symbol=%s, Type=%d, Side=%d", 
             order->order_id, order->symbol, order->type, order->side);
    
    // Validate order
    if (validate_order(order) != SUCCESS) {
        LOG_ERROR("Order validation failed for ID=%ld", order->order_id);
        return;
    }
    
    // Process based on order type
    switch (order->type) {
        case MARKET:
            LOG_DEBUG("Processing market order for %s", order->symbol);
            process_market_order(order);
            break;
            
        case LIMIT:
            LOG_DEBUG("Processing limit order for %s at price %.2f", 
                     order->symbol, order->price);
            process_limit_order(order);
            break;
            
        case STOP:
            LOG_DEBUG("Processing stop order for %s at price %.2f", 
                     order->symbol, order->price);
            process_stop_order(order);
            break;
            
        default:
            LOG_ERROR("Unknown order type %d for order ID=%ld", 
                     order->type, order->order_id);
            return;
    }
    
    LOG_PERF_END(process_order);
}

void broadcast_market_data(const MarketData* market_data) {
    LOG_PERF_START(broadcast_market_data);
    
    LOG_INFO("Broadcasting market data for %s: Last=%.2f, Bid=%.2f, Ask=%.2f", 
             market_data->symbol, market_data->last_price, 
             market_data->bid, market_data->ask);
    
    char buffer[BUFFER_SIZE];
    Message msg;
    msg.type = MARKET_DATA;
    msg.data.market_data = *market_data;
    
    size_t msg_size = serialize_message(&msg, buffer, BUFFER_SIZE);
    if (msg_size <= 0) {
        LOG_ERROR("Failed to serialize market data for broadcast");
        return;
    }
    
    LOG_DEBUG("Serialized market data message size: %zu bytes", msg_size);
    
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < client_count; i++) {
        LOG_DEBUG("Sending market data to client %s", clients[i].id);
        if (send(clients[i].socket, buffer, msg_size, 0) < 0) {
            LOG_ERROR("Failed to send market data to client %s: %s", 
                     clients[i].id, strerror(errno));
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
    
    LOG_DEBUG("Market data broadcast complete to %d clients", client_count);
    LOG_PERF_END(broadcast_market_data);
}

void process_trade_execution(const TradeExecution* trade) {
    LOG_PERF_START(process_trade);
    
    LOG_INFO("Processing trade execution: Trade ID=%ld, Order ID=%ld, Symbol=%s, Price=%.2f, Quantity=%d", 
             trade->trade_id, trade->order_id, trade->symbol, 
             trade->price, trade->quantity);
    
    // Update order book
    if (update_order_book(trade) != SUCCESS) {
        LOG_ERROR("Failed to update order book for trade ID=%ld", trade->trade_id);
        return;
    }
    
    // Generate market data update
    MarketData market_data;
    if (generate_market_data_update(trade, &market_data) == SUCCESS) {
        LOG_DEBUG("Broadcasting market data update after trade");
        broadcast_market_data(&market_data);
    } else {
        LOG_ERROR("Failed to generate market data update for trade ID=%ld", 
                 trade->trade_id);
    }
    
    // Notify clients involved in the trade
    notify_trade_parties(trade);
    
    LOG_PERF_END(process_trade);
}

int validate_order(const Order* order) {
    LOG_DEBUG("Validating order ID=%ld", order->order_id);
    
    if (strlen(order->symbol) == 0 || strlen(order->symbol) > MAX_SYMBOL_LENGTH) {
        LOG_ERROR("Invalid symbol length for order ID=%ld", order->order_id);
        return ERROR_INVALID_SYMBOL;
    }
    
    if (order->quantity <= 0) {
        LOG_ERROR("Invalid quantity %d for order ID=%ld", 
                 order->quantity, order->order_id);
        return ERROR_INVALID_
