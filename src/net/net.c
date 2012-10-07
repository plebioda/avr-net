/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "net.h"

#define DEBUG_MODE
#include "../debug.h"

uint16_t hton16(uint16_t h)
{
	return HTON16(h);
}

uint32_t hton32(uint32_t h)
{
	return HTON32(h);
}

#define NET_ROLAND_CHECKSUM		1
uint16_t net_get_checksum(uint16_t checksum,const uint8_t * data,uint16_t len,uint8_t skip)
{
#if NET_ROLAND_CHECKSUM
	if(len < 1)
		return checksum;
	
	const uint8_t * data_skip = data + skip;
	const uint8_t * data_end = data + len-1;
	
	uint16_t temp;
	
	for(;data < data_end; data+=2)
	{
		/* skip checksum bytes within the data (for checksum plain within the data)*/
		if(data == data_skip)
			continue;
		temp = ((((uint16_t)data[0]) << 8) | (uint16_t)data[1]);
		checksum += temp;
		if(checksum < temp)
			++checksum;
	}
	/* last byte*/
	if(data == data_end)
	{
		temp = ((uint16_t)(*data_end)) << 8;
		checksum += temp;
		if(checksum < temp)
			++checksum;	
	}
	return checksum;
#else
	if(len < 1)
		return checksum;
	const uint8_t * data_skip = &data[skip];
	uint16_t temp;
	uint32_t sum=0;
	while(len > 1)
	{
		if(data != data_skip)
		{
			temp = MAKEUINT16((*data),*(data+1));
			sum += temp;
		}
		data+=2;
		len -= 2;
	}
	if(len > 0)
		sum += (*data);
	while((temp = (sum>>16)))
		sum = (sum & 0xffff) + temp;
	return (uint16_t)sum;
#endif
}
