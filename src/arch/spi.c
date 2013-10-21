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
/**
* @addtogroup spi
* @{
*/
/**
* @file
* SPI implementation
* @author Paweł Lebioda <pawel.lebioda89@gmail.com>
*/

#include "spi.h"

void spi_low_frequency(void)
{
	SPCR = 	(0 << SPIE) | /* SPI interrupt enable */
	 	(1 << SPE)  | /* SPI enable */
		(0 << DORD) | /* Data order: 0 = MSB first, 1 = LSB first */
		(1 << MSTR) | /* Master mode */
		(0 << CPOL) | /* Clock polarity: 0 = SKC low when idle, 1 = SCK high when idle*/
		(0 << CPHA) | /* Clock phase: 0 = sample on rising edge, 1 = sample on falling edge*/
		(1 << SPR1) | /* Clock frequency: fosc/128*/
		(1 << SPR0);
	SPSR &= ~(1<<SPI2X); /* No doubled speed*/
}
void spi_high_frequency(void)
{
	SPCR = 	(0 << SPIE) | /* SPI interrupt enable */
		(1 << SPE)  | /* SPI enable */
		(0 << DORD) | /* Data order: 0 = MSB first, 1 = LSB first */
		(1 << MSTR) | /* Master mode */
		(0 << CPOL) | /* Clock polarity: 0 = SKC low when idle, 1 = SCK high when idle*/
		(0 << CPHA) | /* Clock phase: 0 = sample on rising edge, 1 = sample on falling edge*/
		(0 << SPR1) | /* Clock frequency: fosc/4*/
		(0 << SPR0);
	SPSR |= (1<<SPI2X); /* Doubled speed*/	
}

/**
* Initializes SPI interface
* @returns void
*/
void spi_init(void)
{
	/* Check if already initialized */
//	 if(SPCR & (1<<SPE))
//		 return;
	/* Configure pins*/
	SPI_DDR |= (1<<SPI_MOSI);
	SPI_DDR |= (1<<SPI_SCK);
	SPI_DDR &=~(1<<SPI_MISO);
	/* Configure SPI*/
	spi_high_frequency();
}
/**
* Sends a byte over the SPI bus
* @param[in] data Byte to send
*/
void spi_write(uint8_t data)
{
	SPI_DATA = data;
	SPI_WAIT();
}
/**
* Reads a byte from the SPI bus
* @param[in] data Value which will be on the MOSI bus while reading
* @returns The received byte
*/
uint8_t spi_read(uint8_t data)
{
	SPI_DATA = data;
	SPI_WAIT();
	return (uint8_t)SPI_DATA;
}
/**
* Writess a block of bytes to the SPI bus
* @param[in] data Pointer to data to write
* @param[in] len Length of data
* @returns 0 on success
*/
uint8_t spi_write_block(uint8_t * data,uint16_t len)
{
	while(len--)
	{
		SPI_DATA = *(data++);
		SPI_WAIT();
	}
	return 0;
}
/**
* Reads a block of bytes from the SPI bus
* @param[in] data Pointer to data to write
* @param[in] len Length of data
* @param[in] bus Value which will be on the MOSI bus while reading
* @returns 0 on success
*/
uint8_t spi_read_block(uint8_t * data,uint16_t len,uint8_t bus)
{
	while(len--)
	{
		SPI_DATA = bus;
		SPI_WAIT();
		*(data++) = (uint8_t)SPI_DATA;
	}
	return 0;
}
/**
* @}
*/
/**
* @}
*/
