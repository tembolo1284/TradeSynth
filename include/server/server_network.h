#ifndef TRADESYNTH_SERVER_NETWORK_H
#define TRADESYNTH_SERVER_NETWORK_H

// Network handling functions
int setup_socket(ServerContext* context);
int accept_client(ServerContext* context);
void* handle_client(void* arg);
void disconnect_client(ServerContext* context, ClientConnection* client);

#endif // TRADESYNTH_SERVER_NETWORK_H
