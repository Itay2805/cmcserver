#include "minecraft_protodef.h"

#include <lib/except.h>

// TODO: parse from json
#define PROTOCOL_VERSION 757

err_t process_handshaking_packet_set_protocol(tick_arena_t* arena, client_t* client, handshaking_packet_set_protocol_t* packet) {
    err_t err = NO_ERROR;

    switch (packet->next_state) {
        case PROTOCOL_STATUS: {
            // nothing else to do here
        } break;

        case PROTOCOL_LOGIN: {
            // check the version is valid
            if (packet->protocol_version != PROTOCOL_VERSION) {
                // TODO: disconnect gracefully
                CHECK_FAIL_ERROR(ERROR_PROTOCOL, "Client with invalid protocol version: %d", packet->protocol_version);
            }
        } break;

        default: CHECK_FAIL_ERROR(ERROR_PROTOCOL, "Client sent invalid next state: %d", packet->next_state);
    }

    // advance
    client->state = packet->next_state;

cleanup:
    return err;
}
