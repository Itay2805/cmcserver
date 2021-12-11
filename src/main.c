#include <stdio.h>
#include <stdlib.h>
#include <net/server.h>
#include <string.h>
#include "lib/except.h"

int main() {
    err_t err = NO_ERROR;

    init_err_printf();
    TRACE("Starting server!");

    CHECK_AND_RETHROW(init_server(NULL));
    CHECK_AND_RETHROW(server_start());

cleanup:
    return IS_ERROR(err) ? EXIT_FAILURE : EXIT_SUCCESS;
}
