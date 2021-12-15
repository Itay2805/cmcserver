
#include <minecraft_protodef.h>
#include <lib/except.h>

err_t process_login_packet_login_start(tick_arena_t* arena, client_t* client, login_packet_login_start_t* packet) {
    err_t err = NO_ERROR;

    

cleanup:
    return err;
}

err_t process_login_packet_login_plugin_response(tick_arena_t* arena, client_t* client, login_packet_login_plugin_response_t* packet) {
    err_t err = NO_ERROR;

    CHECK_FAIL_ERROR(ERROR_PROTOCOL, "Invalid plugin response: message_id=%d", packet->message_id);

cleanup:
    return err;
}

err_t process_login_packet_encryption_response(tick_arena_t* arena, client_t* client, login_packet_encryption_response_t* packet) {
    err_t err = NO_ERROR;

    CHECK_FAIL_ERROR(ERROR_PROTOCOL, "Invalid encryption response, no request was sent");

cleanup:
    return err;
}
