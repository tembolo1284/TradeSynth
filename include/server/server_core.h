#ifndef TRADESYNTH_SERVER_CORE_H
#define TRADESYNTH_SERVER_CORE_H

// Core server functions
void signal_handler(int signum);
int validate_server_config(const ServerConfig* config);
ServerContext* initialize_server_context(const ServerConfig* config);
int start_server(ServerContext* context);
void stop_server(ServerContext* context);
void cleanup_server(ServerContext* context);

#endif // TRADESYNTH_SERVER_CORE_H
