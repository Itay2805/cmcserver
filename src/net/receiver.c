#include "receiver.h"
#include "client.h"

#include <stdbool.h>
#include <string.h>
#include <net/server.h>
#include <net/buffer_pool.h>
#include <netinet/in.h>
#include <lib/stb_ds.h>
#include <minecraft_protodef.h>

#define FETCHER_BEGIN \
    switch (receiver_state->line) { \
        case 0:

#define FETCHER_END \
        break; \
        default: CHECK_FAIL("Got to invalid fetcher state?!"); \
    }

#define FETCHER_RETURN \
    do { \
        receiver_state->line = __LINE__; \
        goto cleanup; \
        case __LINE__: \
    } while (0)

err_t receiver_consume_data(client_t* client, uint8_t* data, size_t len) {
    err_t err = NO_ERROR;
    receiver_state_t* receiver_state = &client->receiver_state;

    // fetch all the packets we can from the stream
    while (len > 0) {
        FETCHER_BEGIN
            receiver_state->varint.length = 0;
            while (true) {
                // fetch the byte
                if (len == 0) FETCHER_RETURN;
                uint8_t current_byte = *data;
                len--;
                data++;

                // set the value
                receiver_state->packet_length |= (current_byte & 0x7F) << (receiver_state->varint.length * 7);
                receiver_state->varint.length++;

                // make sure the varint is valid
                CHECK(receiver_state->varint.length <= 5, "varint was too long");

                // check if we finished
                if ((receiver_state->varint.current_byte & 0x80) == 0) {
                    break;
                }
            }

            if (receiver_state->packet_length <= len) {
                // fast path, we can use the buffer directly because the packet
                // is small enough to fit in it
                receiver_state->packet = data;
                data += receiver_state->packet_length;
                len -= receiver_state->packet_length;
            } else {
                // slow path, we need to recv more than the amount we have right
                // now, so we need to properly handle it

                // make sure the packet is valid, if it is get a page from the range
                CHECK(receiver_state->packet_length <= g_server_config.max_recv_packet_size, "Client tried to send packet larger than supported (wanted %d)", receiver_state->packet_length);
                receiver_state->packet = buffer_pool_get_protocol_recv();
                receiver_state->should_return = true;

                // recv the packet data, we will handle the compression or
                // no compression later
                receiver_state->left_to_read = receiver_state->packet_length;
                while (true) {
                    size_t offset = receiver_state->packet_length - receiver_state->left_to_read;
                    size_t can_copy = len > receiver_state->left_to_read ? receiver_state->left_to_read : len;
                    memcpy(receiver_state->packet + offset, data, can_copy);
                    if (receiver_state->encryption) {
                        // TODO: handle encryption at this level
                        CHECK_FAIL("TODO: encryption");
                    }
                    receiver_state->left_to_read -= can_copy;
                    data += can_copy;
                    len -= can_copy;

                    // check if we need to recv more
                    if (receiver_state->left_to_read == 0) {
                        // no, exit from the loop
                        break;
                    } else {
                        // yes, continue
                        FETCHER_RETURN;
                    }
                }
            }

            // handle decompression now
            if (receiver_state->compression) {
                // TODO: now handle compression
                CHECK_FAIL("TODO: compression");
            }

            // now pass the packet for dispatching
            CHECK_AND_RETHROW(dispatch_packet(client, receiver_state->packet, receiver_state->packet_length));

            // we no longer have a use for this packet, return it if
            // it was allocated from the buffer pool
            if (receiver_state->should_return) {
                buffer_pool_return_protocol_send(receiver_state->packet);
                receiver_state->packet = NULL;
                receiver_state->should_return = false;
            }

            // we are done! reset the state and process the packet
            receiver_state->line = 0;
        FETCHER_END
    }

cleanup:
    if (IS_ERROR(err)) {
        // free any buffer we may have allocated
        if (receiver_state->should_return) {
            buffer_pool_return_protocol_send(receiver_state->packet);
            receiver_state->packet = NULL;
            receiver_state->should_return = false;
        }

        // reset the receiver state
        receiver_state->line = 0;
    }

    return err;
}
