#include "serialization/serialization.h"
#include "common/logger.h"

int serialize_message(const Message* msg, char* buffer, size_t buffer_size) {
    LOG_PERF_START(serialize);
    size_t pos = 0;
    
    LOG_DEBUG("Serializing message type %d", msg->type);
    
    // Write message type
    if (pos + sizeof(MessageType) > buffer_size) {
        LOG_ERROR("Buffer overflow while serializing message type");
        return -1;
    }
    memcpy(buffer + pos, &msg->type, sizeof(MessageType));
    pos += sizeof(MessageType);
    
    // Write message data based on type
    switch (msg->type) {
        case ORDER_NEW:
        case ORDER_MODIFY:
        case ORDER_CANCEL:
            LOG_DEBUG("Serializing order: ID=%ld, Symbol=%s", 
                     msg->data.order.order_id, msg->data.order.symbol);
            if (pos + sizeof(Order) > buffer_size) {
                LOG_ERROR("Buffer overflow while serializing order");
                return -1;
            }
            memcpy(buffer + pos, &msg->data.order, sizeof(Order));
            pos += sizeof(Order);
            break;
            
        case MARKET_DATA:
            LOG_DEBUG("Serializing market data for symbol %s", 
                     msg->data.market_data.symbol);
            if (pos + sizeof(MarketData) > buffer_size) {
                LOG_ERROR("Buffer overflow while serializing market data");
                return -1;
            }
            memcpy(buffer + pos, &msg->data.market_data, sizeof(MarketData));
            pos += sizeof(MarketData);
            break;
            
        case TRADE_EXECUTION:
            LOG_DEBUG("Serializing trade execution: ID=%ld", 
                     msg->data.trade.trade_id);
            if (pos + sizeof(TradeExecution) > buffer_size) {
                LOG_ERROR("Buffer overflow while serializing trade execution");
                return -1;
            }
            memcpy(buffer + pos, &msg->data.trade, sizeof(TradeExecution));
            pos += sizeof(TradeExecution);
            break;
            
        default:
            LOG_ERROR("Unknown message type: %d", msg->type);
            return -1;
    }
    
    LOG_DEBUG("Serialization complete, total size: %zu bytes", pos);
    LOG_PERF_END(serialize);
    return pos;
}

int deserialize_message(const char* buffer, size_t buffer_size, Message* msg) {
    LOG_PERF_START(deserialize);
    size_t pos = 0;
    
    // Read message type
    if (pos + sizeof(MessageType) > buffer_size) {
        LOG_ERROR("Buffer underflow while reading message type");
        return -1;
    }
    memcpy(&msg->type, buffer + pos, sizeof(MessageType));
    pos += sizeof(MessageType);
    
    LOG_DEBUG("Deserializing message type %d", msg->type);
    
    // Read message data based on type
    switch (msg->type) {
        case ORDER_NEW:
        case ORDER_MODIFY:
        case ORDER_CANCEL:
            if (pos + sizeof(Order) > buffer_size) {
                LOG_ERROR("Buffer underflow while reading order data");
                return -1;
            }
            memcpy(&msg->data.order, buffer + pos, sizeof(Order));
            LOG_DEBUG("Deserialized order: ID=%ld, Symbol=%s", 
                     msg->data.order.order_id, msg->data.order.symbol);
            pos += sizeof(Order);
            break;
            
        case MARKET_DATA:
            if (pos + sizeof(MarketData) > buffer_size) {
                LOG_ERROR("Buffer underflow while reading market data");
                return -1;
            }
            memcpy(&msg->data.market_data, buffer + pos, sizeof(MarketData));
            LOG_DEBUG("Deserialized market data for symbol %s", 
                     msg->data.market_data.symbol);
            pos += sizeof(MarketData);
            break;
            
        case TRADE_EXECUTION:
            if (pos + sizeof(TradeExecution) > buffer_size) {
                LOG_ERROR("Buffer underflow while reading trade execution");
                return -1;
            }
            memcpy(&msg->data.trade, buffer + pos, sizeof(TradeExecution));
            LOG_DEBUG("Deserialized trade execution: ID=%ld", 
                     msg->data.trade.trade_id);
            pos += sizeof(TradeExecution);
            break;
            
        default:
            LOG_ERROR("Unknown message type during deserialization: %d", msg->type);
            return -1;
    }
    
    LOG_DEBUG("Deserialization complete, processed %zu bytes", pos);
    LOG_PERF_END(deserialize);
    return SUCCESS;
}
