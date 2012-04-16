/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <avr/io.h>
#include <avr/interrupt.h>

#include <string.h>
#include <stdio.h>

#define DEBUG_MODE
#include "debug.h"

#include "arch/interrupts.h"
#include "arch/exmem.h"
#include "arch/spi.h"
#include "arch/uart.h"
#include "arch/i2c.h"

#include "util/fifo.h"

#include "dev/ds1338.h"
#include "dev/enc28j60.h"
#include "dev/sd.h"

#include "sys/timer.h"
#include "sys/rtc.h"

#include "net/hal.h"
#include "net/ethernet.h"
#include "net/ip.h"
#include "net/arp.h"
#include "net/udp.h"
#include "net/tcp.h"

#include "app/app_config.h"
#include "app/tftp.h"
#include "app/tp.h"
#include "app/dhcp.h"
#include "app/echod.h"
#include "app/netstat.h"

void ds1338_print_time(struct date_time * dt);
void udp_callback(udp_socket_t socket,uint8_t * data,uint16_t length)
{
  
}
void tcallback(timer_t timer,void * arg)
{
  static struct date_time dt;
  DEBUG_PRINT_COLOR(B_IBLUE,"ds1338_time: ");
  ds1338_get_date_time(&dt);
  ds1338_print_time(&dt);
  timer_set(timer,30000);
}


void tpcallback(uint8_t status,uint32_t timeval)
{
  static struct date_time date_time;
  DEBUG_PRINT_COLOR(B_IYELLOW,"tp status = %d\n",status);
  if(status)
    return;
  DEBUG_PRINT_COLOR(B_IYELLOW,"tp time sync\n");
  rtc_convert_date_time(timeval,&date_time);
  ds1338_set_date_time(&date_time);
  ds1338_print_time(&date_time);

}

static char * days[7] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};



void ds1338_print_time(struct date_time * dt)
{
  DEBUG_PRINT("%02x:%02x:%02x",dt->hours,dt->minutes,dt->seconds);
  DEBUG_PRINT(" %s ",days[dt->day-1]);
  DEBUG_PRINT("%02x.%02x.%02x",dt->date,dt->month,dt->year);
  DEBUG_PRINT(" format = %02x\n",dt->format);
}

void dhcpcallback(enum dhcp_event event)
{
  DEBUG_PRINT_COLOR(B_IGREEN,"[dhcp]: ");
  switch(event)
  {
    case dhcp_event_lease_acquired:
      DEBUG_PRINT_COLOR(GREEN,"lease acquired\n");
      break;
    case dhcp_event_lease_denied:
      DEBUG_PRINT_COLOR(RED,"lease denied\n");
      break;
    case dhcp_event_lease_expired:
      DEBUG_PRINT_COLOR(B_IRED,"lease expired\n");
      break;
    case dhcp_event_lease_expiring:
      DEBUG_PRINT_COLOR(GREEN,"lease expiring\n");
      break;
    case dhcp_event_error:
    default:
      DEBUG_PRINT_COLOR(B_IRED,"error\n");
      break;
  }
}

void echocallback(enum echo_event event)
{
 DEBUG_PRINT_COLOR(B_IBLUE,"Echo callback: event = %d\n",event); 
}

void sdcallback(enum sd_event event)
{
  DEBUG_PRINT_COLOR(B_IBLUE,"[sd]: ");
  switch(event)
  {
    case sd_event_inserted:
      DEBUG_PRINT_COLOR(IBLUE,"card inserted");
      break;
    case sd_event_inserted_wp:
      DEBUG_PRINT_COLOR(IBLUE,"card inserted");
      DEBUG_PRINT_COLOR(IRED," (Write Protection)");
      break;
    case sd_event_removed:
      DEBUG_PRINT_COLOR(IBLUE,"card removed");
      break;
    case sd_event_initialized:
      DEBUG_PRINT_COLOR(IBLUE,"card initialized");
      break;
    case sd_event_error:
      DEBUG_PRINT_COLOR(B_IRED,"error %x",sd_errno());
      break;
    default:
      DEBUG_PRINT_COLOR(B_IRED,"unknown event");
      break;
  }
  DEBUG_PRINT("\n");
}
void netstat_callback(timer_t timer,void * arg)
{
  netstat(stdout,NETSTAT_OPT_ALL);
  timer_set(timer,5000);
}
void sdtimer_callback(timer_t timer,void * arg)
{
  sd_interrupt();
  timer_set(timer,2000);
}

int main(void)
{
  /* constants */
  const ethernet_address my_mac = {'<','P','A','K','O','>'};
//   const ip_address my_ip = {192,168,1,200};
  
  /* init io */
  DDRB = 0xff;
  PORTB = 0xff;
  DDRE &= ~(1<<7);
  PORTE |= (1<<7);
  
  /* init interrupts */
  interrupt_timer0_init();
  interrupt_exint_init();
  
  /* init arch */
  uart_init();
  spi_init();
  i2c_init();
  
  /* init utils */
  fifo_init();
  
  /* init dev */
  ds1338_init();
  enc28j60_init((uint8_t*)&my_mac); 
  
  /* init sys */
  timer_init();
  rtc_init((0<<RTC_FORMAT_12_24)|(0<<RTC_FORMAT_AM_PM));
  
  /* init net */
  ethernet_init(&my_mac);
  ip_init(0,0,0);
  arp_init();
  udp_init();
  tcp_init();
  

  
  uint8_t ret = echod_start(echocallback);
  DEBUG_PRINT_COLOR(B_IRED,"echo ret=%d\n",ret);
  /* global interrupt enable */
  sei();
  
  stdout = DEBUG_FH;
  
  DEBUG_PRINT("Hello ATMega128!\n");
  DEBUG_PRINT("ENC28J60 rev %d\n",enc28j60_get_revision());
  
  sd_init(sdcallback);
  sd_interrupt();
  
  timer_t sd_timer = timer_alloc(sdtimer_callback);
  timer_set(sd_timer,2000);
  timer_t netstat_timer = timer_alloc(netstat_callback);
  timer_set(netstat_timer,1);
  
//   timer_t rtc_timer = timer_alloc(tcallback);
//   timer_set(rtc_timer,1000);
  
//   uint8_t ret = dhcp_start(dhcpcallback);
//   DEBUG_PRINT("dhcp start ret=  %d\n",ret);
//   tp_get_time(tpcallback);
  udp_socket_t udps;
  udps = udp_socket_alloc(12348,udp_callback);
  
  for(;;)
  {
    
  }
  return 0;
}

ISR(TIMER0_COMP_vect)
{
  //1ms
  timer_tick();
}

ISR(INT7_vect)
{
  while(ethernet_handle_packet());
}

ISR(INT6_vect)
{
//   sd_interrupt();
}