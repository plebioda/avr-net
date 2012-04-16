/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _PARTITION_H
#define _PARTITION_H

#include <stdint.h>

typedef uint32_t (*device_read_t)(uint32_t offset,uint8_t * buffer,uint32_t length);
typedef uint32_t (*device_write_t)(uint32_t offset,const uint8_t * buffer,uint32_t length);

#define PT_OFFSET 	0x1be	//partition table offset in MBR
#define PTE_STATE	0x0	//state of partition offset in partition table entry
#define PTE_TYPE	0x4	//type of partition offset in partition table entry
#define PTE_OFFSET	0x8	//position of partition offset in partition table entry
#define PTE_LENGTH	0xc	//size of partition offset in partition table entry

#define ACTIVE_PARTITION	0x80
#define INACTIVE_PARTITION	0x00

struct partition
{
    uint8_t 		state;
    uint8_t 		type;
    uint32_t		offset;
    uint32_t 		length;
    device_read_t	device_read;
    device_write_t	device_write;
};

uint8_t	partition_open(struct partition * partition,device_read_t device_read,device_write_t device_write,uint8_t partition_index);
uint8_t partition_close(struct partition * partition);

#endif //_PARTITION_H