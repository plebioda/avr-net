/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DHCP_H
#define _DHCP_H

#include <stdint.h>

enum dhcp_event
{
  dhcp_event_error=0
};

typedef void (*dhcp_callback)(enum dhcp_event event);

uint8_t dhcp_start(dhcp_callback callback);
uint8_t dhcp_stop(void);

#define DHCP_ERR_STATE			1
#define DHCP_ERR_SOCKET_ALLOC		2
#define DHCP_ERR_TIMER_ALLOC		3
#define DHCP_ERR_MEMORY			4

#endif //_DHCP_H