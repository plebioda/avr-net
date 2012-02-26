/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _UDP_H
#define _UDP_H

#include <stdint.h>

#include "udp_config.h"
#include "ip.h"
#include "net.h"

typedef int8_t socket_t;

typedef void (*udp_socket_callback)(socket_t socket,uint8_t * data,uint16_t length);

struct udp_header;

uint8_t udp_init(void);
uint8_t udp_handle_packet(const ip_address * ip_remote,const struct udp_header * udp,uint16_t packet_len);

socket_t udp_socket_alloc(uint16_t local_port,udp_socket_callback callback);
void udp_socket_free(socket_t socket);

uint8_t udp_send(socket_t socket,uint16_t length);

#define udp_get_buffer() (ip_get_buffer() + NET_HEADER_SIZE_UDP)
#define udp_get_buffer_size() (ip_get_buffer_size() - NET_HEADER_SIZE_UDP)

#endif //_UDP_H