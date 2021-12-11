#pragma once

#include <stdbool.h>

/**
 * Get a recv buffer for a packet, this is always
 * @return
 */
void* buffer_pool_get_protocol_recv();
void buffer_pool_return_protocol_recv(void* buffer);

void* buffer_pool_get_tcp_recv();
void buffer_pool_return_tcp_recv(void* buffer);

void* buffer_pool_get_protocol_send();
void buffer_pool_return_protocol_send(void* buffer);
