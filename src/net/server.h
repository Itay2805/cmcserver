#pragma once

#include <lib/except.h>

#include <liburing.h>
#include <threads.h>

typedef struct server_config {
    /**
     * The port number of the server
     */
    uint16_t port;

    /**
     * The max amount of concurrent player connections, these
     * are for actual players playing the game
     */
    uint16_t max_connections;

    /**
     * The max amount of players that can be pending for a
     * connection in the server list, if this is too low it
     * can cause players to get a connection refused when
     * trying to ping the server or connect.
     */
    uint16_t max_server_list_pending;

    /**
     * The data size used for the tcp recv, this is much lower than the max recv packet
     * size because tcp packets usually only transmit low amount of data, so no need to use
     * large buffers for this part
     */
    size_t recv_buffer_size;

    /**
     * This is the max packet size for
     */
    size_t max_recv_packet_size;
} server_config_t;

/**
 * The current server config
 */
extern server_config_t g_server_config;

err_t init_server(server_config_t* config);

err_t server_start();
