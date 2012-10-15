/*
 * Copyright (c) 2012 by Pawel Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/**
 * @addtogroup app
 * @{
 * @addtogroup http
 * @{
 * @file httpd.c
 * @author Pawel Lebioda <pawel.lebioda89@gmail.com>
 * @brief HTTP server implementation
 */ 

#include "httpd.h"

#include "../net/tcp.h"

#include "../debug.h"

struct
{
	/**
	 * TCP Socket
	 */ 
	tcp_socket_t socket;
	
	/**
	 * Root directory
	 */ 
	struct fat_dir_entry * root_dir;
} httpd;

static void httpd_socket_callback(tcp_socket_t socket,enum tcp_event event);

uint8_t httpd_start(struct fat_dir_entry * root_dir)
{
	if(!root_dir)
	{
		DBG_ERROR("root_dir==NULL\n");
		return 0;
	}
	httpd.root_dir = root_dir;
	httpd.socket = tcp_socket_alloc(httpd_socket_callback);
	if(httpd.socket < 0)
	{
		DBG_ERROR("tcp_socket_alloc\n");
		return 0;
	}
	if(!tcp_listen(httpd.socket, HTTP_LISTEN_PORT))
	{
		DBG_ERROR("tcp_listen\n");
		httpd_stop();
		return 0;
	}
	DBG_INFO("httpd started\n");
	return 1;
}

uint8_t httpd_restart(void)
{
	return 1;
}

uint8_t httpd_stop(void)
{
	if(httpd.socket < 0)
	{
		tcp_socket_free(httpd.socket);
		httpd.socket = -1;
	}
	return 1;
}

void httpd_socket_callback(tcp_socket_t socket,enum tcp_event event)
{
	switch(event)
	{
	case tcp_event_connection_incoming:
		DBG_INFO("incomming connection\n");
	break;
	case tcp_event_error:	
		DBG_ERROR("error\n");
	break;
	case tcp_event_timeout:
		DBG_ERROR("timeout\n");
	break;
	case tcp_event_connection_established:
	break;
	case tcp_event_reset:
		DBG_ERROR("reset\n");
	break;
	case tcp_event_data_received:
	break;
	case tcp_event_connection_closed:
	break;
	case tcp_event_data_acked:
	break;
	case tcp_event_connection_closing:
	break;
	case tcp_event_connection_idle:
	break;
	case tcp_event_nop:
	break;
	default:
	break;
	}
}

/**
 * @}
 * @}
 */ 
