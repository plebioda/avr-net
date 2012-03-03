/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _FIFO_H
#define _FIFO_H

#include "fifo_config.h"

#include <stdint.h>

struct fifo;

struct fifo * fifo_alloc();
void fifo_free();

uint8_t fifo_clear(struct fifo * fifo);
uint16_t fifo_length(struct fifo * fifo);
uint16_t fifo_size(struct fifo * fifo);
uint16_t fifo_space(struct fifo * fifo);
uint16_t fifo_enqueue(struct fifo * fifo,uint8_t * data,uint16_t len);
uint16_t fifo_dequeue(struct fifo * fifo,uint8_t * data,uint16_t len); 
uint16_t fifo_peek(struct fifo * fifo,uint8_t * data,uint16_t len);
uint16_t fifo_skip(struct fifo * fifo,uint16_t len);
#endif //_FIFO_H