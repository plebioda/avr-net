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

typedef uint8_t ip_address[4];

struct ip_header;


void ip_init(ip_address * addr);
const ip_address * ip_get_addr(void);

uint8_t ip_send_packet(const ip_address * ip_dst,uint8_t protocol,uint16_t length);
uint8_t ip_handle_packet(struct ip_header * header, uint16_t packet_len);


#endif //_IP_H


