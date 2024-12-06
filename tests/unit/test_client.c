// tests/unit/test_client.c
#include <criterion/criterion.h>
#include "../../include/client/client.h"

static void test_connect_callback(void* user_data) {
    *(int*)user_data = 1;
}

Test(client, initialization) {
    ClientConfig config = {
        .server_port = 8080,
        .socket_timeout = DEFAULT_SOCKET_TIMEOUT,
        .reconnect_attempts = DEFAULT_RECONNECT_ATTEMPTS
    };
    strncpy(config.server_host, "localhost", sizeof(config.server_host));
    strncpy(config.client_id, "TEST_CLIENT", MAX_CLIENT_ID_LENGTH);

    ClientContext* client = initialize_client(&config, NULL, NULL);
    cr_assert_not_null(client, "Client initialization failed");
    cr_assert_eq(client->state, CLIENT_DISCONNECTED, "Incorrect initial state");
    cr_assert_eq(client->socket, -1, "Socket should be -1 when disconnected");
    cleanup_client(client);
}

Test(client, callbacks) {
    int connect_called = 0;
    ClientCallbacks callbacks = {
        .on_connect = test_connect_callback
    };

    ClientConfig config = {
        .server_port = 8080
    };
    strncpy(config.server_host, "localhost", sizeof(config.server_host));

    ClientContext* client = initialize_client(&config, &callbacks, &connect_called);
    cr_assert_not_null(client, "Client initialization failed");

    // Simulate connection callback
    client->callbacks.on_connect(client->user_data);
    cr_assert_eq(connect_called, 1, "Connect callback not called");

    cleanup_client(client);
}
