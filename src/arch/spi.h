/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _SPI_H
#define _SPI_H
/**
* \addtogroup arch
* @{
*/
/**
* \addtogroup spi
* @{
*/
/**
* \file
* SPI header
* \author Paweł Lebioda <pawel.lebioda89@gmail.com>
*/

#include <stdint.h>
#include <avr/io.h>

#include "spi_config.h"

/**
* Waits for sending or receiving data byte
*/
#define SPI_WAIT()              while(!(SPSR & (1<<SPIF)))
/**
* SPI data byte register
*/
#define SPI_DATA                SPDR
/**
* Enables SPI interface
*/
#define SPI_ENABLE()            SPCR |= (1<<SPE)
/**
* Disables SPI interface
*/
#define SPI_DISABLE()           SPCR &= ~(1<<SPE)

void spi_init(void);
void spi_write(uint8_t data);
uint8_t spi_read(uint8_t data);

/**
* @}
*/
/**
* @}
*/
#endif //_SPI_H
