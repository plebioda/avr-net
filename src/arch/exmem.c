/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/**
* @addtogroup arch
* @{
*/
#include "exmem.h"

#include <avr/io.h>

void before_main(void)
{
	MCUCR = 1<<SRE;
	SFIOR = 1<<XMBK | 1<<XMM0;
	DDRC = 0x80;
	PORTC &= ~(1<<7);
	SP = 0xffff;
}
/**
* @}
*/
