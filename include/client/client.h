#ifndef TRADESYNTH_CLIENT_H
#define TRADESYNTH_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common/types.h"
#include "common/logger.h"
#include "serialization/serialization.h"
#include "client/client_types.h"

// Client lifecycle management
ClientContext* initialize_client(const ClientConfig* config, const ClientCallbacks* callbacks, void* user_data);
int connect_to_server(ClientContext* context);
void disconnect_from_server(ClientContext* context);
void cleanup_client(ClientContext* context);

// Message sending
int send_order(ClientContext* context, const Order* order);
int request_market_data(ClientContext* context, const char* symbol);

// Message receiving
void* message_receiver_thread(void* arg);

#endif // TRADESYNTH_CLIENT_H
