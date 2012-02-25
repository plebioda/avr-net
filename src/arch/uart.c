/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include "uart.h"

UART_BAUD_DATA g_prog_uart_baud[5] PROGMEM = 
{
        {2400, 207}, // +0.2%
        {4800, 103}, // +0.2%
        {9600, 51}, //  +0.2%
        {14400, 25}, // -0.8%
        {38400, 12}  // +0.2%
};

void            uart_init(uint8_t baud_index)
{
    UCSR0B = 0;
    uint16_t ubrr = pgm_read_word(&g_prog_uart_baud[baud_index].ubrr);
    UBRR0H = (uint8_t)(ubrr>>8);
    UBRR0L = (uint8_t)(ubrr);
    UCSR0A = 0;
    UCSR0B = (1<<RXCIE0) | (1<<RXEN0) | (1<<TXEN0);
    UCSR0C = (1<<URSEL0) | (1<<UCSZ01) | (1<<UCSZ00); 
}

int             uart_putc         (char data,FILE* f)
{
    while(!(1<<UDRE0 & UCSR0A)){}
    UDR0 = data;
    return 0;
}

int             uart_getc         (FILE* f)
{
    char data;
    while(!(1<<RXC0 & UCSR0A)){}
    data = UDR0;
    return data;
}
