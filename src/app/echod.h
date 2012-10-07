/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ECHOD_H
#define _ECHOD_H

#include "echod_config.h"
#include <stdint.h>

enum echo_event
{
	echo_event_error,
	echo_event_established,
	echo_event_data_rcv,
	echo_event_buffer,
	echo_event_closed
};

typedef void (*echo_callback)(enum echo_event event);

uint8_t echod_start(echo_callback callback);
uint8_t echod_stop(void);

#define ECHO_ERR_CALLBACK	1
#define ECHO_ERR_SOCKET		2

#endif //_ECHOD_H