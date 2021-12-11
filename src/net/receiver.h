#pragma once

#include <minecraft/protocol/protocol.h>
#include <lib/except.h>
#include <stdbool.h>

typedef struct receiver_state {
    // the fetch state machine
    size_t left_to_read;

    // varint fetching
    int line;
    struct {
        uint8_t current_byte;
        uint8_t length;
    } varint;

    // the packet data
    int packet_length;
    uint8_t* packet;
    bool should_return;

    // the phase of the protocol (decides which state machine
    // we are going through)
    bool compression;
    bool encryption;
} receiver_state_t;

struct client;

err_t receiver_consume_data(struct client* receiver_state, uint8_t* data, size_t len);
