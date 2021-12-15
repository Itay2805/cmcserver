#pragma once

#include <lib/except.h>

#include <liburing.h>
#include <threads.h>
#include "client.h"

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

    /**
     * The max size for outgoing packet
     */
    size_t max_send_packet_size;
} server_config_t;

/**
 * The current server config
 */
extern server_config_t g_server_config;

/**
 * Initialize the server with the given server config
 *
 * @param config    [IN] The config, NULL for default config
 */
err_t init_server(server_config_t* config);

/**
 * Start the server on the current thread
 */
err_t server_start();

/**
 * This sends a minecraft packet, the buffer should contain the packet id
 * and the payload but not the length.
 *
 * This function will also handle compression if needed.
 *
 * @param client    [IN] The client to send to
 * @param buffer    [IN] The buffer to send
 * @param size      [IN] The size of the buffer to send
 */
err_t server_send_packet(client_t* client, uint8_t* buffer, int32_t size);
