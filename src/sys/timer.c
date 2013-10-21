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

//
#include "../debug.h"

#define TIMER_STATE_UNUSED 	0
#define TIMER_STATE_STOPPED 	1
#define TIMER_STATE_RUNNING 	2

struct timer_core
{
	timer_callback_t callback;
	int32_t ms_left;
	int32_t ms_org;
	uint8_t state;
	timer_mode_t mode;
	void * arg;
};

static struct timer_core timer_cores[TIMER_MAX]; // EXMEM
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
	DBG_INFO("timer tick\n");
	FOREACH_TIMER(timer)
	{
		/* Skip unused timers (without callback) */
		if(!timer->callback)
		{
			continue;
		}
		/* Skip not running timers */
		if(timer->state != TIMER_STATE_RUNNING)
		{
			continue;
		}

		timer->ms_left -= TIMER_MS_PER_TICK;
		if(timer->ms_left < 0)
		{
			timer->callback(timer_number(timer),timer->arg);
			if(TIMER_MODE_PERIODIC == timer->mode)
			{
				timer->ms_left = timer->ms_org;
			}
			else
			{
				timer->state = TIMER_STATE_STOPPED;
			}
		}

	}
}
int32_t timer_get_time(timer_t timer)
{
	if(!timer_valid(timer))
	{
		return 0;
	}

	return timer_cores[timer].ms_left;
}

uint8_t timer_set_arg(timer_t timer,void * arg)
{
	if(!timer_valid(timer))
	{
		return 0;
	}

	timer_cores[timer].arg = arg;

	return 1;
}

static uint8_t timer_valid(const timer_t timer)
{
	return (timer >= 0 && timer < TIMER_MAX && timer_cores[timer].callback!= 0);
}

uint8_t timer_set(timer_t timer,int32_t ms, timer_mode_t mode)
{
	if(!timer_valid(timer) || ms < 0)
	{
		return 0;
	}
		
	timer_cores[timer].ms_org = ms;
	timer_cores[timer].ms_left = ms;
	timer_cores[timer].mode = mode;
	timer_cores[timer].state = TIMER_STATE_RUNNING;
	
	return 1;
}

uint8_t timer_stop(timer_t timer)
{
	if(!timer_valid(timer))
	{
		return 0;
	}

	timer_cores[timer].state = TIMER_STATE_STOPPED;  
	
	return 1;
}

timer_t timer_alloc(timer_callback_t callback)
{
	struct timer_core * timer;
	if(callback == 0)
	{
		return -1;
	}

	FOREACH_TIMER(timer)
	{
		if(timer->callback == 0)
		{
			timer->callback = callback;
			timer->state= TIMER_STATE_STOPPED;
			timer->ms_left = 0;
			
			return timer_number(timer);
		}
	}

	return -1;
}

void timer_free(timer_t timer)
{
	if(timer < 0)
	{
		return;
	}
	if(timer >= TIMER_MAX)
	{
		return;
	}
	timer_cores[timer].callback = 0;
	timer_cores[timer].state = TIMER_STATE_UNUSED;
}

static timer_t timer_number(const struct timer_core * timer)
{
	if(!timer)
	{
		return -1;
	}
	timer_t ret = (timer_t)(((uint16_t)timer - (uint16_t)&timer_cores[0])/sizeof(struct timer_core));
	if (timer_valid(ret))
	{
		return ret;
	}
	else
	{
		return -1;
	}
}
