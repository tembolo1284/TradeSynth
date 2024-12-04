#include "server/server.h"
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

volatile sig_atomic_t server_running = 1;

void signal_handler(int signum) {
    LOG_INFO("Received signal %d, initiating shutdown", signum);
    server_running = 0;
}

int validate_server_config(const ServerConfig* config) {
    if (!config) return 0;

    if (config->port <= 0 || config->port > 65535) {
        LOG_ERROR("Invalid port number: %d", config->port);
        return 0;
    }

    if (config->max_clients <= 0 || config->max_clients > MAX_CLIENTS) {
        LOG_ERROR("Invalid max clients: %d", config->max_clients);
        return 0;
    }

    if (config->socket_timeout < 0) {
        LOG_ERROR("Invalid socket timeout: %d", config->socket_timeout);
        return 0;
    }

    return 1;
}

ServerContext* initialize_server_context(const ServerConfig* config) {
    if (!validate_server_config(config)) {
        LOG_ERROR("Invalid server configuration");
        return NULL;
    }

    ServerContext* context = (ServerContext*)calloc(1, sizeof(ServerContext));
    if (!context) {
        LOG_ERROR("Failed to allocate server context");
        return NULL;
    }

    context->config = *config;
    context->state = SERVER_STOPPED;
    context->stats.start_time = time(NULL);

    pthread_mutex_init(&context->stats_mutex, NULL);
    pthread_mutex_init(&context->clients_mutex, NULL);

    context->clients = (ClientConnection*)calloc(config->max_clients, sizeof(ClientConnection));
    if (!context->clients) {
        LOG_ERROR("Failed to allocate client connection array");
        free(context);
        return NULL;
    }

    LOG_INFO("Server context initialized successfully");
    return context;
}

int start_server(ServerContext* context) {
    if (!context) {
        LOG_ERROR("Null server context");
        return ERROR_INVALID_PARAM;
    }

    LOG_INFO("Starting server on port %d", context->config.port);

    // Set up robust signal handling
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    context->server_socket = setup_socket(context);
    if (context->server_socket < 0) {
        LOG_ERROR("Failed to set up server socket");
        return context->server_socket;
    }

    // Set server socket to non-blocking mode
    int flags = fcntl(context->server_socket, F_GETFL, 0);
    fcntl(context->server_socket, F_SETFL, flags | O_NONBLOCK);

    context->state = SERVER_RUNNING;
    LOG_INFO("Server started successfully");

    // Main server loop
    while (server_running) {
        LOG_DEBUG("Server running: %d", server_running);
        int result = accept_client(context);
        if (result < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(10000); // Sleep briefly to avoid busy looping
            } else {
                LOG_ERROR("Failed to accept client connection: %s", strerror(errno));
            }
        }
    }

    stop_server(context);
    return SUCCESS;
}

void stop_server(ServerContext* context) {
    if (!context) return;

    LOG_INFO("Stopping server");
    context->state = SERVER_STOPPING;

    if (context->server_socket >= 0) {
        close(context->server_socket);
        context->server_socket = -1;
    }

    pthread_mutex_lock(&context->clients_mutex);
    for (int i = 0; i < context->client_count; i++) {
        if (context->clients[i].thread_active) {
            pthread_join(context->clients[i].thread, NULL);
            context->clients[i].thread_active = 0;
        }
        close(context->clients[i].socket);
    }
    context->client_count = 0;
    pthread_mutex_unlock(&context->clients_mutex);

    context->state = SERVER_STOPPED;
    LOG_INFO("Server stopped");
}

void cleanup_server(ServerContext* context) {
    if (!context) return;

    stop_server(context);

    pthread_mutex_destroy(&context->stats_mutex);
    pthread_mutex_destroy(&context->clients_mutex);

    free(context->clients);
    free(context);

    LOG_INFO("Server resources cleaned up");
}

