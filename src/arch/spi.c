/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "spi.h"

void spi_init(uint8_t baud_rate)
{
    SPCR = (0<<SPIE)|(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)|(0<<SPR1)|(0<<SPR0);
    SPSR = (1<<SPI2X);

    /* CS, MOSI, SCK outputs */
    SPI_DDR |= (1<<SPI_MOSI)|(1<<SPI_SCK);
    /* MISO input */
    SPI_DDR &= ~(1<<SPI_MISO);
    /* MOSI, SCK low */
    SPI_PORT &= ~(1<<SPI_MOSI) & ~(1<<SPI_SCK);
    
    SPI_DISABLE();	
}
