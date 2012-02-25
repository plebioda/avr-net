/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _UART_H
#define _UART_H

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdio.h>

#include "uart_config.h"


typedef struct _UART_BAUD_DATA
{
  uint16_t baud;        
  uint16_t ubrr;
} UART_BAUD_DATA;

typedef struct fcheat_file
{
  char *buf;
  unsigned char unget;
  uint8_t flags;
#define FCHEAT_SRD 0x0001
#define FCHEAT_SWR 0x0002
  int size;
  int len;
  int (*put)(char,FILE*);
  int (*get)(FILE*);
} fcheat_file;

#define FCHEAT_STATIC_FDEVOPENR(get) \
{0,0,FCHEAT_SRD,0,0,0,get}

#define FCHEAT_STATIC_FDEVOPENW(put) \
{0,0,FCHEAT_SWR,0,0,put,0}

#define FCHEAT_STATIC_FDEVOPENWR(put,get) \
{0,0,FCHEAT_SWR|FCHEAT_SRD,0,0,put,get}
#endif //_FCHEAT_STDIO_INCLUDED

void            uart_init         (uint8_t baud_index);
int             uart_putc         (char data,FILE*);
int             uart_getc         (FILE*);

#define uart_init()	uart_init(UART_BAUD_INDEX_DEFAULT)

#endif //_UART_H
