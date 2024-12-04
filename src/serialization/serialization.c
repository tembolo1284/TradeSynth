#include "serialization/serialization.h"

int serialize_message(const Message* msg, uint8_t* buffer, size_t buffer_size) {
    if (!msg || !buffer || buffer_size < sizeof(MessageHeader)) {
        return SERIAL_ERROR_BUFFER_OVERFLOW;
    }

    size_t pos = 0;
    MessageHeader header = {
        .version = SERIALIZATION_VERSION,
        .message_size = 0,
        .type = msg->type,
        .payload_size = 0,
        .checksum = 0
    };

    switch (msg->type) {
        case MSG_ORDER_NEW:
        case MSG_ORDER_MODIFY:
        case MSG_ORDER_CANCEL:
            header.payload_size = sizeof(Order);
            if (pos + sizeof(MessageHeader) + header.payload_size > buffer_size) {
                return SERIAL_ERROR_BUFFER_OVERFLOW;
            }
            memcpy(buffer + pos + sizeof(MessageHeader), &msg->data.order, sizeof(Order));
            break;

        case MSG_MARKET_DATA:
            header.payload_size = sizeof(MarketData);
            if (pos + sizeof(MessageHeader) + header.payload_size > buffer_size) {
                return SERIAL_ERROR_BUFFER_OVERFLOW;
            }
            memcpy(buffer + pos + sizeof(MessageHeader), &msg->data.market_data, sizeof(MarketData));
            break;

        case MSG_TRADE_EXEC:
            header.payload_size = sizeof(TradeExecution);
            if (pos + sizeof(MessageHeader) + header.payload_size > buffer_size) {
                return SERIAL_ERROR_BUFFER_OVERFLOW;
            }
            memcpy(buffer + pos + sizeof(MessageHeader), &msg->data.trade, sizeof(TradeExecution));
            break;

        default:
            return SERIAL_ERROR_INVALID_TYPE;
    }

    header.message_size = sizeof(MessageHeader) + header.payload_size;
    header.checksum = calculate_checksum(buffer + sizeof(MessageHeader), header.payload_size);

    memcpy(buffer, &header, sizeof(MessageHeader));

    return header.message_size;
}

int deserialize_message(const uint8_t* buffer, size_t buffer_size, Message* msg) {
    if (!buffer || !msg || buffer_size < sizeof(MessageHeader)) {
        return SERIAL_ERROR_INCOMPLETE;
    }

    MessageHeader header;
    memcpy(&header, buffer, sizeof(MessageHeader));

    if (validate_message_header(&header) != SERIAL_SUCCESS) {
        return SERIAL_ERROR_INVALID_MESSAGE;
    }

    if (buffer_size < header.message_size) {
        return SERIAL_ERROR_INCOMPLETE;
    }

    uint32_t calc_checksum = calculate_checksum(buffer + sizeof(MessageHeader), header.payload_size);
    if (calc_checksum != header.checksum) {
        return SERIAL_ERROR_CHECKSUM;
    }

    msg->type = header.type;
    const uint8_t* payload = buffer + sizeof(MessageHeader);

    switch (msg->type) {
        case MSG_ORDER_NEW:
        case MSG_ORDER_MODIFY:
        case MSG_ORDER_CANCEL:
            if (header.payload_size != sizeof(Order)) {
                return SERIAL_ERROR_INVALID_MESSAGE;
            }
            memcpy(&msg->data.order, payload, sizeof(Order));
            break;

        case MSG_MARKET_DATA:
            if (header.payload_size != sizeof(MarketData)) {
                return SERIAL_ERROR_INVALID_MESSAGE;
            }
            memcpy(&msg->data.market_data, payload, sizeof(MarketData));
            break;

        case MSG_TRADE_EXEC:
            if (header.payload_size != sizeof(TradeExecution)) {
                return SERIAL_ERROR_INVALID_MESSAGE;
            }
            memcpy(&msg->data.trade, payload, sizeof(TradeExecution));
            break;

        default:
            return SERIAL_ERROR_INVALID_TYPE;
    }

    return SERIAL_SUCCESS;
}

uint32_t calculate_checksum(const uint8_t* data, size_t size) {
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum = ((checksum << 5) + checksum) + data[i];
    }
    return checksum;
}

int validate_message_header(const MessageHeader* header) {
    if (!header) return SERIAL_ERROR_INVALID_MESSAGE;
    
    if (header->version != SERIALIZATION_VERSION) {
        return SERIAL_ERROR_INVALID_VERSION;
    }
    
    if (header->message_size < sizeof(MessageHeader)) {
        return SERIAL_ERROR_INVALID_MESSAGE;
    }
    
    if (header->message_size > MAX_MESSAGE_SIZE) {
        return SERIAL_ERROR_BUFFER_OVERFLOW;
    }
    
    return SERIAL_SUCCESS;
}

const char* get_serialization_error(SerializationError error) {
    switch (error) {
        case SERIAL_SUCCESS:
            return "Success";
        case SERIAL_ERROR_BUFFER_OVERFLOW:
            return "Buffer overflow";
        case SERIAL_ERROR_INVALID_VERSION:
            return "Invalid version";
        case SERIAL_ERROR_INVALID_TYPE:
            return "Invalid message type";
        case SERIAL_ERROR_CHECKSUM:
            return "Checksum mismatch";
        case SERIAL_ERROR_INCOMPLETE:
            return "Incomplete message";
        case SERIAL_ERROR_INVALID_MESSAGE:
            return "Invalid message format";
        default:
            return "Unknown error";
    }
}
