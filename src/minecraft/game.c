#include "game.h"

#include <minecraft/protocol/packet_arena.h>

#include <sys/timerfd.h>
#include <sys/epoll.h>

#include <threads.h>
#include <stdbool.h>

/**
 * The game loop thread
 */
static thrd_t m_game_loop_thread;

packet_arena_t* g_current_tick_arena = NULL;

static err_t safe_sleep(long millis) {
    err_t err = NO_ERROR;

    if (millis < 0) {
        WARN("We are lagging!");
        goto cleanup;
    }

    // calculate the initial time to sleep for
    struct timespec req = {
        .tv_sec = millis / 1000,
        .tv_nsec = (millis - (millis / 1000)) * 1000000L
    };

    // sleep and ignore any interrupt by a signal
    while ((nanosleep(&req, &req)) != 0) {
        CHECK_ERRNO(errno == EINTR);
    }

cleanup:
    return err;
}

/**
 * The actual game loop, this mainly manages the time and dispatches the initial
 * work that needs to happen every game tick
 */
static int game_loop_thread(void* arg) {
    err_t err = NO_ERROR;

    // for counting tps
    time_t start = time(NULL);
    int tps = 0;

    // now just do stuff
    volatile bool lol = true;
    while (lol) {
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // TICK START
        struct timespec tick_start;
        CHECK_ERRNO(clock_gettime(CLOCK_MONOTONIC, &tick_start) == 0);

        // increment tps for fun and profit
        tps++;

        // switch the arenas
        g_current_tick_arena = switch_packet_arenas();

        struct timespec tick_end;
        CHECK_ERRNO(clock_gettime(CLOCK_MONOTONIC, &tick_end) == 0);
        // TICK END
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // sleep for the duration we need to sleep
        long tick_duration = (tick_end.tv_sec - tick_start.tv_sec) * 1000 +
                                (tick_end.tv_nsec - tick_start.tv_nsec) / 1000000000;
        CHECK_AND_RETHROW(safe_sleep((1000 / 20) - tick_duration));

        // ticks per second for fun and profit
        if (time(NULL) - start >= 1) {
            TRACE("Current TPS: %d", tps);
            start = time(NULL);
            tps = 0;
        }
    }

cleanup:
    return err;
}

err_t start_game_loop() {
    err_t err = NO_ERROR;

    TRACE("Starting game loop");
    CHECK_ERRNO(thrd_create(&m_game_loop_thread, game_loop_thread, NULL) == 0);

cleanup:
    return err;
}
