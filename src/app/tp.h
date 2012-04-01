/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _TP_H
#define _TP_H

#include "tp_config.h"

#include "../sys/rtc.h"

typedef void (*tp_callback)(uint8_t status,uint32_t time);

uint8_t tp_get_time(tp_callback callback);

#define TP_ERR_CALLBACK		1
#define TP_ERR_SOCKET		2
#define TP_ERR_TIMER		3
#define TP_ERR_CONTEXT		4
#define TP_ERR_ARP_TIMEOUT	5
#define TP_ERR_UDP_SEND		6
#define TP_ERR_TIMEOUT		7
#define TP_ERR_STATE		8
#endif //_TP_H