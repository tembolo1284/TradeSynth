// tests/integration/test_client_server.c
#include <criterion/criterion.h>
#include <unistd.h>
#include "../../include/server/server.h"
#include "../../include/client/client.h"

Test(integration, client_server_connection, .timeout = 5) {
    ServerConfig server_config = {
        .port = 8080,
        .max_clients = 1
    };
    ServerContext* server = initialize_server_context(&server_config);
    cr_assert_not_null(server, "Server initialization failed");
    
    int server_result = start_server(server);
    cr_assert_eq(server_result, SUCCESS, "Server start failed");
    
    usleep(100000);  // 100ms wait for server start
    
    ClientConfig client_config = {
        .server_port = 8080,
        .socket_timeout = DEFAULT_SOCKET_TIMEOUT
    };
    strncpy(client_config.server_host, "localhost", sizeof(client_config.server_host));
    strncpy(client_config.client_id, "TEST_CLIENT", MAX_CLIENT_ID_LENGTH);
    
    ClientContext* client = initialize_client(&client_config, NULL, NULL);
    cr_assert_not_null(client, "Client initialization failed");
    
    int connect_result = connect_to_server(client);
    cr_assert_eq(connect_result, SUCCESS, "Client connection failed");
    cr_assert_eq(client->state, CLIENT_CONNECTED, "Client not in connected state");
    
    cleanup_client(client);
    cleanup_server(server);
}

Test(integration, message_exchange, .timeout = 5) {
    ServerConfig server_config = {
        .port = 8080,
        .max_clients = 1
    };
    ServerContext* server = initialize_server_context(&server_config);
    cr_assert_not_null(server, "Server initialization failed");
    
    start_server(server);
    usleep(100000);

    ClientConfig client_config = {
        .server_port = 8080
    };
    strncpy(client_config.server_host, "localhost", sizeof(client_config.server_host));
    strncpy(client_config.client_id, "TEST_CLIENT", MAX_CLIENT_ID_LENGTH);

    ClientContext* client = initialize_client(&client_config, NULL, NULL);
    cr_assert_not_null(client, "Client initialization failed");
    
    connect_to_server(client);

    // Test order message
    Order order = {
        .order_id = 12345,
        .type = ORDER_TYPE_LIMIT,
        .side = ORDER_SIDE_BUY,
        .price = double_to_price(100.50),
        .quantity = 100
    };
    strncpy(order.symbol, "AAPL", MAX_SYMBOL_LENGTH);
    
    int send_result = send_order(client, &order);
    cr_assert_eq(send_result, SUCCESS, "Failed to send order");

    // Allow time for message processing
    usleep(100000);

    cleanup_client(client);
    cleanup_server(server);
}
