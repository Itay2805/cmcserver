#include "buffer_pool.h"

#include "server.h"

#include <sys/mman.h>
#include <stddef.h>
#include <lib/stb_ds.h>

typedef struct free_buffer {
    struct free_buffer* next;
} free_buffer_t;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Recv is only done at a single path, so this can be lockless
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void** m_protocol_recv_buffers = NULL;

void* buffer_pool_get_protocol_recv() {
    if (arrlen(m_protocol_recv_buffers) == 0) {
        // we have no free mappings, just allocate a new one
        void* ptr = mmap(NULL, g_server_config.max_recv_packet_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED) {
            return NULL;
        } else {
            return ptr;
        }
    }

    // we have a mapping free already, reuse it
    void* buffer = arrpop(m_protocol_recv_buffers);

    // tell the kernel we are going to use this very soon, so prepare
    // the page range
    madvise(buffer, g_server_config.max_recv_packet_size, MADV_WILLNEED);

    return buffer;
}

void buffer_pool_return_protocol_recv(void* buffer) {
    // just queue it
    arrpush(m_protocol_recv_buffers, buffer);

    // tell the kernel we are no longer using this page range and it can
    // free it should there be memory preasure
    madvise(buffer, g_server_config.max_recv_packet_size, MADV_FREE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Recv is only done at a single path, so this can be lockless
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void** m_tcp_recv = NULL;

void* buffer_pool_get_tcp_recv() {
    if (arrlen(m_tcp_recv) == 0) {
        // we have no free mappings, just allocate a new one
        void* ptr = mmap(NULL, g_server_config.recv_buffer_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED) {
            return NULL;
        } else {
            return ptr;
        }
    }

    // we have a mapping free already, reuse it
    void* buffer = arrpop(m_tcp_recv);

    // tell the kernel we are going to use this very soon, so prepare
    // the page range
    madvise(buffer, g_server_config.recv_buffer_size, MADV_WILLNEED);

    return buffer;
}

void buffer_pool_return_tcp_recv(void* buffer) {
    // just queue it
    arrpush(m_tcp_recv, buffer);

    // tell the kernel we are no longer using this page range and it can
    // free it should there be memory preasure
    madvise(buffer, g_server_config.recv_buffer_size, MADV_FREE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sending could be done at alot of points, so this need to be with locking
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void* buffer_pool_get_protocol_send() {
    return NULL;
}

void buffer_pool_return_protocol_send(void* buffer) {

}
