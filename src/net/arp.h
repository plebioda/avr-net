/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ARP_H
#define _ARP_H

#include <stdint.h>

#include "ip.h"
#include "ethernet.h"
#include "arp_config.h"

struct arp_header;

#define ARP_HW_ADDR_TYPE_ETHERNET	0x0001
#define ARP_HW_ADDR_SIZE_ETHERNET	0x06
#define ARP_PROTO_ADDR_TYPE_IP		0x0800
#define ARP_PROTO_ADDR_SIZE_IP		0x04
#define ARP_OPERATION_REQUEST		0x0001
#define ARP_OPERATION_REPLY		0x0001


uint8_t arp_handle_packet(struct arp_header * header,uint16_t packet_length);
uint8_t arp_get_mac(ip_address * ip_addr,ethernet_address * ethernet_addr);


#endif //_ARP_H