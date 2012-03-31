/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _I2C_CONFIG_H
#define _I2C_CONFIG_H

#define I2C_DDR		DDRD
#define I2C_PORT	PORTD
#define I2C_SDA		1
#define I2C_SCL		0

#define I2C_SCL_FREQ	10000UL
#define I2C_PRESCALER	0
#define I2C_TWBR	((F_CPU/I2C_SCL_FREQ-16)/(1<<((I2C_PRESCALER<<1)+1)))


#endif //_I2C_CONFIG_H