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
* \addtogroup uart
* @{
*/
/**
* \file UART implementation
* \author Paweł Lebioda <pawel.lebioda89@gmail.com>
*/
#include "uart.h"

#define UBRRVAL (F_CPU/(UART_BAUD_RATE*16)-1)

/**
* Initializes UART interface
*/
void uart_init(void)
{
    UBRR1H = 12>>8;
    UBRR1L = 12&0xff;
    UCSR1A = (1<<U2X1);
    UCSR1B = (1<<RXEN1) | (1<<TXEN1);
    UCSR1C = (1<<UCSZ11) | (1<<UCSZ10); 
}
/**
* Sends a byte over UART bus
* \param[in] data The byte to send
* \param[in] fh File handler to which to write data (unused)
* \returns 0
*/
int uart_putc(char data,FILE* fh)
{
    while(!((1<<UDRE1) & UCSR1A));
    UDR1 = data;
    return 0;
}
/**
* Receives a byte from UART
*\param[in] fh File handler from which to read (unused)
*\returns Received byte
*/
int uart_getc         (FILE* fh)
{
    char data;
    while(!(1<<RXC1 & UCSR1A)){}
    data = UDR1;
    return data;
}
/**
* @}
*/
/**
* @}
*/