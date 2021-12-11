#include "minecraft_protodef.h"

#include <lib/except.h>

// TODO: parse from json
#define PROTOCOL_VERSION 757

err_t process_handshaking_packet_set_protocol(client_t* client, handshaking_packet_set_protocol_t* packet) {
    err_t err = NO_ERROR;

    TRACE("Got a new client handshake:");
    TRACE("\tprotocol version: %d", packet->protocol_version);
    TRACE("\tserver: %s:%d", packet->server_host, packet->server_port);

    switch (packet->next_state) {
        case PROTOCOL_STATUS: {
            // nothing else to do here
        } break;

        case PROTOCOL_LOGIN: {
            // check the version is valid
            if (packet->protocol_version != PROTOCOL_VERSION) {
                // TODO: disconnect gracefully
                CHECK_FAIL();
            }
        } break;

        default: CHECK_FAIL();
    }

    // advance
    client->state = packet->next_state;

cleanup:
    return err;
}
