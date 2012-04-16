/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "ethernet.h"
#include "hal.h"
#include "../arch/exmem.h"
#include "ip.h"
#include "arp.h"
// #define DEBUG_MODE
#include "../debug.h"

#include <string.h>
#include <stdio.h>

struct ethernet_header
{
	ethernet_address 	dst;
	ethernet_address 	src;
	uint16_t 		type;
};


static struct ethernet_stats ethernet_stats;
static ethernet_address ethernet_mac;

static char ethernet_addr_char[2*sizeof(ethernet_address) + 5 + 1];

uint8_t ethernet_tx_buffer[ETHERNET_MAX_PACKET_SIZE + NET_HEADER_SIZE_ETHERNET]; // EXMEM
uint8_t ethernet_rx_buffer[ETHERNET_MAX_PACKET_SIZE + NET_HEADER_SIZE_ETHERNET]; //EXMEM

const struct ethernet_stats * ethernet_get_stats(void)
{
  return (const struct ethernet_stats*)&ethernet_stats;
}
const char * ethernet_addr_str(const ethernet_address * addr)
{
  sprintf(ethernet_addr_char,
	  "%02x:%02x:%02x:%02x:%02x:%02x",
	  *((uint8_t*)addr+0),
	  *((uint8_t*)addr+1),
	  *((uint8_t*)addr+2),
	  *((uint8_t*)addr+3),
	  *((uint8_t*)addr+4),
	  *((uint8_t*)addr+5));
  return (const char*)ethernet_addr_char;
}

void ethernet_init(const ethernet_address * mac)
{
  memset(&ethernet_stats,0,sizeof(ethernet_stats));
  memset(ethernet_mac,1,sizeof(ethernet_mac));
  if(mac)
    memcpy(&ethernet_mac,mac,sizeof(ethernet_mac));
}


const ethernet_address * ethernet_get_mac()
{
  return (const ethernet_address*)&ethernet_mac;
}

uint8_t ethernet_handle_packet()
{
  /* receive packet */
  uint16_t packet_size = hal_receive_packet(ethernet_rx_buffer,sizeof(ethernet_rx_buffer));
   
  if(packet_size < 1)
    return 0;
 
//   DEBUG_PRINT("Packet size = %d\n",packet_size);
  
  /* get ethernet header */
  struct ethernet_header * header = (struct ethernet_header*)ethernet_rx_buffer;
  
  /* remove ethernet header from packet for upper layers */
  packet_size -= sizeof(*header);
  uint8_t * data = (uint8_t*)(header+1);
  
  /* redirect packet to the proper upper layer */
  uint8_t ret=1;
  switch(header->type)
  {
    case HTON16(ETHERNET_TYPE_IP):
      ret = ip_handle_packet((struct ip_header*)data,packet_size,(const ethernet_address*)&header->src);
      break;
    case HTON16(ETHERNET_TYPE_ARP):
      ret = arp_handle_packet((struct arp_header*)data,packet_size);
      break;
    default:
      return 0;
  }
  ethernet_stats.rx_packets++;
  return ret;
}

uint8_t ethernet_send_packet(ethernet_address * dst,uint16_t type,uint16_t len)
{
  DEBUG_PRINT_COLOR(B_IRED,"eth sp len=%d\n",len);
  if(len > ETHERNET_MAX_PACKET_SIZE -NET_HEADER_SIZE_ETHERNET)
    return 0;
  struct ethernet_header * header = (struct ethernet_header*)ethernet_tx_buffer;
  if(dst == ETHERNET_ADDR_BROADCAST)
    memset(&header->dst,0xff,sizeof(ethernet_address));
  else
    memcpy(&header->dst,dst,sizeof(ethernet_address));
  memcpy(&header->src,&ethernet_mac,sizeof(ethernet_address));
  header->type = hton16(type);
  ethernet_stats.tx_packets++;
  return hal_send_packet(ethernet_tx_buffer,(len + NET_HEADER_SIZE_ETHERNET));			
}