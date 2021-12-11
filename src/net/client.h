#pragma once

#include <net/receiver.h>
#include <lib/list.h>
#include <netinet/in.h>

typedef struct client {
    /**
     * The node of the client
     */
    list_node_t node;

    /**
     * The socket of the client
     */
    int socket;
    struct sockaddr_in address;

    /**
     * The state of the protocol, used for properly
     * consuming bytes as they arrive
     */
    receiver_state_t receiver_state;

    /**
     * The current state in the protocol
     */
    protocol_state_t state;

    /**
     * The buffer used for recv
     */
    uint8_t* recv_buffer;
} client_t;
