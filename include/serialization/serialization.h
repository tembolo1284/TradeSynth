#ifndef TRADESYNTH_SERIALIZATION_H
#define TRADESYNTH_SERIALIZATION_H

#include "common/types.h"
#include "common/logger.h"
#include <stdint.h>
#include <string.h>
#include <endian.h>

#define SERIALIZATION_VERSION 1
#define MAX_MESSAGE_SIZE 8192

typedef enum {
    SERIAL_SUCCESS = 0,
    SERIAL_ERROR_BUFFER_OVERFLOW = -1,
    SERIAL_ERROR_INVALID_VERSION = -2,
    SERIAL_ERROR_INVALID_TYPE = -3,
    SERIAL_ERROR_CHECKSUM = -4,
    SERIAL_ERROR_INCOMPLETE = -5,
    SERIAL_ERROR_INVALID_MESSAGE = -6
} SerializationError;

typedef struct {
    uint32_t version;
    uint32_t message_size;
    MessageType type;
    uint32_t payload_size;
    uint32_t checksum;
} MessageHeader;

// Function declarations
int serialize_message(const Message* msg, uint8_t* buffer, size_t buffer_size);
int deserialize_message(const uint8_t* buffer, size_t buffer_size, Message* msg);
uint32_t calculate_checksum(const uint8_t* data, size_t size);
int validate_message_header(const MessageHeader* header);
const char* get_serialization_error(SerializationError error);

#endif // TRADESYNTH_SERIALIZATION_H
