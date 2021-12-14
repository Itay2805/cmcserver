#include "lib/except.h"

#include <minecraft/protocol/packet_arena.h>
#include <minecraft/game.h>

#include <net/server.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main() {
    err_t err = NO_ERROR;

    init_err_printf();
    TRACE("Initializing server");
    CHECK_AND_RETHROW(init_packet_arenas());
    CHECK_AND_RETHROW(start_game_loop());

    TRACE("Starting server!");
    CHECK_AND_RETHROW(init_server(NULL));
    CHECK_AND_RETHROW(server_start());

cleanup:
    return IS_ERROR(err) ? EXIT_FAILURE : EXIT_SUCCESS;
}
