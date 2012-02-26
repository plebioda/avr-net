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

#define ARP_TABLE_SIZE			2
#define ARP_TIMER_TICK_MS		1000
#define ARP_TABLE_ENTRY_TIMEOUT		0xa
#define ARP_TABLE_ENTRY_REQ_TIME	0xa

#endif //_ARP_CONFIG_H