/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <avr/io.h>
#include <avr/interrupt.h>

#define DEBUG_MODE
#include "debug.h"

#include "arch/exmem.h"
#include "arch/spi.h"
#include "arch/uart.h"
#include "arch/i2c.h"

#include "sys/timer.h"

#include "util/fifo.h"

#include "net/hal.h"
#include "net/ethernet.h"
#include "net/ip.h"
#include "net/arp.h"
#include "net/udp.h"
#include "net/tcp.h"

#include "app/app_config.h"
#include "app/tftp.h"

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
// void udp_callback(udp_socket_t socket,uint8_t * data,uint16_t len);

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
void tcallback(timer_t timer,void * arg)
{
    PORTD ^= 1<<7;
    timer_set(timer,10);
}

int main(void)
{
  timer_init();
  DDRD |= 1<<7;
  DDRB = 0xff;
  PORTB = 0xff;
  DDRE = 0xff;
  PORTE = 0xff;
  DEBUG_INIT();
  spi_init(0);
  const ethernet_address my_mac = {'<','P','A','K','O','>'};
//   const ip_address my_ip = {192,168,1,200};
//   fifo_init();
//   hal_init((const uint8_t*)my_mac);
//   ethernet_init(&my_mac);
//   ip_init(&my_ip);  
//   arp_init();
  TCNT0 = 0;
  TCCR0 |= (1<<WGM01)|(0<<WGM00)|(0<<CS02)|(1<<CS01)|(1<<CS00);
  OCR0 = 31;
  TIMSK |= (1<<OCIE0)|(0<<TOIE0);
  sei();
  timer_t timer = timer_alloc(tcallback);  
  timer_set(timer,100);
//   const ip_address rem_ip = {192,168,1,16};
  DEBUG_PRINT("Hello ATMega128!\n");
//   DEBUG_PRINT("ENC28J60 rev %d\n",enc28j60_get_revision());
  i2c_init();
  uint8_t i2cbuff[13];
  uint8_t rtc_addr=0;
  DEBUG_PRINT("i2c twbr=%d\n",TWBR);
  uint8_t i2cret;
  i2cbuff[0]=7;
  i2cbuff[1]=0x13;
  i2cret = i2c_write(0xd0,i2cbuff,2);
  for(;;)
  {
    i2cret = i2c_write(0xd0,&rtc_addr,1);
    DEBUG_PRINT("i2c_write ret=%d\n",i2cret);
    i2cret = i2c_read(0xd0,i2cbuff,12);
    DEBUG_PRINT("i2c_read ret=%d\n",i2cret);
    for(i2cret = 0 ; i2cret < 8 ; i2cret++)
      DEBUG_PRINT("%x|",i2cbuff[i2cret]);
    DEBUG_PRINT("\n");
    _delay_ms(200);
    _delay_ms(200);
    _delay_ms(200);
    _delay_ms(200);
    _delay_ms(200);
    _delay_ms(200);
    
  }
  return 0;
}
SIGNAL(SIG_OUTPUT_COMPARE0)
{
  //1ms
  timer_tick();
}



#if NET_UDP
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
#endif //NET_UDP




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