/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "partition.h"

#include "../debug.h"

#ifdef DEBUG_MODE

void print_partition(struct partition * p)
{
	DBG_INFO("Partition:\n");
	DBG_INFO("State: %s\n",(p->state==0x80 ? "ACTIVE" : (p->state==0 ? "INACTIVE":"?")));
	DBG_INFO("Type: %d\n",(p->type));
	DBG_INFO("Offset: %d\n",(p->offset));
	DBG_INFO("Length: %d\n",(p->length));
}

#endif

#define read32(ptr)	(uint32_t)(*((uint32_t*)ptr))
#define read16(ptr)	(uint16_t)(*((uint16_t*)buff))

uint8_t	partition_open(struct partition * partition,device_read_t device_read,device_write_t device_write,uint8_t index)
{
	/* 16 bytes fo partition table entry*/
	uint8_t buff[0x10];		
	/* device_read function must be defined, number of partition entries == 4*/
	if(!partition || device_read == 0 || index>=4)
		return 1;
	partition->device_read = device_read;
	partition->device_write = device_write;
	partition->device_read(PT_OFFSET + (index<<4),buff,sizeof(buff));
	partition->state = buff[PTE_STATE];
	if(partition->state == INACTIVE_PARTITION)
		return 1;
	partition->type = buff[PTE_TYPE];
	partition->offset = read32(&buff[PTE_OFFSET]);
	partition->length = read32(&buff[PTE_LENGTH]);
#ifdef DEBUG_MODE
	print_partition(partition);
#endif
	return 0;
}

uint8_t partition_close(struct partition * partition)
{
	if(!partition)
		return 1;
	partition->state = INACTIVE_PARTITION;
	return 0;
}
