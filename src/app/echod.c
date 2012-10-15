/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/**
 * @addtogroup app
 * @{
 * @addtogroup echo
 * @{
 * @file echod.c 
 * @author Pawel Lebioda <pawel.lebioda89@gmail.com>
 * @brief This file contains echo server implementation
 */

#include "echod.h"

#if ECHO_USE_TCP
#include "../net/tcp.h"
#else
#include "../net/udp.h"
#endif

#include "../sys/timer.h"

#include "../debug.h"

/**
 * Echo server context structure
 */ 
struct echo_server
{
#if ECHO_USE_TCP
	/**
	 * TCP Socket
	 */ 
	tcp_socket_t socket;
#else
	/**
	 * USP Socket
	 */ 
	udp_socket_t socket;
#endif
	/**
	 * Echo server buffer
	 */
	uint8_t buff[ECHO_BUFF_SIZE];
} echod;

#if ECHO_USE_TCP
static void echo_incoming(tcp_socket_t socket,enum tcp_event event);
#else
static void echo_incoming(udp_socket_t socket,uint8_t * data,uint16_t length);
#endif


uint8_t echod_start(void)
{
#if ECHO_USE_TCP
	tcp_socket_t socket = tcp_socket_alloc(echo_incoming);
#else
	udp_socket_t socket = udp_socket_alloc(ECHO_LOCAL_PORT,echo_incoming);
#endif
	if(socket < 0)
		return ECHO_ERR_SOCKET;
	echod.socket = socket;
	DBG_INFO("socketd %d socket %d\n",echod.socket,socket);
#if ECHO_USE_TCP
	tcp_listen(socket,ECHO_LOCAL_PORT);
#else
#endif
	return 0;
}

uint8_t echod_stop(void)
{
	return 0;
}

#if ECHO_USE_TCP
/**
 * TCP socket callback
 * @param [in] socket TCP socket
 * @param [in] event TCP event type
 */ 
void echo_incoming(tcp_socket_t socket,enum tcp_event event)
{
	DBG_INFO("echo incoming event = %d\n",event);
	switch(event)
	{
		case tcp_event_connection_incoming:
			tcp_accept(socket);
			break;
		case tcp_event_error:
		case tcp_event_timeout:
		case tcp_event_connection_established:
			break;
		case tcp_event_reset:
		case tcp_event_data_received:
		{
			DBG_INFO("Data received\n");
			int16_t len;
			while((len=tcp_read(socket,echod.buff,ECHO_BUFF_SIZE))>0)
				tcp_write(socket,echod.buff,len);
			if(len<0)
			{
				/*TODO*/
				tcp_close(socket);
				return;
			}
			break;
		}
		case tcp_event_connection_closed:
			tcp_socket_free(socket);
			break;
		case tcp_event_data_acked:
		case tcp_event_connection_closing:
		case tcp_event_connection_idle:
			break;
		case tcp_event_nop:
		default:
			/*TODO*/
			break;
	}
}

#else
/**
 * USP socket callback
 * @param [in] socket UDP socket
 * @param [in] data Pointer to data
 * @param [in] length Data length
 */ 
void echo_incoming(udp_socket_t socket,uint8_t * data,uint16_t length)
{
	/*TODO*/
}
#endif //ECHO_USE_TCP

/**
 * @}
 * @}
 */ 
