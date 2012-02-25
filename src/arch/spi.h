/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _SPI_H
#define _SPI_H

#define SPI_WAIT()              while(!(SPSR & (1<<SPIF)))
#define SPI_DATA                SPDR
#define SPI_ENABLE()            SPCR |= (1<<SPE)
#define SPI_DISABLE()           SPCR &= ~(1<<SPE)

void spi_init(uint8_t baud_rate);

#endif //_SPI_H
