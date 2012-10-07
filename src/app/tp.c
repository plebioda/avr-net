/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "tp.h"

#include "../net/udp.h"
#include "../net/tcp.h"
#include "../net/arp.h"

#include "../sys/timer.h"

#include <string.h>

#define DEBUG_MODE
#include "../debug.h"


/* 
RFC 868: Time Protocol

This RFC specifies a standard for the ARPA Internet community.	Hosts on
the ARPA Internet that choose to implement a Time Protocol are expected
to adopt and implement this standard.

This protocol provides a site-independent, machine readable date and
time.	The Time service sends back to the originating source the time in
seconds since midnight on January first 1900.

One motivation arises from the fact that not all systems have a
date/time clock, and all are subject to occasional human or machine
error.	The use of time-servers makes it possible to quickly confirm or
correct a system's idea of the time, by making a brief poll of several
independent sites on the network.

This protocol may be used either above the Transmission Control Protocol
(TCP) or above the User Datagram Protocol (UDP).

When used via TCP the time service works as follows:

	 S: Listen on port 37 (45 octal).

	 U: Connect to port 37.

	 S: Send the time as a 32 bit binary number.

	 U: Receive the time.

	 U: Close the connection.

	 S: Close the connection.

	 The server listens for a connection on port 37.	When the connection
	 is established, the server returns a 32-bit time value and closes the
	 connection.	If the server is unable to determine the time at its
	 site, it should either refuse the connection or close it without
	 sending anything.

RFC 868																												 May 1983
Time Protocol																													 

When used via UDP the time service works as follows:

	 S: Listen on port 37 (45 octal).

	 U: Send an empty datagram to port 37.

	 S: Receive the empty datagram.

	 S: Send a datagram containing the time as a 32 bit binary number.

	 U: Receive the time datagram.

	 The server listens for a datagram on port 37.	When a datagram
	 arrives, the server returns a datagram containing the 32-bit time
	 value.	If the server is unable to determine the time at its site, it
	 should discard the arriving datagram and make no reply.

The Time

The time is the number of seconds since 00:00 (midnight) 1 January 1900
GMT, such that the time 1 is 12:00:01 am on 1 January 1900 GMT; this
base will serve until the year 2036.
*/

enum tp_state
{
	tp_state_idle=0,
	tp_state_wait_for_reply,
	tp_state_wait_for_conn,
	tp_state_wait_for_data,
	tp_state_wait_closing
};

static struct
{
#if TP_USE_TCP
	tcp_socket_t socket;
 #else
	udp_socket_t socket;
	uint8_t rtx;
 #endif
	timer_t timer;
	tp_callback callback;
	enum tp_state state;
} tpc;

static ip_address tp_server_ip = TP_SERVER_IP;
 
static void tp_timer_callback(timer_t timer,void * arg);
static void tp_reset(void);
#if TP_USE_TCP
static void tp_socket_callback(tcp_socket_t socket,enum tcp_event event);
#else
static void tp_socket_callback(udp_socket_t socket,uint8_t * data,uint16_t len);
#endif 

