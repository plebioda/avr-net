/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

// 
#include "../debug.h"

#include "fifo.h"
#include "../arch/exmem.h"

#include <stdint.h>
#include <string.h>

struct fifo
{
	uint8_t buffer[FIFO_SIZE];
	uint16_t length;
	uint8_t * first;
	uint8_t * last;
};

static struct fifo fifos[FIFO_MAX_COUNT]; // EXMEM

#define FOREACH_FIFO(fifo) for(fifo = &fifos[0] ; fifo < &fifos[FIFO_MAX_COUNT]; fifo++)

#define FIFO_WRITE_FLAG_PGM		0x80

static uint8_t fifo_valid(struct fifo * fifo);
static uint16_t fifo_write(struct fifo * fifo,const void * data,uint16_t len,uint8_t mode);

void fifo_init()
{
	memset(fifos,0,sizeof(fifos));
}
struct fifo * fifo_alloc()
{
	struct fifo * fifo;
	FOREACH_FIFO(fifo)
	{
		if(fifo->first == 0 && fifo->last == 0)
		{
				if(!fifo_clear(fifo))
					continue;
				return fifo;
		}
	}
	return 0;
}

uint8_t fifo_clear(struct fifo * fifo)
{
	if(!fifo_valid(fifo))
		return 0;
	fifo->length = 0;
	fifo->first = fifo->buffer;
	fifo->last = fifo->buffer;
	return 1;
}

void fifo_free(struct fifo * fifo)
{
	if(!fifo_valid(fifo))
		return;
	fifo->first = fifo->last = 0;
}

uint8_t fifo_valid(struct fifo * fifo)
{
	return (fifo >= &fifos[0] && fifo < &fifos[FIFO_MAX_COUNT]);
}
uint16_t fifo_length(struct fifo * fifo)
{
	if(!fifo_valid(fifo))
		return 0;
	return fifo->length;
}
uint16_t fifo_size(struct fifo * fifo)
{
	if(!fifo_valid(fifo))
		return 0;
	return FIFO_SIZE;
}
uint16_t fifo_space(struct fifo * fifo)
{	
	if(!fifo_valid(fifo))
		return 0;
	return (FIFO_SIZE-fifo->length);
}

uint16_t fifo_write(struct fifo * fifo,const void * data,uint16_t len,uint8_t mode)
{
	if(!fifo_valid(fifo))
		return 0;
	if(fifo->length + len > FIFO_SIZE)
			len = FIFO_SIZE - fifo->length;
	if(!len)
		return 0;
	uint16_t ret = len;
	fifo->length += len;
	uint16_t bytes_to_bound = (uint16_t)&fifo->buffer[FIFO_SIZE] - (uint16_t)fifo->last;
	if(len > bytes_to_bound)
	{
		if(mode & FIFO_WRITE_FLAG_PGM)
			memcpy_P(fifo->last,(prog_uint8_t*)data,bytes_to_bound);
		else
			memcpy(fifo->last,(uint8_t*)data,bytes_to_bound);
		len -= bytes_to_bound;
		fifo->last = fifo->buffer;
		data += bytes_to_bound;
	}
	if(mode & FIFO_WRITE_FLAG_PGM)
		memcpy_P(fifo->last,(prog_uint8_t*)data,len);
	else
		memcpy(fifo->last,(uint8_t*)data,len);
	fifo->last += len;
	if(fifo->last >= &fifo->buffer[FIFO_SIZE])
		fifo->last = fifo->buffer;
	return ret;		
}
uint16_t fifo_enqueue(struct fifo * fifo,const uint8_t * data,uint16_t len)
{
	return fifo_write(fifo,(void*)data,len,0);
}

uint16_t fifo_enqueue_P(struct fifo * fifo,const prog_uint8_t * data,uint16_t len)
{
	return fifo_write(fifo,(void*)data,len,FIFO_WRITE_FLAG_PGM);
}

