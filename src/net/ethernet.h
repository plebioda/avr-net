/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ETHERNET_H
#define _ETHERNET_H

#include <stdint.h>
#include <avr/pgmspace.h>
#include "ethernet_config.h"
#include "net.h"

#define ETHERNET_TYPE_IP	0x0800
#define ETHERNET_TYPE_ARP	0x0806

#define ETHERNET_ADDR_BROADCAST	0

extern uint8_t ethernet_tx_buffer[];

struct ethernet_stats
{
  uint32_t rx_packets;
  uint32_t tx_packets;
};

typedef uint8_t ethernet_address[6];

void ethernet_init(const ethernet_address * mac);

const ethernet_address * ethernet_get_mac(void);
const char * ethernet_addr_str(const ethernet_address * addr);
const struct ethernet_stats * ethernet_get_stats(void);

uint8_t ethernet_handle_packet(void);
uint8_t ethernet_send_packet(ethernet_address * dst,uint16_t type,uint16_t len);

#define ethernet_get_buffer()	(&ethernet_tx_buffer[NET_HEADER_SIZE_ETHERNET])
#define ethernet_get_broadcast()
#define ethernet_get_buffer_size() (ETHERNET_MAX_PACKET_SIZE -NET_HEADER_SIZE_ETHERNET)

#endif //_ETHERNET_H



