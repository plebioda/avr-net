/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "udp.h"

#if NET_UDP

#define DEBUG_MODE
#include "../debug.h"

//#include "../arch/exmem.h"

#include <string.h>

/*
*  0      7 8     15 16    23 24    31  
* +--------+--------+--------+--------+ 
* |     Source      |   Destination   | 
* |      Port       |      Port       | 
* +--------+--------+--------+--------+ 
* |                 |                 | 
* |     Length      |    Checksum     | 
* +--------+--------+--------+--------+ 
* 
*/
struct udp_header
{
    uint16_t port_source;
    uint16_t port_destination;
    uint16_t length;
    uint16_t checksum;
};

struct udp_socket
{
    udp_socket_callback callback;
    uint16_t port_local;
    uint16_t port_remote;
    ip_address ip_remote;
};

static struct udp_socket udp_sockets[UDP_SOCKET_MAX]; // EXMEM

#define FOREACH_UDP_SOCKET(socket) for((socket) = &udp_sockets[0] ; (socket) < &udp_sockets[UDP_SOCKET_MAX] ; (socket)++)

uint16_t 	udp_get_checksum(const ip_address * ip_addr,const struct udp_header * udp,uint16_t packet_len);
uint8_t 	udp_is_free_port(uint16_t port);
udp_socket_t 	udp_socket_num(struct udp_socket * socket);
uint8_t 	udp_socket_is_valid(udp_socket_t socket);
uint16_t 	udp_get_free_local_port(void);

uint8_t udp_init(void)
{
    memset(udp_sockets,0,sizeof(udp_sockets));
    return 1;
}
uint8_t udp_socket_is_valid(udp_socket_t socket)
{
    return (socket >= 0 && socket < UDP_SOCKET_MAX);
}
udp_socket_t udp_socket_num(struct udp_socket * socket)
{
    if(!socket)
      return -1;
    udp_socket_t socket_num = (udp_socket_t)(((uint16_t)socket - (uint16_t)udp_sockets)/sizeof(struct udp_socket));
    if(udp_socket_is_valid(socket_num))
      return socket_num;
    return -1;
}

udp_socket_t udp_socket_alloc(uint16_t local_port,udp_socket_callback callback)
{
  if(!callback)
    return -1;
  if(local_port != UDP_PORT_ANY)
  {
    if(!udp_is_free_port(local_port))
      return -1;
  }
  else
  {
    local_port = udp_get_free_local_port();
  }
  struct udp_socket * socket;
  udp_socket_t socket_num = -1;
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

void udp_socket_free(udp_socket_t socket_num)
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
  /* UDP pseudo header = ip protocol + total udp packet length (header+data) + dst ip + src ip
        0      7 8     15 16    23 24    31 
       +--------+--------+--------+--------+
       |          source address           |
       +--------+--------+--------+--------+
       |        destination address        |
       +--------+--------+--------+--------+
       |  zero  |protocol|   UDP length    |
       +--------+--------+--------+--------+
    
  */
  uint16_t checksum = IP_PROTOCOL_UDP + packet_len;
  checksum = net_get_checksum(checksum,(const uint8_t*)ip_get_addr(),sizeof(ip_address),4);
  checksum = net_get_checksum(checksum,(const uint8_t*)ip_addr,sizeof(ip_address),4);
  /* udp header + data */
  checksum = net_get_checksum(checksum,(const uint8_t*)udp,packet_len,6);
  return ~checksum;
}

