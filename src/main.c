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

// uint8_t data[1500];
 // 	len = tcp_write_string_P(socket,
// 				 PSTR("HTTP/1.1 200 OK\r\n"
// 				 "Content-Type: text/html\r\n"
// // 				 "Content-Length: 65\r\n"
// 				 "<html>"
// 				 "<body"
// 				 "><h1>Hello avr-net!</h1>"
// 				 "<p>ATMega162</p>"
// 				 "</body>"
// 				 "</html>"));


// void tcp_callback(tcp_socket_t socket,enum tcp_event event)
// {
//     static uint16_t counter = 0;
// //     DEBUG_PRINT("tcpcallbck:soc%d event:",socket);
//     int16_t len;
//     uint8_t * ptr;
//     switch(event)
//     {
//       case tcp_event_nop:
// 	break;
//       case tcp_event_connection_established:
// 	counter = 0;
// 	DEBUG_PRINT("Con.est.,port=%u\n",tcp_get_remote_port(socket));
// 	break;
//       case tcp_event_connection_incoming:
// 	DEBUG_PRINT("Con.inc.,port=%u\n",tcp_get_remote_port(socket));
// 	tcp_accept(socket);
// 	break;
//       case tcp_event_error:
// 	DEBUG_PRINT("Error\n");
// 	tcp_socket_free(socket);
// 	break;
//       case tcp_event_timeout:
// 	DEBUG_PRINT("Timeout\n");
// // 	tcp_connect(socket,&ip_remote,80);
// // 	tcp_socket_free(socket);
// 	break;
//       case tcp_event_data_received:
//       {
// 	len = tcp_read(socket,data,1500);
// 	DEBUG_PRINT("Data received len = %d:\n",len);
// 	if(len <= 0)
// 	  break;	
// 	ptr = data;
// 	if(*ptr == 0xff)
// 	  break;
// 	counter += len;
// 	uint16_t i = len;
// 	while(i--)
// 	{
// // 	    DEBUG_PRINT("%c",*(ptr));
// 	    if(*ptr >= 'a' && *ptr <= 'z')
// 	      *ptr -= 0x20;
// 	    ptr++;
// 	}
// 	tcp_write(socket,data,len);
// 	if(counter > 2000)
// 	  tcp_close(socket);
// 	break;
//       }
//       case tcp_event_connection_closing:
// 	DEBUG_PRINT("connection closing\n");
// 	break;
//       case tcp_event_connection_closed:
// 	DEBUG_PRINT("connection closed\n");
// 	tcp_socket_free(socket);
// 	break;
//       case tcp_event_data_acked:
// 	DEBUG_PRINT("data acked\n");
// 	break;
//       default:
// 	DEBUG_PRINT("event=%d\n",event);
// 	break;
//     }
// }
// 


void ds1338_print_time(struct date_time * dt);
void tcallback(timer_t timer,void * arg)
{
  static struct date_time dt;
  DEBUG_PRINT_COLOR(B_IBLUE,"ds1338_time: ");
  ds1338_get_date_time(&dt);
  ds1338_print_time(&dt);
  timer_set(timer,30000);
}

void udp_callback(udp_socket_t socket,uint8_t * data,uint16_t len);
void ds1338_print_time(struct date_time * dt);
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
int main(void)
{
  /* constants */
  const ethernet_address my_mac = {'<','P','A','K','O','>'};
//   const ip_address my_ip = {192,168,1,200};
  
  /* init io */
  DDRB = 0xff;
  PORTB = 0xff;
  
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
  
  /* global interrupt enable */
  sei();
  
  DEBUG_PRINT("Hello ATMega128!\n");
  DEBUG_PRINT("ENC28J60 rev %d\n",enc28j60_get_revision());
  
  

//   timer_t rtc_timer = timer_alloc(tcallback);
//   timer_set(rtc_timer,1000);
  
//   uint8_t ret = dhcp_start(dhcpcallback);
//   DEBUG_PRINT("dhcp start ret=  %d\n",ret);
  tp_get_time(tpcallback);
//   udp_socket_t udps;
//   udps = udp_socket_alloc(12348,udp_callback);
  
  for(;;)
  {

  }
  return 0;
}

SIGNAL(SIG_OUTPUT_COMPARE0)
{
  //1ms
  timer_tick();
}

SIGNAL(SIG_INTERRUPT7)
{
  ethernet_handle_packet();
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