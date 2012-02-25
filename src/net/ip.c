/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <string.h>

#include "ip.h"


struct ip_header
{
	union
	{
		uint8_t version;
		uint8_t header_length; 
	} vihl;	
	uint8_t 	type_of_service;
	uint16_t 	length;
	uint16_t 	id;
	union
	{
		uint16_t flags;
		uint16_t fragment_offset;
	} ffo;
	uint8_t 	ttl;
	uint8_t 	protocol;
	uint16_t 	checksum;
	ip_addr	 	src;
	ip_addr 	dst;
};

static ip_address ip_addr;


void ip_init(ip_address * addr)
{
	memset(&ip_addr,0,sizeof(ip_address));
	if(addr)
		memcpy(&ip_addr,addr,sizeof(ip_address));
}

const ip_address * ip_get_addr()
{
	return &ip_addr;
}

uint8_t ip_send_packet(const ip_addr * ip_dst,uint8_t protocol,uint16_t length)
{

}

uint8_t ip_handle_packet(struct ip_header * header, uint16_t packet_len)
{
	
}

