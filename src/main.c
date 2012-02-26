/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <avr/io.h>
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

void timer_c(timer_t timer)
{
    DEBUG_PRINT("Timer %d\n",timer);
    timer_set(timer,5000+timer*1000);
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
  DEBUG_PRINT_COLOR(B_IRED,"Initialized...\n");  
  /* (clk = 8Mhz) / 256 = 31.25 kHz -> 32 us */
  TCNT0 = 0xffff;
  TCCR0 |= (1<<CS02) | (0<<CS01) | (0<<CS00);
  TIMSK = (1<<TOIE0);

  sei();
//   timer_t timer = timer_alloc(timer_c);
//   timer_set(timer,1000);
  ip_address ip = {192,168,1,18};
  ethernet_address mac = {0,0,0,0,0,0};
//   uint8_t found = 0;
  arp_get_mac((const ip_address*)&ip,&mac);
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
