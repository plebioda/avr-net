/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _EXMEM_H
#define _EXMEM_H
/**
* \addtogroup arch
* @{
*/
#include "exmem_config.h"
#include <avr/io.h>

// #if EXMEM_USE_ATTR
// 	#define EXMEM __attribute__ ((section(".exram")))
// #else
// 	#define EXMEM
// #endif

void before_main(void) __attribute__((naked)) __attribute__((section(".init3")));
/**
* @}
*/
#endif //_EXMEM_H
