/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* for memset, memcpy,...*/
#include <string.h>	

#include "ethernet.h"
#include "hal.h"

struct ethernet_header
{
	ethernet_addr 	dst;
	ethernet_addr 	src;
	uint16_t 	type;
};

static ethernet_address ethernet_mac;

uint8_t 	ethernet_tx_buffer[ETHERNET_MAX_PACKET_SIZE + NET_HEADER_SIZE_ETHERNET];
uint8_t 	ethernet_rx_buffer[ETHERNET_MAX_PACKET_SIZE + NET_HEADER_SIZE_ETHERNET];

void ethernet_init(ethernet_address * mac)
{
	memset(ethernet_mac,1,sizeof(ethernet_mac));
	if(mac)
		memcpy(&ethernet_mac,mac,sizeof(ethernet_mac));
}

const ethernet_address * ethernet_get_mac()
{
	return &eth_mac;
}

uint8_t ethernet_handle_packet()
{
	/* receive packet */
	uint16_t packet_size = hal_receive_packet(ethernet_rx_buffer,sizeof(ethernet_rx_buffer));

	/* get ethernet header */
	struct ethernet_header * header = (struct ethernet_header*)ethernet_rx_buffer;
	
	/* remove ethernet header from packet for upper layers */
	packet_size -= sizeof(*header);
	uint8_t * data = (uint8_t*)(header+1);
	
	/* redirect packet to the proper upper layer */
	switch(header->type)
	{
		case HTON16(ETHERNET_TYPE_IP):
			ip_handle_packet((struct ip_header*)data,packet);
			break;
		case HTON16(ETHERNET_TYPE_ARP):
			arp_handle_packet();
			break;
		default:
			return 0;
	}
	return 1;
}

uint8_t ethernet_send_packet(ethernet_address * dst,uint16_t type,uint16_t len)
{
	if(len > ETHERNET_MAX_PACKET_SIZE -NET_HEADER_SIZE_ETHERNET)
		return 0;	
	struct ethernet_header * header = (struct ethernet_header*)ethernet_tx_buffer;
	memcpy(&header->dst,dst,sizeof(ethernet_addr));
	memcpy(&header->src,&ethernet_mac,sizeof(ethernet_addr));
	header->type = hton16(type);
	return hal_send_packet(ethernet_tx_buffer,(len + NET_HEADER_SIZE_ETHERNET));			
}


