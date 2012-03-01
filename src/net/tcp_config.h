/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _TCP_CONFIG_H
#define _TCP_CONFIG_H

#include "net.h"

#define TCP_MAX_SOCKETS		2

#define TCP_MSS			(ETHERNET_MAX_PACKET_SIZE - NET_HEADER_SIZE_ETHERNET - NET_HEADER_SIZE_IP - NET_HEADER_SIZE_TCP)	

#endif //_TCP_CONFIG_H