uint8_t tp_get_time(tp_callback callback)
{
	if(!callback)
		return TP_ERR_CALLBACK;
	memset(&tpc,0,sizeof(tpc));
	tpc.timer=-1;
	tpc.socket=-1;
#if TP_USE_TCP
	tcp_socket_t socket = tcp_socket_alloc(tp_socket_callback);
#else
	udp_socket_t socket = udp_socket_alloc(UDP_PORT_ANY,tp_socket_callback);
#endif
	if(socket < 0)
		return TP_ERR_SOCKET;
	timer_t timer = timer_alloc(tp_timer_callback);
	if(timer < 0)
	{
 #if TP_USE_TCP
		tcp_socket_free(socket);
#else
		udp_socket_free(socket);
#endif	 
		return TP_ERR_TIMER;
	}
	tpc.callback = callback;
	tpc.socket = socket;
	tpc.timer = timer;
#if TP_USE_TCP
	tcp_connect(tpc.socket,&tp_server_ip,TP_REMOTE_PORT);
	tpc.state = tp_state_wait_for_conn;
	timer_set(timer,TP_CONN_TIMEOUT);
#else
	udp_bind_remote(tpc.socket,TP_REMOTE_PORT,(const ip_address*)&tp_server_ip);
	udp_send(tpc.socket,0);
	tpc.state = tp_state_wait_for_reply;
	timer_set(timer,TP_ARP_TIMEOUT);
#endif
	return 0;
}
void tp_timer_callback(timer_t timer,void * arg)
{
	if(timer != tpc.timer)
	{
		tpc.callback(TP_ERR_CONTEXT,0);
		tp_reset();
		return;
	}
	DEBUG_PRINT("tp timer state =%d\n",tpc.state);
	switch(tpc.state)
	{
		case tp_state_idle:
			DEBUG_PRINT_COLOR(B_IYELLOW,"tp state idle\n");
			break;
#if TP_USE_TCP			
		case tp_state_wait_for_conn:
		{
			DEBUG_PRINT_COLOR(B_IYELLOW,"tp state wait for conn\n");
			tcp_close(tpc.socket);
			tpc.callback(TP_ERR_TIMEOUT,0);
			tp_reset();
			break;
		}
		case tp_state_wait_for_data:
		{
			DEBUG_PRINT_COLOR(B_IYELLOW,"tp state wait for data\n");
			tcp_close(tpc.socket);
			tpc.callback(TP_ERR_TIMEOUT,0);
			tp_reset();
			break;
		}
		case tp_state_wait_closing:
		{
			tp_reset();
			break;	
		}
#else
		case tp_state_wait_for_reply:
		{
		 DEBUG_PRINT_COLOR(B_IYELLOW,"tp state wait for reply\n");
		 if(tpc.rtx++ >= TP_RTX_MAX)
		 {
			 tpc.callback(TP_ERR_TIMEOUT,0);
			 tp_reset();
			 return;
		 }
		 udp_send(tpc.socket,0);
		 timer_set(timer,TP_RTX_TIMEOUT);
		 break;
		}
#endif
		default:
		{
			tpc.callback(TP_ERR_STATE,0);
			tp_reset();
			return; 
		}
	}
}
#if TP_USE_TCP
static void tp_socket_callback(tcp_socket_t socket,enum tcp_event event)
{
	if(socket != tpc.socket)
	{
		tpc.callback(TP_ERR_SOCKET,0);
		tp_reset();
		return;
	}
	DEBUG_PRINT_COLOR(B_IYELLOW,"tp event %d\n",event);
	switch(event)
	{
		case tcp_event_connection_established:
			DEBUG_PRINT_COLOR(B_IYELLOW,"tp con established\n",event);
			timer_set(tpc.timer,TP_RTX_TIMEOUT);
			break;
		case tcp_event_data_received:
		{
			timer_stop(tpc.timer);
			uint8_t data[4];
			tcp_read(socket,data,4);
			uint32_t timeval = ntoh32(*((uint32_t*)data));
			tpc.callback(0,timeval);
			tcp_close(socket);
			tpc.state = tp_state_wait_closing;
			timer_set(tpc.timer,TP_RTX_TIMEOUT);
			break;
		}
		case tcp_event_connection_closed:
			tp_reset();
		default:
			break;
	}
}
#else //TP_USE_TCP
void tp_socket_callback(udp_socket_t socket,uint8_t * data,uint16_t len)
{
	if(socket != tpc.socket)
	{
		tpc.callback(TP_ERR_SOCKET,0);
		tp_reset();
		return;		
	}
	timer_stop(tpc.timer);
	if(tpc.state != tp_state_wait_for_reply)
	{
		tpc.callback(TP_ERR_STATE,0);
		tp_reset();
		return;
	}
	uint32_t timeval = ntoh32(*((uint32_t*)data));
	tpc.callback(0,timeval);
	tp_reset();
}
#endif //TP_USE_TCP
void tp_reset(void)
{
#if TP_USE_TCP
	tcp_socket_free(tpc.socket);
#else
	udp_socket_free(tpc.socket);
	tpc.rtx=0;
#endif
	timer_free(tpc.timer);
	memset(&tpc,0,sizeof(tpc));
	tpc.timer=-1;
	tpc.socket=-1;
}

