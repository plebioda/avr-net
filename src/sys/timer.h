/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _TIMER_H
#define _TIMER_H

#include <stdint.h>

#include "timer_config.h"

typedef enum 
{
	TIMER_MODE_ONE_SHOT,
	TIMER_MODE_PERIODIC
} timer_mode_t;

typedef int8_t timer_t;
typedef void (*timer_callback_t)(timer_t timer,void * arg);

void timer_init(void);
void timer_tick(void);
uint8_t timer_set(timer_t timer,int32_t ms, timer_mode_t);
uint8_t timer_stop(timer_t timer);
int32_t timer_get_time(timer_t timer);

timer_t timer_alloc(timer_callback_t callback);
void timer_free(timer_t);
uint8_t timer_set_arg(timer_t timer,void * arg);



#endif //_TIMER_H
