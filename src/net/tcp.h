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

typedef int8_t tcp_socket_t;
typedef void (*tcp_socket_callback)(tcp_socket_t socket,uint8_t * data, uint16_t length);

struct tcp_header;

uint8_t tcp_init(void);
uint8_t tcp_handle_packet(const ip_address * ip_remote,const struct tcp_header * tcp,uint16_t length);


#endif //_TCP_H