uint16_t fifo_dequeue(struct fifo * fifo,uint8_t * data,uint16_t len)
{
	if(!fifo_valid(fifo))
		return 0;
	if(fifo->length < len)
		len = fifo->length;
	if(!len)
		return 0;
	fifo->length -= len;
	uint16_t ret = len;
	uint16_t bytes_to_bound = (uint16_t)&fifo->buffer[FIFO_SIZE] - (uint16_t)fifo->first;
	if(len > bytes_to_bound)
	{
		memcpy(data,fifo->first,bytes_to_bound);
		len -= bytes_to_bound;
		data += bytes_to_bound;
		fifo->first = fifo->buffer;
	}
	memcpy(data,fifo->first,len);
	fifo->first += len;
	if(fifo->first >= &fifo->buffer[FIFO_SIZE])
		fifo->first = fifo->buffer;
	return ret;
}
uint16_t fifo_peek(struct fifo * fifo,uint8_t * data,uint16_t len,uint16_t offset)
{
	/* check if fifo pointer is valid */
	if(!fifo_valid(fifo))
		return 0; 
	/* if total number of bytes to skip and to peek is greater than 
		actual length of fifo, clap this value to fifo's length*/
	uint16_t offset_len = offset+len;
	if(offset_len > fifo->length)
		offset_len = fifo->length;
	/* now get actual number of bytes to peek */
	len = offset_len - offset;
	/* save number of peeked bytes */
	uint16_t ret = len;
	/* we cannot modify any pointer of fifo so we use temp pointer */
	uint8_t * ptr = fifo->first;
	/* get number of bytes to boundary of fifo's buffer */
	uint16_t bytes_to_bound = (uint16_t)&fifo->buffer[FIFO_SIZE] - (uint16_t)ptr;
	/* if number bytes to skip exceeds number of bytes to boundary 
		set temp pointer to beggining of the buffer and decrease number of bytes to skip left */
	if(offset > bytes_to_bound)
	{
		offset -= bytes_to_bound;
		ptr = fifo->buffer;
	}
	/* update pointer */
	ptr += offset;
	/* do the same for number of bytes to peek */
	bytes_to_bound = (uint16_t)&fifo->buffer[FIFO_SIZE] - (uint16_t)ptr;
	if(len > bytes_to_bound)
	{
		/* copy part of bytes up to boundary */
		memcpy(data,ptr,bytes_to_bound);
		/* update length and pointer */
		len -= bytes_to_bound;
		ptr = fifo->buffer;
		data += bytes_to_bound;
	}
	/* copy the rest */
	memcpy(data,ptr,len);
	return ret;
}
uint16_t fifo_skip(struct fifo * fifo,uint16_t len)
{
	/* check if fifo pointer is valid*/
	if(!fifo_valid(fifo))
		return 0;
	/* if number of bytes to skip is greater than 
	actual length of fifo, clap this value to fifo's length*/
	if(len > fifo->length)
		len = fifo->length;
	/* return 0 if fifo is empty or number of bytes to skip is zero*/
	if(!len)
		return 0;
	/* save number of skipped bytes to return */
	uint16_t ret = len;
	/* update fifo's length */
	fifo->length -= len;
	/* get number of bytes to boundary of fifo's buffer */
	uint16_t bytes_to_bound = (uint16_t)&fifo->buffer[FIFO_SIZE] - (uint16_t)fifo->first;
	/* if number bytes to skip exceeds number of bytes to boundary 
	set fifo's first pointer to beggining of the buffer and decrease number of bytes to skip left*/
	if(len > bytes_to_bound)
	{
		fifo->first = fifo->buffer;
		len -= bytes_to_bound;
	}
	/* shift fifo's first pointer */
	fifo->first += len;
	return ret;
}

// #ifdef DEBUG_MODE
void fifo_print(struct fifo * fifo)
{
	uint8_t * data = fifo->buffer;
	uint16_t i;
	DBG_INFO("FIFO length: %d\n",fifo_length(fifo));
	DBG_INFO("FIFO space: %d\n",fifo_space(fifo));
	DBG_INFO("FIFO content:\n");
	DBG_INFO(" i	hex dec char\n");
	for(i=0;i<FIFO_SIZE;i++)
	{
		DBG_INFO("%3d: %02x %3d",i,*(data),*(data));
		if(data == fifo->first)
		{
			DEBUG_PRINT(" <- first");
		}
		if (data == fifo->last)
		{
			DEBUG_PRINT(" <- last");
		}
		data++;
		DEBUG_PRINT("\n");
	}
}
// #endif //DEBUG_MODE