uint8_t udp_handle_packet(const ip_address * ip_remote,const struct udp_header * udp,uint16_t packet_len)
{	
  DEBUG_PRINT_COLOR(B_ICYAN,"udp handle\n");
    if(packet_len < sizeof(struct udp_header))
      return 0;
    DEBUG_PRINT_COLOR(B_ICYAN,"length ok\n");
    /*check checksum */
    if(udp_get_checksum(ip_remote,udp,packet_len) != ntoh16(udp->checksum))
      return 0;
    DEBUG_PRINT_COLOR(B_ICYAN,"checksum ok\n");
    /* get local port number */
    uint16_t port_local = ntoh16(udp->port_destination);
    /* get remote port number */
    uint16_t port_remote = ntoh16(udp->port_source);
    /* check port and remote ports*/
    if(port_local == 0 || port_remote == 0)
      return 0;
    DEBUG_PRINT_COLOR(B_ICYAN,"ports ok\n");
    struct udp_socket * socket;
    udp_socket_t socket_num = -1;
    DEBUG_PRINT_COLOR(B_ICYAN,"udp handle remote %d local %d\n",port_remote,port_local);
    FOREACH_UDP_SOCKET(socket)
    {
	/* check if socket is used and if whether packet is directed to this one*/
	if(socket->callback == 0 || socket->port_local != port_local)
	  continue;
	/* if socket has remote port binded check if is equal to src port*/
	if(socket->port_remote != 0 && socket->port_remote != port_remote)
	  continue;
	/* if socket has remote ip binded check if is equal to src ip*/
	if((socket->ip_remote[0] | 
	  socket->ip_remote[1] |
	  socket->ip_remote[2] |
	  socket->ip_remote[3]) != 0x00 && memcmp(&socket->ip_remote,ip_remote,sizeof(ip_address)) != 0)
	  continue;
	/* get socket_number */
	socket_num = udp_socket_num(socket);
	if(socket_num < 0)
	  continue;
	/* bind remote port and ip to socket so it can get this values later if needed */
	memcpy(&socket->ip_remote,ip_remote,sizeof(ip_address));
	socket->port_remote = port_remote;
	/* call callback function of socket */
	socket->callback(socket_num,(uint8_t*)udp + sizeof(struct udp_header),packet_len-sizeof(struct udp_header));
	return 1;
    }
    return 0;
}
uint8_t udp_bind_remote(udp_socket_t socket,uint16_t remote_port,const ip_address * remote_ip)
{
    if(!udp_socket_is_valid(socket))
      return 0;
    struct udp_socket * s = &udp_sockets[socket];
    memset(&s->ip_remote,0,sizeof(s->ip_remote));
    s->port_remote = remote_port;
    if(remote_ip)
      memcpy(&s->ip_remote,remote_ip,sizeof(ip_address));
    return 1;
}
uint8_t udp_unbind_remote(udp_socket_t socket)
{
    if(!udp_socket_is_valid(socket))
      return 0;
    struct udp_socket * s = &udp_sockets[socket];
    memset(&s->port_remote,0,sizeof(s->port_remote) + sizeof(s->ip_remote));
    return 1;
}
uint8_t udp_bind_local(udp_socket_t socket,uint16_t local_port)
{
    if(!udp_socket_is_valid(socket))
      return 0;
    struct udp_socket * s = &udp_sockets[socket];
    if(s->port_local == local_port)
      return 1;
    if(!udp_is_free_port(local_port))
      return 0;
    s->port_local = local_port;
    return 1;
}

uint16_t udp_get_free_local_port(void)
{
    uint16_t port = 1024;
    do
    {
      port++;
    }while(!udp_is_free_port(port) && port !=0); 
    return port;
}
uint8_t udp_send(udp_socket_t socket_num,uint16_t length)
{
    DEBUG_PRINT("udp send %d socket %d\n",length,socket_num);
    if(length < 0 || !udp_socket_is_valid(socket_num))
      return 0;
    struct udp_socket * socket = &udp_sockets[socket_num];
    if(socket->port_remote < 1)
      return 0;
    uint16_t packet_max_length = ip_get_buffer_size() - sizeof(struct udp_header);
    if(length > packet_max_length )
      length = packet_max_length;

    length += sizeof(struct udp_header);
    struct udp_header * udp = (struct udp_header*)ip_get_buffer();
    udp->length = hton16(length);
    udp->port_destination = hton16(socket->port_remote);
    udp->port_source = hton16(socket->port_local);
    udp->checksum = hton16(udp_get_checksum((const ip_address*)&socket->ip_remote,udp,length));
    return ip_send_packet((const ip_address*)&socket->ip_remote,IP_PROTOCOL_UDP,length);
}

#endif //NET_UDP