/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG_MODE
#include "../debug.h"

#include "udp.h"
#include "../arch/exmem.h"

#include <string.h>

struct udp_header
{
    uint16_t port_source;
    uint16_t port_destination;
    uint16_t length;
    uint16_t checksum;
};

struct udp_socket
{
    uint16_t port_local;
    uint16_t port_remote;
    ip_address ip_remote;
    udp_socket_callback callback;
};

static struct udp_socket udp_sockets[UDP_SOCKET_MAX] EXMEM;

#define FOREACH_UDP_SOCKET(socket) for((socket) = &udp_sockets[0] ; (socket) < &udp_sockets[UDP_SOCKET_MAX] ; (socket)++)

uint16_t udp_get_checksum(const ip_address * ip_addr,const struct udp_header * udp,uint16_t packet_len);
uint8_t udp_is_free_port(uint16_t port);
socket_t udp_socket_num(struct udp_socket * socket);
uint8_t udp_socket_is_valid(socket_t socket);

uint8_t udp_init(void)
{
    memset(udp_sockets,0,sizeof(udp_sockets));
}
uint8_t udp_socket_is_valid(socket_t socket)
{
    return (socket >= 0 && socket < UDP_SOCKET_MAX);
}
socket_t udp_socket_num(struct udp_socket * socket)
{
    if(!socket)
      return -1;
    socket_t socket_num = (socket_t)(((uint16_t)socket - (uint16_t)udp_sockets)/sizeof(struct udp_socket));
    if(udp_socket_is_valid(socket_num))
      return socket_num;
    return -1;
}

socket_t udp_socket_alloc(uint16_t local_port,udp_socket_callback callback)
{
  if(local_port == 0 || callback == 0)
    return -1;
  if(!udp_is_free_port(local_port))
    return -1;
  struct udp_socket * socket;
  socket_t socket_num = -1;
  FOREACH_UDP_SOCKET(socket)
  {
      if(socket->callback != 0)
	continue;
      socket_num = udp_socket_num(socket);
      if(socket_num<0)
	continue;
      memset(socket,0,sizeof(struct udp_socket));
      socket->callback = callback;
      socket->port_local = local_port;
      return socket_num;
  }
  return socket_num;
}

void udp_socket_free(socket_t socket_num)
{
  if(!udp_socket_is_valid(socket_num))
    return;
  memset(&udp_sockets[socket_num],0,sizeof(struct udp_socket));
}

uint8_t udp_is_free_port(uint16_t port)
{
    struct udp_socket * socket;
    FOREACH_UDP_SOCKET(socket)
    {
	if(socket->callback != 0 && socket->port_local == port)
	  return 0;
    }
    return 1;
}

uint16_t udp_get_checksum(const ip_address * ip_addr,const struct udp_header * udp,uint16_t packet_len)
{
    uint16_t checksum = IP_PROTOCOL_UDP + packet_len;
    checksum = net_get_checksum(checksum,(const uint8_t*)ip_get_addr(),sizeof(ip_address),4);
    checksum = net_get_checksum(checksum,(const uint8_t*)ip_addr,sizeof(ip_address),4);
    
    checksum = net_get_checksum(checksum,(const uint8_t*)udp,packet_len,6);
    return ~checksum;
}

uint8_t udp_handle_packet(const ip_address * ip_remote,const struct udp_header * udp,uint16_t packet_len)
{	
    if(packet_len < sizeof(struct udp_header))
      return 0;
    /*check checksum */
    if(udp_get_checksum(ip_remote,udp,packet_len) != ntoh16(udp->checksum))
      return 0;
    /* get local port number */
    uint16_t port_local = ntoh16(udp->port_destination);

    /* get remote port number */
    uint16_t port_remote = ntoh16(udp->port_source);
    
    if(port_local == 0 || port_remote == 0)
      return 0;
    struct udp_socket * socket;
    socket_t socket_num = -1;
    FOREACH_UDP_SOCKET(socket)
    {
	if(socket->callback == 0 || socket->port_local != port_local)
	  continue;
	socket_num = udp_socket_num(socket);
	if(socket_num < 0)
	  continue;
	memcpy(&socket->ip_remote,ip_remote,sizeof(ip_address));
	socket->port_remote = udp->port_source;
	socket->callback(socket_num,(uint8_t*)udp + sizeof(struct udp_header),packet_len-sizeof(struct udp_header));
    }
}

uint8_t udp_send(socket_t socket,uint16_t length)
{
    if(length < 1 || !udp_socket_is_valid(socket))
      return 0;
    
}