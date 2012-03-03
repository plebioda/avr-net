/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG_MODE
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

static struct fifo fifos[FIFO_MAX_COUNT] EXMEM;

#define FOREACH_FIFO(fifo) for(fifo = &fifos[0] ; fifo < &fifos[FIFO_MAX_COUNT]; fifo++)

static uint8_t fifo_valid(struct fifo * fifo);
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
      return 0xffff;
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
uint16_t fifo_enqueue(struct fifo * fifo,uint8_t * data,uint16_t len)
{
    if(!fifo_valid(fifo))
      return 0;
    if(fifo->length + len > FIFO_SIZE)
      len = FIFO_SIZE - fifo->length;
    if(!len)
      return 0;
    fifo->length += len;
    uint16_t bytes_to_bound = (uint16_t)&fifo->buffer[FIFO_SIZE] - (uint16_t)fifo->last;
    if(len > bytes_to_bound)
    {
      memcpy(fifo->last,data,bytes_to_bound);
      len -= bytes_to_bound;
      fifo->last = fifo->buffer;
      data += bytes_to_bound;
    }
    memcpy(fifo->last,data,len);
    fifo->last += len;
    if(fifo->last >= &fifo->buffer[FIFO_SIZE])
      fifo->last = fifo->buffer;
    return len;
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
}
uint16_t fifo_peek(struct fifo * fifo,uint8_t * data,uint16_t len)
{
  
}
uint16_t fifo_skip(struct fifo * fifo,uint16_t len)
{
  
}

#ifdef DEBUG_MODE
void fifo_print(struct fifo * fifo)
{
    uint8_t * data =  fifo->buffer;
    uint16_t i;
    DEBUG_PRINT("FIFO length: %d\n",fifo_length(fifo));
    DEBUG_PRINT("FIFO space: %d\n",fifo_space(fifo));
    DEBUG_PRINT("FIFO content:\n");
    DEBUG_PRINT(" i  hex dec char\n");
    for(i=0;i<FIFO_SIZE;i++)
    {
      DEBUG_PRINT("%3d: %02x %3d",i,*(data),*(data));
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
#endif //DEBUG_MODE