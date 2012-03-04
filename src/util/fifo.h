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
#include <avr/pgmspace.h>


struct fifo;

struct fifo * fifo_alloc(void);
void fifo_free(struct fifo * fifo);
void fifo_init(void);

uint8_t fifo_clear(struct fifo * fifo);
uint16_t fifo_length(struct fifo * fifo);
uint16_t fifo_size(struct fifo * fifo);
uint16_t fifo_space(struct fifo * fifo);
uint16_t fifo_enqueue(struct fifo * fifo,const uint8_t * data,uint16_t len);
uint16_t fifo_enqueue_P(struct fifo * fifo,const prog_uint8_t * data,uint16_t len);
uint16_t fifo_dequeue(struct fifo * fifo,uint8_t * data,uint16_t len); 
uint16_t fifo_peek(const struct fifo * fifo,uint8_t * data,uint16_t len,uint16_t offset);
uint16_t fifo_skip(struct fifo * fifo,uint16_t len);

// #ifdef DEBUG_MODE
void fifo_print(struct fifo * fifo);
// #endif //DEBUG_MODE

#endif //_FIFO_H