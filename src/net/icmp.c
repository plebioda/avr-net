/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "icmp.h"

#if NET_ICMP

#define DEBUG_MODE
#include "../debug.h"

#include <string.h>

#include "net.h"
#include "ethernet.h"
#include "ip.h"

// #include "../util/memory.h"

#define ICMP_TYPE_ECHO_REPLY			0x00
#define ICMP_TYPE_ECHO_REQUEST			0x08
#define ICMP_TYPE_DESTINATION_UNREACHABLE	0x03

struct  icmp_header
{
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
};

uint8_t icmp_send_echo_reply(const ip_address * ip_addr,const struct icmp_header * icmp,uint16_t packet_len);


uint8_t icmp_handle_packet(const ip_address * ip_addr,const struct icmp_header * icmp,uint16_t packet_len)
{
    if(packet_len < sizeof(struct icmp_header))
      return 0;
    
    /* check checksum */
    if(hton16(icmp->checksum) != ~net_get_checksum(0,(uint8_t*)icmp,packet_len,2))
      return 0;

    /* parse icmp packet */
    switch(icmp->type)
    {
      case ICMP_TYPE_ECHO_REQUEST:
	return icmp_send_echo_reply(ip_addr,icmp,packet_len);
      case ICMP_TYPE_ECHO_REPLY:
	break;
      case ICMP_TYPE_DESTINATION_UNREACHABLE:
	break;
    }
    return 0;
}

uint8_t icmp_send_echo_reply(const ip_address * ip_addr,const struct icmp_header * icmp,uint16_t packet_len)
{
  struct icmp_header * icmp_reply = (struct icmp_header*)ip_get_buffer();
  
  memcpy(icmp_reply,icmp,packet_len);
  
  /* set type */
  icmp_reply->type = ICMP_TYPE_ECHO_REPLY;
  
  /* compute checksum */
  icmp_reply->checksum = hton16(~net_get_checksum(0,(const uint8_t*)icmp_reply,packet_len,2));
  
  /* send ip packet */
  return ip_send_packet(ip_addr,IP_PROTOCOL_ICMP,packet_len);
      
}

#endif //NET_ICMP