#include "spin_lock.h"

#include <stdatomic.h>
#include <stdbool.h>

static bool spin_lock_try_lock_weak(spin_lock_t* lock) {
    size_t value = false;
    return atomic_compare_exchange_weak_explicit(&lock->locked, &value, true, memory_order_acquire, memory_order_relaxed);
}

static bool spin_lock_is_locked(spin_lock_t* lock) {
    return atomic_load_explicit(&lock->locked, memory_order_relaxed);
}

void spin_lock_enter(spin_lock_t* lock) {
    while (!spin_lock_try_lock_weak(lock)) {
        while (spin_lock_is_locked(lock)) {
            __builtin_ia32_pause();
        }
    }
}

void spin_lock_leave(spin_lock_t* lock) {
    return atomic_store_explicit(&lock->locked, false, memory_order_release);
}
