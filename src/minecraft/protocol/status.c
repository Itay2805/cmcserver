
#include <minecraft_protodef.h>
#include <lib/except.h>

err_t process_status_packet_ping(tick_arena_t* arena, client_t* client, status_packet_ping_t* packet) {
    err_t err = NO_ERROR;

    // send back a pong
    status_packet_pong_t pong = {
        .time = packet->time
    };
    CHECK_AND_RETHROW(send_status_packet_pong(client, &pong));

cleanup:
    return err;
}

// TODO: something proper
static char m_server_info[] =
        "{"
        "\"version\":{"
        "\"name\":\"1.18.1\","
        "\"protocol\":757"
        "},"
        "\"description\": {"
        "\"text\":\"Hello World!\""
        "}"
        "}";

err_t process_status_packet_ping_start(tick_arena_t* arena, client_t* client, status_packet_ping_start_t* packet) {
    err_t err = NO_ERROR;

    status_packet_server_info_t server_info = {
        .response = {
            .elements = m_server_info,
            .length = ARRAY_LEN(m_server_info)
        }
    };
    CHECK_AND_RETHROW(send_status_packet_server_info(client, &server_info));

cleanup:
    return err;
}

