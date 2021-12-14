#include "ticket_lock.h"

#include <stdatomic.h>

void ticket_lock_enter(ticket_lock_t* lock) {
    const size_t ticket = atomic_fetch_add_explicit(&lock->next_ticket, 1, memory_order_relaxed);
    while (atomic_load_explicit(&lock->now_serving, memory_order_acquire) != ticket) {
        __builtin_ia32_pause();
    }
}

void ticket_lock_leave(ticket_lock_t* lock) {
    const size_t new = atomic_load_explicit(&lock->now_serving, memory_order_relaxed) + 1;
    atomic_store_explicit(&lock->now_serving, new, memory_order_release);
}
