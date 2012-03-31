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
#define DEBUG_MODE
#include "../debug.h"

#include <string.h>

struct ethernet_header
{
	ethernet_address 	dst;
	ethernet_address 	src;
	uint16_t 		type;
};

static ethernet_address ethernet_mac;

uint8_t ethernet_tx_buffer[ETHERNET_MAX_PACKET_SIZE + NET_HEADER_SIZE_ETHERNET]; // EXMEM
uint8_t ethernet_rx_buffer[ETHERNET_MAX_PACKET_SIZE + NET_HEADER_SIZE_ETHERNET]; //EXMEM

void ethernet_init(const ethernet_address * mac)
{
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
  switch(header->type)
  {
    case HTON16(ETHERNET_TYPE_IP):
      return ip_handle_packet((struct ip_header*)data,packet_size,(const ethernet_address*)&header->src);
    case HTON16(ETHERNET_TYPE_ARP):
      DEBUG_PRINT("eth type arp\n");
      return arp_handle_packet((struct arp_header*)data,packet_size);
    default:
      return 0;
  }
  return 1;
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
  uint16_t i=0;
  DEBUG_PRINT("eth send packet:\n");
  for(i=0;i<len+NET_HEADER_SIZE_ETHERNET;i++)
    DEBUG_PRINT("%02x",ethernet_tx_buffer[i]);
  DEBUG_PRINT("\n----------------------------------\n");
  return hal_send_packet(ethernet_tx_buffer,(len + NET_HEADER_SIZE_ETHERNET));			
}