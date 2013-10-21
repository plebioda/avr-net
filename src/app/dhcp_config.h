/*
 * Copyright (c) 2012 by Pawel Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DHCP_CONFIG_H
#define _DHCP_CONFIG_H

#define DHCP_SEC_BOUND		100	
#define DHCP_TIMEOUT		1000
#define DHCP_RTX		4
#define DHCP_USE_HOST_NAME	1
#if DHCP_USE_HOST_NAME
	#define DHCP_HOST_NAME	"AVR-NET"
#endif

#endif //_DHCP_CONFIG_H
