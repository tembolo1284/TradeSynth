#include "server/server.h"

int process_order(ServerContext* context, const Order* order) {
    LOG_PERF_START(process_order);
    
    if (!context || !order) {
        LOG_ERROR("Invalid parameters in process_order");
        return ERROR_INVALID_PARAM;
    }
    
    LOG_INFO("Processing order ID=%lu for symbol %s", order->order_id, order->symbol);
    
    Message response = {
        .type = MSG_ORDER_STATUS,
        .data.order = *order,
        .timestamp = time(NULL),
        .sequence_num = context->stats.messages_processed++
    };
    
    response.data.order.status = ORDER_STATUS_NEW;
    
    uint8_t buffer[BUFFER_SIZE];
    size_t msg_size = serialize_message(&response, buffer, BUFFER_SIZE);
    
    if (msg_size > 0) {
        pthread_mutex_lock(&context->clients_mutex);
        for (int i = 0; i < context->client_count; i++) {
            if (strcmp(context->clients[i].id, order->client_id) == 0) {
                if (send(context->clients[i].socket, buffer, msg_size, 0) < 0) {
                    LOG_ERROR("Failed to send order response: %s", strerror(errno));
                } else {
                    context->clients[i].messages_sent++;
                }
                break;
            }
        }
        pthread_mutex_unlock(&context->clients_mutex);
    }
    
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
        .data.market_data = *market_data,
        .timestamp = time(NULL),
        .sequence_num = context->stats.messages_processed++
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
        } else {
            context->clients[i].messages_sent++;
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
    
    Message msg = {
        .type = MSG_TRADE_EXEC,
        .data.trade = *trade,
        .timestamp = time(NULL),
        .sequence_num = context->stats.messages_processed++
    };
    
    uint8_t buffer[BUFFER_SIZE];
    size_t msg_size = serialize_message(&msg, buffer, BUFFER_SIZE);
    if (msg_size <= 0) {
        LOG_ERROR("Failed to serialize trade execution message");
        return ERROR_SERIALIZATION;
    }
    
    pthread_mutex_lock(&context->clients_mutex);
    for (int i = 0; i < context->client_count; i++) {
        if (strcmp(context->clients[i].id, trade->buyer_id) == 0 ||
            strcmp(context->clients[i].id, trade->seller_id) == 0) {
            if (send(context->clients[i].socket, buffer, msg_size, 0) < 0) {
                LOG_ERROR("Failed to send trade execution to client %s: %s", 
                         context->clients[i].id, strerror(errno));
            } else {
                context->clients[i].messages_sent++;
            }
        }
    }
    pthread_mutex_unlock(&context->clients_mutex);
    
    LOG_PERF_END(process_trade);
    return SUCCESS;
}
