#pragma once

#include <stdalign.h>
#include <stddef.h>

typedef struct ticket_lock {
    alignas(64) size_t now_serving;
    alignas(64) size_t next_ticket;
} ticket_lock_t;

void ticket_lock_enter(ticket_lock_t* lock);

void ticket_lock_leave(ticket_lock_t* lock);
