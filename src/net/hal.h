/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _HAL_H
#define _HAL_H

#include "../dev/enc28j60.h"

#define hal_init(mac)	enc28j60_init((mac))

#define hal_send_packet(buff,len) enc28j60_send_packet((buff),(len))

#define hal_receive_packet(buff,max_len) enc28j60_receive_packet((buff),(max_len))

#define hal_link_up()	


#endif //_HAL_H


