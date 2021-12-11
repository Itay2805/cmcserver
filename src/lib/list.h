#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct list_node {
    struct list_node* next;
    struct list_node* prev;
} list_node_t;

typedef list_node_t list_t;

#define INIT_LIST(list) ((list_t){ .next = (list), .prev = (list) })

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
    #define LIST_ENTRY(node_ptr, type, member) \
            ({ \
                const typeof(((type*)0)->member) *__mptr = (node_ptr); \
                (type*) ((char*)__mptr - offsetof(type, member)); \
            })
#else
    #define LIST_ENTRY(node_ptr, type, member) \
            ( \
                (type*) ((char*)(node_ptr) - offsetof(type, member)) \
            )
#endif

static inline bool list_empty(list_t* head) {
    return head->next == head;
}

static inline void list_add_tail(list_t* head, list_node_t* new_node) {
    list_node_t* prev = head;
    head->prev = new_node;
    new_node->next = head;
    new_node->prev = prev;
    prev->next = new_node;
}

static inline void list_del(list_node_t* entry) {
    list_node_t* next = entry->next;
    list_node_t* prev = entry->prev;
    next->prev = prev;
    prev->next = next;
    entry->next = (list_node_t*) 0x100;
    entry->prev = (list_node_t*) 0x200;
}
