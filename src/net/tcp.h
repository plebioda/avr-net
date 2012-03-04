/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _TCP_H
#define _TCP_H

#include "tcp_config.h"
#include "ip.h"

#include <stdint.h>

enum tcp_event
{
    tcp_event_nop =0,
    tcp_event_connection_incoming,
    tcp_event_error,
    tcp_event_timeout,
    tcp_event_connection_established,
    tcp_event_reset,
    tcp_event_data_received
    
};


typedef int8_t tcp_socket_t;
typedef void (*tcp_socket_callback)(tcp_socket_t socket,enum tcp_event event);

struct tcp_header;

#define TCP_PORT_ANY	0

uint8_t tcp_init(void);
uint8_t tcp_handle_packet(const ip_address * ip_remote,const struct tcp_header * tcp,uint16_t length);

tcp_socket_t tcp_socket_alloc(tcp_socket_callback callback);
uint8_t tcp_socket_free(tcp_socket_t socket);

uint8_t tcp_listen(tcp_socket_t socket,uint16_t port);
uint8_t tcp_connect(tcp_socket_t socket,ip_address * ip,uint16_t port);
uint8_t tcp_disconnect(tcp_socket_t socket);
uint8_t tcp_accept(tcp_socket_t socket);

int16_t tcp_read(tcp_socket_t socket,uint8_t * data,uint16_t maxlen);

uint16_t tcp_get_remote_port(tcp_socket_t socket);
const ip_address * tcp_get_remote_ip(tcp_socket_t socket);

#define tcp_get_buffer_size() 	(ip_get_buffer_size() - sizeof(struct tcp_header))

#endif //_TCP_H