/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include "echod.h"

#if ECHO_USE_TCP
#include "../net/tcp.h"
#else
#include "../net/udp.h"
#endif

#include "../sys/timer.h"

#define DEBUG_MODE
#include "../debug.h"

struct 
{
#if ECHO_USE_TCP
	tcp_socket_t socket;
#else
	udp_socket_t socket;
#endif
	echo_callback callback;
	uint8_t buff[ECHO_BUFF_SIZE];
} echod;

#if ECHO_USE_TCP
void echo_incoming(tcp_socket_t socket,enum tcp_event event);
#else
void echo_incoming(udp_socket_t socket,uint8_t * data,uint16_t length);
#endif


uint8_t echod_start(echo_callback callback)
{
	if(!callback)
		 return ECHO_ERR_CALLBACK;
#if ECHO_USE_TCP
	tcp_socket_t socket = tcp_socket_alloc(echo_incoming);
#else
	udp_socket_t socket = udp_socket_alloc(ECHO_LOCAL_PORT,echo_incoming);
#endif
	if(socket < 0)
		return ECHO_ERR_SOCKET;
	echod.socket = socket;
	DEBUG_PRINT_COLOR(B_IRED,"socketd %d socket %d\n",echod.socket,socket);
#if ECHO_USE_TCP
	tcp_listen(socket,ECHO_LOCAL_PORT);
#else
#endif
	echod.callback = callback;
	return 0;
}

uint8_t echod_stop(void)
{
	return 0;
}

void echo_incoming(tcp_socket_t socket,enum tcp_event event)
{
	DEBUG_PRINT_COLOR(B_IRED,"echo incoming event = %d\n",event);
	switch(event)
	{
		case tcp_event_connection_incoming:
			tcp_accept(socket);
			break;
		case tcp_event_error:
		case tcp_event_timeout:
		case tcp_event_connection_established:
			echod.callback(echo_event_established);
			break;
		case tcp_event_reset:
		case tcp_event_data_received:
		{
			int16_t len;
			while((len=tcp_read(socket,echod.buff,ECHO_BUFF_SIZE))>0)
				tcp_write(socket,echod.buff,len);
			if(len<0)
			{
				/*TODO*/
				echod.callback(echo_event_buffer);
				tcp_close(socket);
				return;
			}
			echod.callback(echo_event_data_rcv);
			break;
		}
		case tcp_event_connection_closed:
			echod.callback(echo_event_closed);
			tcp_socket_free(socket);
			break;
		case tcp_event_data_acked:
		case tcp_event_connection_closing:
		case tcp_event_connection_idle:
			break;
		case tcp_event_nop:
		default:
			/*TODO*/
			echod.callback(echo_event_error);
			break;
	}
}
