/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

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
* SPI implementation
* \author Paweł Lebioda <pawel.lebioda89@gmail.com>
*/

#include "spi.h"

/**
* Initializes SPI interface
* \returns void
*/
void spi_init(void)
{
    SPCR = (0<<SPIE)|(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)|(0<<SPR1)|(0<<SPR0);
    SPSR = (1<<SPI2X);

    /* CS, MOSI, SCK outputs */
//     SPI_DDR |= (1<<SPI_MOSI)|(1<<SPI_SCK);
    /* MISO input */
//     SPI_DDR &= ~(1<<SPI_MISO);
//     SPI_PORT |= (1<<SPI_MISO)|(1<<SPI_MOSI);
    
    SPI_ENABLE();	
}
/**
* Sends a byte over the SPI bus
* \param[in] data Byte to send
*/
void spi_write(uint8_t data)
{
  SPI_DATA = data;
  SPI_WAIT();
}
/**
* Reads a byte from the SPI bus
* \param[in] data Value which will be on the MOSI bus while reading
* \returns The received byte
*/
uint8_t spi_read(uint8_t data)
{
  SPI_DATA = data;
  SPI_WAIT();
  return (uint8_t)SPI_DATA;
}
/**
* Writess a block of bytes to the SPI bus
* \param[in] data Pointer to data to write
* \param[in] len Length of data
* \returns 0 on success
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
* \param[in] data Pointer to data to write
* \param[in] len Length of data
* \param[in] bus Value which will be on the MOSI bus while reading
* \returns 0 on success
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