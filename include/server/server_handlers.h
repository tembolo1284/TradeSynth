//include/server/server_handlers.h
#ifndef TRADESYNTH_SERVER_HANDLERS_H
#define TRADESYNTH_SERVER_HANDLERS_H

#include "common/types.h"
#include "server/server_types.h"

// Generic message handling function
int handle_message(ServerContext* context, int client_socket, const Message* msg);
uint64_t generate_order_id(ServerContext* context);
uint64_t generate_trade_id(ServerContext* context);

// Message type handlers
int handle_heartbeat(ServerContext* context, int client_socket, const Message* msg);
int handle_order_message(ServerContext* context, int client_socket, const Message* msg);
int handle_market_data(ServerContext* context, int client_socket, const Message* msg);
int handle_trade_exec(ServerContext* context, int client_socket, const Message* msg);
int handle_error(ServerContext* context, int client_socket, const Message* msg);

// Core processing functions
int process_order(ServerContext* context, const Order* order);
int broadcast_market_data(ServerContext* context, const MarketData* market_data);
int process_trade_execution(ServerContext* context, const TradeExecution* trade);

// Helper functions
int send_response_message(int client_socket, const Message* response);

#endif // TRADESYNTH_SERVER_HANDLERS_H
