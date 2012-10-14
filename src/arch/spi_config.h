/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _SPI_CONFIG_H
#define _SPI_CONFIG_H
/**
* @addtogroup arch
* @{
*/
/**
* @addtogroup spi
* @{
*/
/**
* @file
* SPI configuration
* @author Paweł Lebioda <pawel.lebioda89@gmail.com>
*/
/**
* @name Configuration
* @{
*/
/**
* SPI IO port register
*/
#define SPI_PORT		PORTB
/**
* SPI IO direction register
*/
#define SPI_DDR			DDRB
/**
* MOSI pin number
*/
#define SPI_MOSI		2
/**
* MISO pin number
*/
#define SPI_MISO		3
/**
* SCK pin number
*/
#define SPI_SCK			1
/**
* @}
*/
/**
* @}
*/
/**
* @}
*/

#endif //_SPI_CONFIG_H
