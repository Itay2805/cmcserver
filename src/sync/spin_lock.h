#pragma once

#include <stdalign.h>
#include <stddef.h>

typedef struct spin_lock {
    alignas(64) size_t locked;
} spin_lock_t;

void spin_lock_enter(spin_lock_t* lock);

void spin_lock_leave(spin_lock_t* lock);
