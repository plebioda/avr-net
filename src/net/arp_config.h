/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ARP_CONFIG_H
#define _ARP_CONFIG_H

#include "../sys/timer_config.h"

#define ARP_TABLE_SIZE			4
#define ARP_TIMER_TICK_MS		10*TIMER_MS_PER_TICK
#define ARP_TABLE_ENTRY_TIMEOUT		0xff

#endif //_ARP_CONFIG_H