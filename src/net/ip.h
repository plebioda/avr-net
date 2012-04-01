/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _IP_H
#define _IP_H

#include <stdint.h>

#include <avr/pgmspace.h>

#include "ethernet.h"

#define IP_PROTOCOL_ICMP	1
#define IP_PROTOCOL_UDP		17
#define IP_PROTOCOL_TCP		6

typedef uint8_t ip_address[4];

struct ip_header;


void ip_init(const ip_address * addr,const ip_address * netmask,const ip_address * gateway);
const ip_address * ip_get_addr(void);
const ip_address * ip_get_broadcast(void);

uint8_t ip_send_packet(const ip_address * ip_dst,uint8_t protocol,uint16_t length);
uint8_t ip_handle_packet(struct ip_header * header, uint16_t packet_len,const ethernet_address * mac );


#define ip_get_buffer() (ethernet_get_buffer() + NET_HEADER_SIZE_IP)
#define ip_get_buffer_size() (ethernet_get_buffer_size() - NET_HEADER_SIZE_IP)

#endif //_IP_H


