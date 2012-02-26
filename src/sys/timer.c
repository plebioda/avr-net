/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <string.h>

#include "timer.h"

#include "../arch/exmem.h"

#define DEBUG_MODE
#include "../debug.h"

#define TIMER_FLAG_STOPPED 	0
#define TIMER_FLAG_ACTIVE 	1
#define TIMER_FLAG_RUNNING 	2

struct timer_core
{
  timer_callback_t callback;
  int32_t ms_left;
  uint8_t flags;
};

static struct timer_core timer_cores[TIMER_MAX] EXMEM;
static timer_t timer_number(const struct timer_core * timer);
static uint8_t timer_valid(const timer_t timer);

#define FOREACH_TIMER(timer) for(timer = &timer_cores[0];timer < &timer_cores[TIMER_MAX] ; ++(timer))

void timer_init(void)
{
    struct timer_core * timer;
    FOREACH_TIMER(timer)
    {
	memset(timer,0,sizeof(*timer));
    }
}

void timer_tick()
{
    struct timer_core * timer;
    FOREACH_TIMER(timer)
    {
	/* Skip unused timers (without callback) */
	if(!timer->callback)
	  continue;
	/* Skip not running timers */
	if(timer->flags != TIMER_FLAG_RUNNING)
	  continue;

	timer->ms_left -= TIMER_MS_PER_TICK;
	if(timer->ms_left < 0)
	  timer->callback(timer_number(timer));
	
    }
}
static uint8_t timer_valid(const timer_t timer)
{
    return (timer >= 0 && timer < TIMER_MAX && timer_cores[timer].callback!= 0);
}
uint8_t timer_set(timer_t timer,int32_t ms)
{
    if(!timer_valid(timer))
      return 0;
    timer_cores[timer].ms_left = ms;
    timer_cores[timer].flags = TIMER_FLAG_RUNNING;
    return 1;
}

uint8_t timer_stop(timer_t timer)
{
    if(!timer_valid(timer))
      return 0;
    timer_cores[timer].flags = TIMER_FLAG_STOPPED;  
    return 1;
}

timer_t timer_alloc(timer_callback_t callback)
{
    struct timer_core * timer;
    if(callback == 0)
      return -1;
    FOREACH_TIMER(timer)
    {
	if(timer->callback == 0)
	{
	    timer->callback = callback;
	    timer->flags = TIMER_FLAG_ACTIVE;
	    timer->ms_left = 0;
	    return timer_number(timer);
	}
    }
    return -1;
}

void timer_free(timer_t timer)
{
    if(timer < 0)
      return;
    if(timer >= TIMER_MAX)
      return;
    timer_cores[timer].callback = 0;
}

static timer_t timer_number(const struct timer_core * timer)
{
    if(!timer)
      return -1;
    timer_t ret = (timer_t)(((uint16_t)timer - (uint16_t)&timer_cores[0])/sizeof(struct timer_core));
    if (timer_valid(ret))
      return ret;
    return -1;
}