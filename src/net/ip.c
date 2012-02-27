/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <string.h>

#include "ethernet.h"
#include "ip.h"
#include "net.h"
#include "arp.h"
#include "icmp.h"
#include "udp.h"

#define DEBUG_MODE
#include "../debug.h"

#define IP_V4		0x4

#define IP_VIHL_HL_MASK	0xf
#define IP_FLAGS_MORE_FRAGMENTS 0x01
#define IP_FLAGS_DO_NOT_FRAGMENT 0x02
struct ip_header
{
  union
  {
    uint8_t version;
    uint8_t header_length;
  } vihl;
  uint8_t 	type_of_service;
  uint16_t 	length;
  uint16_t 	id;
  union
  {
    uint16_t flags;
    uint16_t fragment_offset;
  } ffo;
  uint8_t 	ttl;
  uint8_t 	protocol;
  uint16_t 	checksum;
  ip_address	src;
  ip_address 	dst;
};

static ip_address ip_addr;
static ip_address ip_broadcast;

uint8_t ip_is_broadcast(const ip_address * ip);

uint8_t ip_is_broadcast(const ip_address * ip)
{
    return (!memcmp(ip,&ip_broadcast,sizeof(ip_address)));
}


void ip_init(ip_address * addr)
{
  memset(&ip_broadcast,0xff,sizeof(ip_address));
  memset(&ip_addr,0,sizeof(ip_address));
  if(addr)
    memcpy(&ip_addr,addr,sizeof(ip_address));
  DEBUG_PRINT_COLOR(U_BLUE,"ip_init:");
  DEBUG_PRINT("%d.%d.%d.%d",(*addr)[0],(*addr)[1],(*addr)[2],(*addr)[3]);
  DEBUG_PRINT("\n");
}

const ip_address * ip_get_addr(void)
{
  return (const ip_address*)&ip_addr;
}

uint8_t ip_send_packet(const ip_address * ip_dst,uint8_t protocol,uint16_t length)
{
  ethernet_address mac;
  
  /* chech if ip dst address is broadcast */
  if(ip_is_broadcast(ip_dst))
  {
      /* if so then get ip bradcast addr and set mac broadcast*/
      memset(&mac,0xff,sizeof(ethernet_address));
  }
  else
  {
      /* otherwise try to get mac form arp table */
      if(!arp_get_mac(ip_dst,&mac))
      /* if there is no mac in arp table
	 the request for this mac is send
	 but we can't send this packet at this time
	 so we return 0 which means that packet was not send
      */
	return 0;
  }
  struct ip_header * ip = (struct ip_header*)ethernet_get_buffer();
  
  /* clear ip header */
  memset(ip,0,sizeof(struct ip_header));
  
  /* set version */
  ip->vihl.version = (IP_V4)<<4;
  
  /* set header length */
  ip->vihl.header_length |= (sizeof(struct ip_header) / 4) & 0xf;
  
  /* set ip packet length */
  uint16_t total_len = (uint16_t)sizeof(struct ip_header) + length;
  ip->length = hton16(total_len);
  
  /* set time to live */
  ip->ttl = 64;
  
  /* set protocol */
  ip->protocol = protocol;
  
  /* set src addr */
  memcpy(&ip->src,ip_get_addr(),sizeof(ip_address));
  
  /* set dst addr */
  memcpy(&ip->dst,ip_dst,sizeof(ip_address));
  
  /* compute checksum */
  ip->checksum = hton16(~net_get_checksum(0,(const uint8_t*)ip,sizeof(struct ip_header),10));
  
  /* send packet */
  return ethernet_send_packet(&mac,ETHERNET_TYPE_IP,total_len);
}

uint8_t ip_handle_packet(struct ip_header * header, uint16_t packet_len,const ethernet_address * mac )
{	
  if(packet_len < sizeof(struct ip_header))
    return 0;

  /* Check IP version */
  if((header->vihl.version>>4) != IP_V4)
    return 0;

  /* get header length */
  uint8_t header_length = (header->vihl.header_length & IP_VIHL_HL_MASK)*4;
  
  /* get packet length */
  uint16_t packet_length = ntoh16(header->length);
  
  /* check packet length */
  if(packet_length > packet_len)
    return 0;

  /* do not support fragmentation */
  if(ntoh16(header->ffo.flags) & (IP_FLAGS_MORE_FRAGMENTS << 13) || ntoh16(header->ffo.fragment_offset) & 0x1fff)
    return 0;

  /* check destination ip address */
  if(memcmp(&header->dst,ip_get_addr(),sizeof(ip_address)))
  {
      /* check if this is broadcast packet */
      if(header->dst[0] != 0xff ||
	header->dst[1] != 0xff ||
	header->dst[2] != 0xff ||
	header->dst[3] != 0xff)
	return 0;
  }

  /* check checksum */
  if(ntoh16(header->checksum) != (~net_get_checksum(0,(const uint8_t*)header,header_length,10)))
    return 0;

  /* add to arp table if ip does not exist */
  arp_table_insert((const ip_address*)&header->src,mac);
  
  /* redirect packet to the proper upper layer */
  switch(header->protocol)
  {
    case IP_PROTOCOL_ICMP:
      icmp_handle_packet((const ip_address*)&header->src,(const struct icmp_header*)((const uint8_t*)header + header_length),packet_length-header_length);
      break;
    case IP_PROTOCOL_UDP:
      udp_handle_packet((const ip_address*)&header->src,(const struct udp_header*)((const uint8_t*)header + header_length),packet_length-header_length);
      break;
    default:
      return 0;
  }
}

