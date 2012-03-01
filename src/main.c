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


void udp_callback(udp_socket_t socket,uint8_t * data,uint16_t len);
void tcp_callback(tcp_socket_t socket,enum tcp_event event)
{
    DEBUG_PRINT("tcp callback: socket %d event %d\n",socket,event);
    
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
  
  /* Timer 1 init*/
  /* (clk = 8Mhz) / 256 = 31.25 kHz -> 32 us */
  TCNT1 = 0xffff;
  TCCR1B |= (1<<CS12) | (0<<CS11) | (0<<CS10);
  TIMSK = (1<<TOIE1);
  sei();
  
  udp_socket_t socket = udp_socket_alloc(12348,udp_callback);
  
#if APP_TFTP
  tftpd_init();
#endif
  tcp_init();
  DEBUG_PRINT_COLOR(B_IYELLOW,"Initialized...\n");  
  uint8_t ret;
  tcp_socket_t tcp_socket;
  tcp_socket = tcp_socket_alloc(tcp_callback);
  DEBUG_PRINT("socket = %d\n",tcp_socket);
  ret = tcp_listen(tcp_socket,80);
  DEBUG_PRINT("listen = %d\n",ret);
  for(;;)
  {
	ethernet_handle_packet();
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