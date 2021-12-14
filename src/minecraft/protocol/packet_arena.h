#pragma once

#include <semaphore.h>
#include <stdalign.h>
#include <stdint.h>
#include <sync/spin_lock.h>
#include <lib/except.h>

typedef struct packet_arena {
    // TODO: arena related stuff
    uint8_t* start;
    size_t current_offset;
    size_t max_size;
    size_t last_offset;

    // allocation lock
    spin_lock_t lock;

    // how many people are still potentially
    // this arena, used to sync on switch
    // packet arenas
    alignas(64) size_t users;
} packet_arena_t;

/**
 * Initialize the packet arenas
 */
err_t init_packet_arenas();

/**
 * Switch between the arenas, done once a game tick
 *
 * Returns the arena to use for the current game tick
 */
packet_arena_t* switch_packet_arenas();

/**
 * Get an arena to use for the packet
 */
packet_arena_t* get_packet_arena();

/**
 * Return the arena, meaning we no longer going to allocate data from it
 *
 * @param arena [IN] The arena we got from the `get_packet_arena`
 */
void return_packet_arena(packet_arena_t* arena);

/**
 * Request data from the given arena
 *
 * @remark
 * This will not use any locks, so it is fine of only one
 * thread is going to allocate from this but not fine if more than one
 * thread is going to use it
 *
 * @param arena     [IN] The arena to allocate from
 * @param size      [IN] The size to allocate
 */
void* packet_arena_alloc_unlocked(packet_arena_t* arena, size_t size);

/**
 * Allocate data from the arena
 *
 * @param arena     [IN] The arena to allocate from
 * @param size      [IN] The size to allocate
 */
void* packet_arena_alloc(packet_arena_t* arena, size_t size);
