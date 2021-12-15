#include "server.h"
#include "client.h"
#include "buffer_pool.h"

#include <netinet/in.h>
#include <strings.h>
#include <lib/stb_ds.h>
#include <net/receiver.h>
#include <sync/spin_lock.h>

typedef enum request_type {
    REQUEST_ACCEPT,
    REQUEST_RECV,
    REQUEST_SEND
} request_type_t;

typedef struct request {
    // the type of the request
    request_type_t type;

    union {
        struct {
            client_t* client;
        } recv;

        struct {
            // the client that is sending
            client_t* client;

            // for sending we have a bit more
            // data for a nicer send state
            struct iovec vecs[3];
            size_t vecs_count;

            // and additionally we have a data
            // for the varints that need to be sent
            uint8_t varint_temp[5 * 2];
        } send;

        struct {
            // the address of the accepted socket
            struct sockaddr client_addr;
            socklen_t client_addr_len;
        } accept;
    };
} request_t;

/**
 * The default server config, if one is not provided this is used
 */
static server_config_t m_default_config = {
    .port = 25565,
    .max_connections = 4096,
    .max_server_list_pending = 512,
    .recv_buffer_size = 4096,
    .max_recv_packet_size = 65536,
    .max_send_packet_size = 65536,
};

/**
 * The server socket
 */
static int m_server_socket = -1;

/**
 * IS the server running currently
 */
static bool m_running = false;

/**
 * The io uring
 */
static struct io_uring m_ring = { 0 };

/**
 * The network clients
 */
static list_t m_clients = INIT_LIST(&m_clients);

server_config_t g_server_config = { 0 };

/**
 * A pool of requests that can be used for submitting
 * stuff to the io uring
 */
static request_t** m_requests_pool = NULL;

/**
 * Guards against concurrent sends
 */
static spin_lock_t m_request_lock;

err_t init_server(server_config_t* config) {
    err_t err = NO_ERROR;
    int enable = 1;

    if (config == NULL) {
        config = &m_default_config;
    }

    // create the server socket
    m_server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    CHECK_ERRNO(m_server_socket >= 0);
    CHECK_ERRNO(0 == setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)));
    struct sockaddr_in server_address = {
        .sin_family = AF_INET,
        .sin_port = htons(config->port),
        .sin_addr = {
            .s_addr = htons(INADDR_ANY)
        }
    };
    CHECK_ERRNO(0 == bind(m_server_socket, (const struct sockaddr *)&server_address, sizeof(server_address)));
    CHECK_ERRNO(0 == listen(m_server_socket, config->max_server_list_pending));

    // setup the io uring
    CHECK_ERRNO(0 == io_uring_queue_init(config->max_connections + 1, &m_ring, 0));

    // set the config
    g_server_config = *config;

cleanup:
    if (IS_ERROR(err)) {
        SAFE_CLOSE(m_server_socket);
    }
    return err;
}

static request_t* get_request() {
    request_t* req = NULL;

    spin_lock_enter(&m_request_lock);
    if (arrlen(m_requests_pool) == 0) {
        req = calloc(1, sizeof(request_t));
    } else {
        req = arrpop(m_requests_pool);
    }
    spin_lock_leave(&m_request_lock);

    return req;
}

static err_t add_accept() {
    err_t err = NO_ERROR;

    // setup the request
    request_t* request = get_request();
    CHECK_ERRNO(request != NULL);
    request->type = REQUEST_ACCEPT;
    request->accept.client_addr_len = sizeof(request->accept.client_addr);

    // get an sqe
    struct io_uring_sqe* sqe = io_uring_get_sqe(&m_ring);
    CHECK_ERRNO(sqe != NULL);

    // setup the sqe for the next accept
    io_uring_prep_accept(sqe, m_server_socket, &request->accept.client_addr, &request->accept.client_addr_len, 0);
    io_uring_sqe_set_flags(sqe, 0);
    sqe->user_data = (uint64_t)request;

cleanup:
    return err;
}

static err_t add_recv(client_t* client) {
    err_t err = NO_ERROR;

    // setup the request
    request_t* request = get_request();
    CHECK_ERRNO(request != NULL);
    request->type = REQUEST_RECV;
    request->recv.client = client;

    // get an sqe
    struct io_uring_sqe* sqe = io_uring_get_sqe(&m_ring);
    CHECK_ERRNO(sqe != NULL);

    // setup the sqe for recv on the client
    io_uring_prep_recv(sqe, client->socket, client->recv_buffer, g_server_config.recv_buffer_size, 0);
    io_uring_sqe_set_flags(sqe, 0);
    sqe->user_data = (uint64_t)request;

cleanup:
    return err;
}

