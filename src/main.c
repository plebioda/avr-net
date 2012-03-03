/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <avr/io.h>
#include <avr/iom162.h>
#include <avr/interrupt.h>

#define DEBUG_MODE
#include "debug.h"

#include "arch/exmem.h"
#include "arch/spi.h"
#include "arch/uart.h"

#include "sys/timer.h"

#include "net/hal.h"
#include "net/ethernet.h"
#include "net/ip.h"
#include "net/arp.h"
#include "net/udp.h"
#include "net/tcp.h"

#include "app/app_config.h"
#include "app/tftp.h"

ip_address ip_remote = {192,168,2,1};

void udp_callback(udp_socket_t socket,uint8_t * data,uint16_t len);
void tcp_callback(tcp_socket_t socket,enum tcp_event event)
{
    DEBUG_PRINT("tcp callback: socket %d event: ",socket);
    switch(event)
    {
      case tcp_event_nop:
	break;
      case tcp_event_connection_established:
	DEBUG_PRINT("Connection established, port = %u\n",tcp_get_remote_port(socket));
	break;
      case tcp_event_connection_incoming:
	DEBUG_PRINT("Incoming connection, port = %u\n",tcp_get_remote_port(socket));
	tcp_accept(socket);
	break;
      case tcp_event_error:
	DEBUG_PRINT("Error\n");
	tcp_socket_free(socket);
	break;
      case tcp_event_timeout:
	DEBUG_PRINT("Timeout\n");
// 	tcp_connect(socket,&ip_remote,80);
	tcp_socket_free(socket);
	break;
    }
}

int main(void)
{
  timer_init();
  DEBUG_INIT();
  spi_init(0);
  ethernet_address my_mac = {'<','P','A','K','O','>'};
  ip_address my_ip = {192,168,2,200};
  ethernet_init(&my_mac);
  ip_init(&my_ip);
  hal_init(my_mac);
  arp_init();
  udp_init();
  
  /* Timer 1 init*/
  /* (clk = 8Mhz) / 256 = 31.25 kHz -> 32 us */
  TCNT1 = 312;
  TCCR1B |= (1<<CS12) | (0<<CS11) | (0<<CS10);
  TIMSK |= (1<<TOIE1);
  /* disable USART0 Receive interrupt */
  UCSR0B &= ~(1<<RXCIE0);
  sei();
  
//   udp_socket_t socket = udp_socket_alloc(12348,udp_callback);
  
#if APP_TFTP
  tftpd_init();
#endif
  tcp_init();
  DEBUG_PRINT_COLOR(B_IYELLOW,"Initialized...\n");  
  uint8_t ret;
  tcp_socket_t tcp_socket,tcp_socket80;
  tcp_socket = tcp_socket_alloc(tcp_callback);
  tcp_socket80 = tcp_socket_alloc(tcp_callback);
  DEBUG_PRINT("socket = %d\n",tcp_socket);
//   ret = tcp_listen(tcp_socket);

//   ret = tcp_connect(tcp_socket,&ip_remote,80);
  tcp_listen(tcp_socket80,80);
//   DEBUG_PRINT("connect = %d\n",ret);
  for(;;)
  {
	cli();
	ethernet_handle_packet();
	sei();
	_delay_us(10);
	
  }
  return 0;
}

SIGNAL(SIG_OVERFLOW1)
{
    /* 10 ms / 32 us = 312.5 */
    TCNT1 = 312;
    timer_tick();    
}

void udp_callback(udp_socket_t socket,uint8_t * data,uint16_t len)
{
    uint16_t i;
    uint8_t * tx_buffer = udp_get_buffer();
    DEBUG_PRINT("Socket: %d\nResponse:\n",socket);
    for(i=0;i<len;i++)
    {
      data[i] -= 0x20;
      tx_buffer[i] = data[i];
      DEBUG_PRINT("%c",data[i]);
    }
    DEBUG_PRINT("\n");
    udp_send(socket,len);
}	
// #define DEBUG_ALL_INTERRUPTS
#ifdef DEBUG_ALL_INTERRUPTS
SIGNAL(SIG_OVERFLOW0)
{
    DEBUG_PRINT("SIG_OVERFLOW0\n");
}
SIGNAL(SIG_INTERRUPT0)
{
    DEBUG_PRINT("SIG_INTERRUPT0\n");
}

SIGNAL(SIG_INTERRUPT1)
{
    DEBUG_PRINT("SIG_INTERRUPT1\n");
}
SIGNAL(SIG_INTERRUPT2)
{
    DEBUG_PRINT("SIG_INTERRUPT2\n");
}
SIGNAL(SIG_PIN_CHANGE0)
{
    DEBUG_PRINT("SIG_PIN_CHANGE0\n");
}
SIGNAL(SIG_PIN_CHANGE1)
{
    DEBUG_PRINT("SIG_PIN_CHANGE1\n");
}
SIGNAL(SIG_INPUT_CAPTURE3)
{
    DEBUG_PRINT("SIG_INPUT_CAPTURE3\n");
}
SIGNAL(SIG_OUTPUT_COMPARE3A)
{
    DEBUG_PRINT("SIG_OUTPUT_COMPARE3A\n");
}

SIGNAL(SIG_OUTPUT_COMPARE3B)
{
    DEBUG_PRINT("SIG_OUTPUT_COMPARE3B\n");
}

SIGNAL(SIG_OVERFLOW3)
{
    DEBUG_PRINT("SIG_OVERFLOW3\n");
}
SIGNAL(SIG_OUTPUT_COMPARE2)
{
    DEBUG_PRINT("SIG_OUTPUT_COMPARE2\n");
}
SIGNAL(SIG_OVERFLOW2)
{
    DEBUG_PRINT("SIG_OVERFLOW2\n");
}
SIGNAL(SIG_INPUT_CAPTURE1)
{
    DEBUG_PRINT("SIG_INPUT_CAPTURE1\n");
}
SIGNAL(SIG_OUTPUT_COMPARE1A)
{
    DEBUG_PRINT("SIG_OUTPUT_COMPARE1A\n");
}
SIGNAL(SIG_OUTPUT_COMPARE1B)
{
    DEBUG_PRINT("SIG_OUTPUT_COMPARE1B\n");
}
SIGNAL(SIG_OUTPUT_COMPARE0)
{
    DEBUG_PRINT("SIG_OUTPUT_COMPARE0\n");
}

SIGNAL(SIG_SPI)
{
    DEBUG_PRINT("SIG_SPI\n");
}
SIGNAL(SIG_USART0_RECV)
{
    DEBUG_PRINT("SIG_USART0_RECV\n");
}
SIGNAL(SIG_USART1_RECV)
{
    DEBUG_PRINT("SIG_USART1_RECV\n");
}
SIGNAL(SIG_USART0_DATA)
{
    DEBUG_PRINT("SIG_USART0_DATA\n");
}
SIGNAL(SIG_USART1_DATA)
{
    DEBUG_PRINT("SIG_USART1_DATA\n");
}
SIGNAL(SIG_USART0_TRANS)
{
    DEBUG_PRINT("SIG_USART0_TRANS\n");
}
SIGNAL(SIG_USART1_TRANS)
{
    DEBUG_PRINT("SIG_USART1_TRANS\n");
}
SIGNAL(SIG_EEPROM_READY)
{
    DEBUG_PRINT("SIG_EEPROM_READY\n");
}
SIGNAL(SIG_COMPARATOR)
{
    DEBUG_PRINT("SIG_COMPARATOR\n");
}
SIGNAL(SIG_SPM_READY)
{
    DEBUG_PRINT("SIG_SPM_READY\n");
}
#endif