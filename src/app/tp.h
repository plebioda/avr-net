/*
 * Copyright (c) 2012 by Pawel Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/**
 * @addtogroup app
 * @{
 * @addtogroup tp
 * @{
 * @file tp.h
 * @author Pawel Lebioda <pawel.lebioda89@gmail.com>
 * @brief This file contains declarations of Time Protocol client module
 */ 
#ifndef _TP_H
#define _TP_H

#include <stdint.h>

#include "tp_config.h"

#define TP_ERR_CALLBACK		1
#define TP_ERR_SOCKET		2
#define TP_ERR_TIMER		3
#define TP_ERR_CONTEXT		4
#define TP_ERR_ARP_TIMEOUT	5
#define TP_ERR_UDP_SEND		6
#define TP_ERR_TIMEOUT		7
#define TP_ERR_STATE		8

/**
 * Time Protocol Callback function
 * @param [in] event Event type
 * @param [in] time Time
 */  
typedef void (*tp_callback)(uint8_t event,uint32_t time);

/**
 * Initializes request of current time from time server
 * @param [in] callback Callback function which will be invoked when time
 * client finish its job
 */ 
uint8_t tp_get_time(tp_callback callback);

/**
 * @}
 * @}
 */
#endif //_TP_H
