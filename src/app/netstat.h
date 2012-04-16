/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _NETSTAT_H
#define _NETSTAT_H

#include <stdio.h>
#include <stdint.h>

#define NETSTAT_OPT_ALL		0xff
#define NETSTAT_OPT_IFACE	0
#define NETSTAT_OPT_IP		1
#define NETSTAT_OPT_ARP		2
#define NETSTAT_OPT_UDP		3
#define NETSTAT_OPT_TCP		4

uint8_t netstat(FILE * fh,uint8_t args);


#endif //_NETSTAT_H
