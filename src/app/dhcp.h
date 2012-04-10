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

#include "dhcp_config.h"

enum dhcp_event
{
  dhcp_event_error=0,
  dhcp_event_timeout,
  dhcp_event_lease_acquired,
  dhcp_event_lease_expiring,
  dhcp_event_lease_expired,
  dhcp_event_lease_denied
};

typedef void (*dhcp_callback)(enum dhcp_event event);

uint8_t dhcp_start(dhcp_callback callback);
void dhcp_stop(void);

#define DHCP_ERR_STATE			1
#define DHCP_ERR_SOCKET_ALLOC		2
#define DHCP_ERR_TIMER_ALLOC		3
#define DHCP_ERR_MEMORY			4

#endif //_DHCP_H