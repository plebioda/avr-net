/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ICMP_H
#define _ICMP_H

#include "net_config.h"

#include "ip.h"

#include <stdint.h>

struct icmp_header;

uint8_t icmp_handle_packet(const ip_address * ip_addr,const struct icmp_header * icmp,uint16_t packet_len);


#endif //_ICMP_H