#ifndef TRADESYNTH_SERVER_H
#define TRADESYNTH_SERVER_H

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
#include "server/server_types.h"
#include "server/server_core.h"
#include "server/server_network.h"
#include "server/server_handlers.h"

// Shared extern declaration for server running flag
extern volatile sig_atomic_t server_running;

#endif // TRADESYNTH_SERVER_H
