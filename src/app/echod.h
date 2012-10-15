/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ECHOD_H
#define _ECHOD_H

/**
 * @addtogroup app
 * @{
 * @addtogroup echo
 * @{
 * @file echod.h
 * @author Pawel Lebioda <pawel.lebioda89@gmail.com>
 * @brief This file contains declarations of functions used for echo server
 * maintanance
 */ 

#include "echod_config.h"
#include <stdint.h>

#define ECHO_ERR_SOCKET		1

/**
 * Initializes echo server
 * @param [in] callback Echo server callback function
 * @return One for success, zero for failure
 */  
uint8_t echod_start(void);

/**
 * Stops echo server
 * @return One for success, zero for failure
 */ 
uint8_t echod_stop(void);

/**
 * @}
 * @}
 */ 
#endif //_ECHOD_H
