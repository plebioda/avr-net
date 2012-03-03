/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fifo.h"
#include "../arch/exmem.h"

#include <stdint.h>

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
    if(
}
uint16_t fifo_dequeue(struct fifo * fifo,uint8_t * data,uint16_t len)
{
  
}
uint16_t fifo_peek(struct fifo * fifo,uint8_t * data,uint16_t len)
{
  
}
uint16_t fifo_skip(struct fifo * fifo,uint16_t len)
{
  
}