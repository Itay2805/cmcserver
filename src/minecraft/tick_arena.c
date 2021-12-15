#include "tick_arena.h"

#include <sync/ticket_lock.h>

#include <stdatomic.h>
#include <sys/mman.h>
#include <string.h>

/**
 * A lock on the current arena, so we can properly switch it
 * and sync the packet processing and the game tick threads
 */
static ticket_lock_t m_arena_lock;

/**
 * The actual arenas
 */
static tick_arena_t m_arena_1 = {0 };
static tick_arena_t m_arena_2 = {0 };

/**
 * The arena that is going to be used for allocating in the current
 * tick and the packets that are going to be set in the current tick
 */
static tick_arena_t* m_current_tick_arena;
static tick_arena_t* m_next_tick_arena;

/**
 * Initialize an arena, just allocate the buffer
 *
 * We are going to allocate a constant size of 1GB for the whole arena, should
 * be far more than enough
 *
 * @param arena [IN] The arena
 */
static err_t init_arena(tick_arena_t* arena) {
    err_t err = NO_ERROR;

    arena->start = mmap(NULL, SIZE_1GB, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    CHECK_ERRNO(arena->start != MAP_FAILED);

    arena->max_size = SIZE_1GB;

cleanup:
    return err;
}

err_t init_tick_arenas() {
    err_t err = NO_ERROR;

    TRACE("Initializing packet arenas");

    // init our two arenas
    CHECK_AND_RETHROW(init_arena(&m_arena_1));
    CHECK_AND_RETHROW(init_arena(&m_arena_2));

    // assign them
    m_current_tick_arena = &m_arena_1;
    m_next_tick_arena = &m_arena_2;

cleanup:
    return err;
}

/**
 * Reset the arena
 *
 * This will also tell the kernel that area which was not used now can be freed by the kernel
 * if so needed
 *
 * @param arena [IN] The arena
 */
static void reset_arena(tick_arena_t* arena) {
    if (arena->current_offset < arena->last_offset && arena->last_offset - arena->current_offset > SIZE_2MB) {
        // we are gonna tell the kernel we don't need the memory that is allocated above the allocation space because it
        // means we don't need it this frame
        madvise((void*)ALIGN_UP((uintptr_t)arena->start + arena->current_offset, PAGE_SIZE), ALIGN_DOWN(arena->last_offset - arena->current_offset, PAGE_SIZE), MADV_FREE);
    }

    // now reset it
    arena->last_offset = arena->current_offset;
    arena->current_offset = 0;
}

tick_arena_t* switch_tick_arenas() {
    // lock the arena so no one can use it
    ticket_lock_enter(&m_arena_lock);

    // switch the pointers
    tick_arena_t* next_tick_arena = m_next_tick_arena;
    m_next_tick_arena = m_current_tick_arena;
    m_current_tick_arena = next_tick_arena;

    // reset the arena for the new allocations
    reset_arena(m_next_tick_arena);

    // leave it
    ticket_lock_leave(&m_arena_lock);

    // now we need to wait until everyone is done using the old arena
    while (atomic_load_explicit(&m_current_tick_arena->users, memory_order_consume) != 0) {
        __builtin_ia32_pause();
    }

    // return the arena for the current tick
    return m_current_tick_arena;
}

tick_arena_t* get_tick_arena() {
    ticket_lock_enter(&m_arena_lock);
    tick_arena_t* arena = m_current_tick_arena;
    arena->users++;
    ticket_lock_leave(&m_arena_lock);
    return arena;
}

void return_tick_arena(tick_arena_t* arena) {
    atomic_fetch_add_explicit(&m_current_tick_arena->users, -1, memory_order_release);
}

void* tick_arena_alloc_unlocked(tick_arena_t* arena, size_t size) {
    if (arena->current_offset + size > arena->max_size) {
        return NULL;
    }
    void* ptr = &arena->start[arena->current_offset];
    arena->current_offset += size;
    return ptr;
}

void* tick_arena_alloc(tick_arena_t* arena, size_t size) {
    spin_lock_enter(&arena->lock);
    void* ptr = tick_arena_alloc_unlocked(arena, size);
    spin_lock_leave(&arena->lock);
    return ptr;
}
