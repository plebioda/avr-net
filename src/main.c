/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <avr/io.h>
#include <avr/iom162.h>
#include <avr/signal.h>
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

void timer_c(timer_t timer)
{
    DEBUG_PRINT("Timer %d\n",timer);
    timer_set(timer,5000+timer*1000);
}

void udp_callback(socket_t socket,uint8_t * data,uint16_t len)
{
    uint16_t i;
    uint8_t ret;
    uint8_t * tx_buffer = udp_get_buffer();
    DEBUG_PRINT("Socket %d response:\n",socket);
    for(i=0;i<len;i++)
    {
      data[i] -= 0x20;
      tx_buffer[i] = data[i];
      DEBUG_PRINT("%c",data[i]);
    }
    DEBUG_PRINT("\n");
    ret = udp_send(socket,len);
    DEBUG_PRINT("ret = %d\n",ret);
}	

int main(void)
{
  timer_init();
  DEBUG_INIT();
  spi_init(0);
  ethernet_address my_mac = {'<','P','A','K','O','>'};
  ip_address my_ip = {192,168,1,200};
  ethernet_init(&my_mac);
  ip_init(&my_ip);
  hal_init(my_mac);
  arp_init();
  udp_init();
  DEBUG_PRINT_COLOR(B_IRED,"Initialized...\n");  
  /* (clk = 8Mhz) / 256 = 31.25 kHz -> 32 us */
  TCNT0 = 0xffff;
  TCCR0 |= (1<<CS02) | (0<<CS01) | (0<<CS00);
  TIMSK = (1<<TOIE0);
// 
  sei();
//   timer_t timer = timer_alloc(timer_c);
//   timer_set(timer,1000);
  ip_address ip = {192,168,1,18};
  ethernet_address mac = {0,0,0,0,0,0};
//   uint8_t found = 0;
  arp_get_mac((const ip_address*)&ip,&mac);
  socket_t socket = udp_socket_alloc(12348,udp_callback);
//   udp_bind_remote(socket,12341,&ip);
  DEBUG_PRINT("socket = %d\n",socket);
  for(;;)
  {
	ethernet_handle_packet();
// 	if(!found && arp_get_mac(&ip,&mac))
// 	{
// 	    found = 1;
// 	    DEBUG_PRINT_COLOR(B_IYELLOW,"Requested mac: %x:%x:%x:%x:%x:%x\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
// 	}
  }
  return 0;
}

SIGNAL(SIG_OVERFLOW0)
{
    /* 10 ms / 32 us = 312.5 */
    TCNT0 = 312;
    timer_tick();    
}
// SIGNAL(SIG_OVERFLOW0)
// {
//     DEBUG_PRINT("SIG_OVERFLOW0\n");
// }
// SIGNAL(SIG_INTERRUPT0)
// {
//     DEBUG_PRINT("SIG_INTERRUPT0\n");
// }
// 
// SIGNAL(SIG_INTERRUPT1)
// {
//     DEBUG_PRINT("SIG_INTERRUPT1\n");
// }
// SIGNAL(SIG_INTERRUPT2)
// {
//     DEBUG_PRINT("SIG_INTERRUPT2\n");
// }
// SIGNAL(SIG_PIN_CHANGE0)
// {
//     DEBUG_PRINT("SIG_PIN_CHANGE0\n");
// }
// SIGNAL(SIG_PIN_CHANGE1)
// {
//     DEBUG_PRINT("SIG_PIN_CHANGE1\n");
// }
// SIGNAL(SIG_INPUT_CAPTURE3)
// {
//     DEBUG_PRINT("SIG_INPUT_CAPTURE3\n");
// }
// SIGNAL(SIG_OUTPUT_COMPARE3A)
// {
//     DEBUG_PRINT("SIG_OUTPUT_COMPARE3A\n");
// }
// 
// SIGNAL(SIG_OUTPUT_COMPARE3B)
// {
//     DEBUG_PRINT("SIG_OUTPUT_COMPARE3B\n");
// }
// 
// SIGNAL(SIG_OVERFLOW3)
// {
//     DEBUG_PRINT("SIG_OVERFLOW3\n");
// }
// SIGNAL(SIG_OUTPUT_COMPARE2)
// {
//     DEBUG_PRINT("SIG_OUTPUT_COMPARE2\n");
// }
// SIGNAL(SIG_OVERFLOW2)
// {
//     DEBUG_PRINT("SIG_OVERFLOW2\n");
// }
// SIGNAL(SIG_INPUT_CAPTURE1)
// {
//     DEBUG_PRINT("SIG_INPUT_CAPTURE1\n");
// }
// SIGNAL(SIG_OUTPUT_COMPARE1A)
// {
//     DEBUG_PRINT("SIG_OUTPUT_COMPARE1A\n");
// }
// SIGNAL(SIG_OUTPUT_COMPARE1B)
// {
//     DEBUG_PRINT("SIG_OUTPUT_COMPARE1B\n");
// }
// SIGNAL(SIG_OVERFLOW1)
// {
//     DEBUG_PRINT("SIG_OVERFLOW1\n");
// }
// SIGNAL(SIG_OUTPUT_COMPARE0)
// {
//     DEBUG_PRINT("SIG_OUTPUT_COMPARE0\n");
// }
// 
// SIGNAL(SIG_SPI)
// {
//     DEBUG_PRINT("SIG_SPI\n");
// }
// SIGNAL(SIG_USART0_RECV)
// {
//     DEBUG_PRINT("SIG_USART0_RECV\n");
// }
// SIGNAL(SIG_USART1_RECV)
// {
//     DEBUG_PRINT("SIG_USART1_RECV\n");
// }
// SIGNAL(SIG_USART0_DATA)
// {
//     DEBUG_PRINT("SIG_USART0_DATA\n");
// }
// SIGNAL(SIG_USART1_DATA)
// {
//     DEBUG_PRINT("SIG_USART1_DATA\n");
// }
// SIGNAL(SIG_USART0_TRANS)
// {
//     DEBUG_PRINT("SIG_USART0_TRANS\n");
// }
// SIGNAL(SIG_USART1_TRANS)
// {
//     DEBUG_PRINT("SIG_USART1_TRANS\n");
// }
// SIGNAL(SIG_EEPROM_READY)
// {
//     DEBUG_PRINT("SIG_EEPROM_READY\n");
// }
// SIGNAL(SIG_COMPARATOR)
// {
//     DEBUG_PRINT("SIG_COMPARATOR\n");
// }
// SIGNAL(SIG_SPM_READY)
// {
//     DEBUG_PRINT("SIG_SPM_READY\n");
// }

