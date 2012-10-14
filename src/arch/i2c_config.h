/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _I2C_CONFIG_H
#define _I2C_CONFIG_H
/**
* @addtogroup arch
* @{
*/
/**
* @addtogroup i2c
* @{
*/
/**
* @file i2c configuration
* @author Paweł Lebioda <pawel.lebioda89@gmail.com>
*/
/**
* @name Configuration
*/
/**
* i2c IO register
*/
#define I2C_PORT	PORTD
/**
* i2c IO direction register
*/
#define I2C_DDR		DDRD
/**
* i2c SDA (Serial Data) pin number
*/
#define I2C_SDA		1
/**
* i2c SCL (Serial Clock) pin number
*/
#define I2C_SCL		0
/**
* i2c clock frequency
*/
#define I2C_SCL_FREQ	10000UL
/**
* i2c prescaler
* @see AVR datasheet
*/
#define I2C_PRESCALER	0
/**
* i2c TWBR register value 
* @note This value is computed automatically according configuration and AVR datasheet
* @see ATMega128L datasheet page. 203
*/
#define I2C_TWBR	((F_CPU/I2C_SCL_FREQ-16)/(1<<((I2C_PRESCALER<<1)+1)))
/**
* @}
*/
/**
* @}
*/
/**
* @}
*/
#endif //_I2C_CONFIG_H