static void disconnect_client(client_t* client) {
    // remove from the active clients
    list_del(&client->node);

    // log it
    TRACE("Client disconnect from: %d.%d.%d.%d:%d",
          client->address.sin_addr.s_addr & 0xFF, (client->address.sin_addr.s_addr >> 8) & 0xFF,
          (client->address.sin_addr.s_addr >> 16) & 0xFF, (client->address.sin_addr.s_addr >> 24) & 0xFF,
          ntohs(client->address.sin_port));

    // close the socket
    buffer_pool_return_tcp_recv(client->recv_buffer);
    shutdown(client->socket, SHUT_RDWR);
    client->socket = -1;

    // TODO: notify that the client has disconnected

    // we can safely free the client now
    free(client);
}

err_t server_send_packet(client_t* client, uint8_t* buffer, int32_t size) {
    err_t err = NO_ERROR;

    // setup the request
    request_t* request = get_request();
    CHECK_ERRNO(request != NULL);
    request->type = REQUEST_SEND;
    request->send.client = client;

    // TODO: compression support
    if (client->receiver_state.compression) {
        CHECK_FAIL("TODO: compression support");
    } else {
        // serialize the packet length
        int length_len = protocol_write_varint(request->send.varint_temp, 5, size);
        CHECK(length_len > 0);

        // set the length base
        request->send.vecs[0].iov_base = request->send.varint_temp;
        request->send.vecs[0].iov_len = length_len;

        // set the buffer base
        request->send.vecs[1].iov_base = buffer;
        request->send.vecs[1].iov_len = size;

        // we have two vecs
        request->send.vecs_count = 2;
    }

    // get an sqe
    struct io_uring_sqe* sqe = io_uring_get_sqe(&m_ring);
    CHECK_ERRNO(sqe != NULL);

    // setup the sqe for recv on the client
    io_uring_prep_writev(sqe, client->socket, request->send.vecs, request->send.vecs_count, 0);
    io_uring_sqe_set_flags(sqe, 0);
    sqe->user_data = (uint64_t)request;

cleanup:
    return err;
}

err_t server_start() {
    err_t err = NO_ERROR;

    CHECK(m_server_socket != -1);

    // the server can run now
    m_running = true;

    // add an accept
    add_accept();

    // wait for a max of all events at the same time
    while (m_running) {
        // pull an event from the ring
        io_uring_submit_and_wait(&m_ring, 1);

        size_t count = 0;
        size_t head = 0;
        struct io_uring_cqe* cqe = NULL;
        io_uring_for_each_cqe(&m_ring, head, cqe) {
            count++;

            // get the client
            request_t* request = (request_t*)cqe->user_data;
            CHECK(request != NULL);

            switch (request->type) {
                case REQUEST_ACCEPT: {
                    // make sure we got no error
                    CHECK_ERROR(cqe->res >= 0, -cqe->res, "Got error accepting");

                    // TODO: ipv6 support
                    struct sockaddr_in* addr = (struct sockaddr_in*)&request->accept.client_addr;
                    CHECK(addr->sin_family == AF_INET);
                    TRACE("New connection from: %d.%d.%d.%d:%d",
                          addr->sin_addr.s_addr & 0xFF, (addr->sin_addr.s_addr >> 8) & 0xFF,
                          (addr->sin_addr.s_addr >> 16) & 0xFF, (addr->sin_addr.s_addr >> 24) & 0xFF,
                          ntohs(addr->sin_port));

                    // an accept has finished, get a new client and set it up, accept should
                    // never give us an error, if it does then error out
                    client_t* new_client = calloc(1, sizeof(client_t));
                    new_client->address = *addr;
                    new_client->socket = cqe->res;
                    new_client->recv_buffer = buffer_pool_get_tcp_recv();
                    list_add_tail(&m_clients, &new_client->node);

                    // add pending recv and accept
                    CHECK_AND_RETHROW(add_recv(new_client));
                    CHECK_AND_RETHROW(add_accept());
                } break;

                case REQUEST_RECV: {
                    client_t* client = request->recv.client;
                    if (cqe->res <= 0) {
                        // disconnected
                        disconnect_client(client);
                    } else {
                        // we got data from socket
                        err = receiver_consume_data(client, request->recv.client->recv_buffer, cqe->res);
                        if (err == ERROR_PROTOCOL) {
                            // we got an error at the protocol level, not really
                            // important, force disconnect the client
                            disconnect_client(client);
                        } else {
                            // make sure we did not get any other error
                            CHECK_AND_RETHROW(err);

                            // re-add the recv
                            add_recv(client);
                        }
                    }
                } break;

                case REQUEST_SEND: {
                    client_t* client = request->send.client;
                    if (cqe->res <= 0) {
                        // disconnected
                        disconnect_client(client);
                    } else {
                        // the send is done, return the data used for actually
                        // sending the data
                        buffer_pool_return_protocol_send(request->send.vecs[request->send.vecs_count - 1].iov_base);
                    }
                } break;
            }

            // return the request to the pool until the next one is needed
            spin_lock_enter(&m_request_lock);
            arrpush(m_requests_pool, request);
            spin_lock_leave(&m_request_lock);
        }
        io_uring_cq_advance(&m_ring, count);
    }

cleanup:
    m_running = false;
    return err;
}

