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
#include <stdio.h>

#include "udp_config.h"
#include "ip.h"
#include "net.h"

typedef int8_t udp_socket_t;

typedef void (*udp_socket_callback)(udp_socket_t socket,uint8_t * data,uint16_t length);

struct udp_header;

uint8_t udp_init(void);
uint8_t udp_handle_packet(const ip_address * ip_remote,const struct udp_header * udp,uint16_t packet_len);

#define UDP_PORT_ANY	0
#define UDP_PORT_BOOTPS	67
#define UDP_PORT_BOOTPC	68


udp_socket_t udp_socket_alloc(uint16_t local_port,udp_socket_callback callback);
void udp_socket_free(udp_socket_t socket);

uint8_t udp_send(udp_socket_t socket,uint16_t length);
uint8_t udp_bind_remote(udp_socket_t socket,uint16_t remote_port,const ip_address * remote_ip);
uint8_t udp_unbind_remote(udp_socket_t socket);
uint8_t udp_bind_local(udp_socket_t socket,uint16_t local_port);

void udp_print_stat(FILE * fh);

#define udp_get_buffer() (ip_get_buffer() + NET_HEADER_SIZE_UDP)
#define udp_get_buffer_size() (ip_get_buffer_size() - NET_HEADER_SIZE_UDP)

#endif //_UDP_H