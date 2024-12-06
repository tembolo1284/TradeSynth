// tests/unit/test_serialization.c
#include <criterion/criterion.h>
#include "../../include/serialization/serialization.h"

Test(serialization, message_serialization) {
   Message msg = {
       .type = MSG_ORDER_NEW,
       .sequence_num = 1,
       .timestamp = time(NULL),
       .data.order = {
           .order_id = 12345,
           .type = ORDER_TYPE_LIMIT,
           .side = ORDER_SIDE_BUY,
           .status = ORDER_STATUS_NEW,
           .price = double_to_price(100.50),
           .quantity = 100
       }
   };
   strncpy(msg.data.order.symbol, "AAPL", MAX_SYMBOL_LENGTH);
   
   uint8_t buffer[BUFFER_SIZE];
   ssize_t size = serialize_message(&msg, buffer, BUFFER_SIZE);
   cr_assert(size > 0, "Serialization failed");
   
   Message decoded;
   ssize_t decode_size = deserialize_message(buffer, size, &decoded);
   cr_assert_eq(decode_size, size, "Deserialization size mismatch");
   cr_assert_eq(decoded.type, msg.type, "Message type mismatch");
   cr_assert_eq(decoded.sequence_num, msg.sequence_num, "Sequence number mismatch");
   cr_assert_eq(decoded.data.order.order_id, msg.data.order.order_id, "Order ID mismatch");
   cr_assert_eq(decoded.data.order.type, msg.data.order.type, "Order type mismatch");
   cr_assert_eq(decoded.data.order.side, msg.data.order.side, "Order side mismatch");
   cr_assert_eq(decoded.data.order.price.mantissa, msg.data.order.price.mantissa, "Price mismatch");
   cr_assert_str_eq(decoded.data.order.symbol, msg.data.order.symbol, "Symbol mismatch");
}
