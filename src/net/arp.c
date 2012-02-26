/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "arp.h"
#include "../arch/exmem.h"
#include "../sys/timer.h"

#define DEBUG_MODE
#include "../debug.h"

#include <string.h>

struct arp_header
{
    uint16_t 		hardware_type;
    uint16_t 		protocol_type;
    uint8_t 		hardware_addr_len;
    uint8_t 		protocol_addr_len;
    uint16_t 		operation_code;
    ethernet_address 	sender_hardware_addr;
    ip_address 		sender_protocol_addr;
    ethernet_address 	target_hardware_addr;
    ip_address 		target_protocol_addr;
};

#define ARP_TABLE_ENTRY_STATUS_EMPTY	0 
#define ARP_TABLE_ENTRY_STATUS_REQUEST	1
#define ARP_TABLE_ENTRY_STATUS_TIMEOUT	2

struct arp_table_entry
{
    uint8_t		status;
    ip_address 		ip_addr;
    ethernet_address 	ethernet_addr;
    uint16_t		timeout;
};

static struct arp_table_entry 	arp_table[ARP_TABLE_SIZE] EXMEM;
static timer_t arp_timer;

#define FOREACH_ARP_ENTRY(entry) for(entry = &arp_table[0] ; entry < &arp_table[ARP_TABLE_SIZE] ; entry++)

void arp_table_insert(const ip_address * ip_addr,const ethernet_address * ethernet_addr);
uint8_t arp_send_reply(const struct arp_header * header);
void arp_timer_tick(timer_t timer);
uint8_t arp_send_request(ip_address * ip_addr);

uint8_t arp_init(void)
{
    /* Clear arp table */
    memset(arp_table, 0 ,sizeof(arp_table));
    /* get system timer */
    arp_timer = timer_alloc((timer_callback_t)arp_timer_tick);
    /* Check timer */
    if(arp_timer < 0)
      return 0;
    /* Set timer */
    timer_set(arp_timer,ARP_TIMER_TICK_MS);
    return 1;
}

uint8_t arp_handle_packet(struct arp_header * header,uint16_t packet_length)
{
    /* Check packet length */
    if(packet_length < sizeof(struct arp_header))
      return 0;
    /* Check hardware address size */
    if(header->hardware_addr_len != ARP_HW_ADDR_SIZE_ETHERNET)
      return 0;
    /* Check protocol address size */
    if(header->protocol_addr_len != ARP_PROTO_ADDR_SIZE_IP)
      return 0;
    /* Check hardware address type */
    if(header->hardware_type != HTON16(ARP_HW_ADDR_TYPE_ETHERNET))
      return 0;
    /* Check protocol address type */
    if(header->protocol_type != HTON16(ARP_PROTO_ADDR_TYPE_IP))
      return 0;    
    /* Check whether target protocol address is our's */
    if(memcmp(header->target_protocol_addr,ip_get_addr(),sizeof(ip_address)))
      return 0;
    /* Parse operation code of packet */
    if(header->operation_code != HTON16(ARP_OPERATION_REQUEST) && header->operation_code != HTON16(ARP_OPERATION_REPLY))
      return 0;
    arp_table_insert((const ip_address*)&header->sender_protocol_addr,(const ethernet_address*)&header->sender_hardware_addr);
    if(header->operation_code == HTON16(ARP_OPERATION_REQUEST))
      return arp_send_reply(header);
    return 1;
}

uint8_t arp_send_reply(const struct arp_header * header)
{
    struct arp_header * arp_reply = (struct arp_header*)ethernet_get_buffer();
    
    arp_reply->hardware_addr_len = header->hardware_addr_len;
    arp_reply->hardware_type = header->hardware_type;
    arp_reply->protocol_addr_len = header->protocol_addr_len;
    arp_reply->protocol_type = header->protocol_type;
    arp_reply->operation_code = HTON16(ARP_OPERATION_REPLY);
    memcpy(&arp_reply->target_hardware_addr,&header->sender_hardware_addr,sizeof(ethernet_address));
    memcpy(&arp_reply->target_protocol_addr,&header->sender_protocol_addr,sizeof(ip_address));
    memcpy(&arp_reply->sender_protocol_addr,ip_get_addr(),sizeof(ip_address));
    memcpy(&arp_reply->sender_hardware_addr,ethernet_get_mac(),sizeof(ethernet_address));
    
    return ethernet_send_packet(&arp_reply->target_hardware_addr,ETHERNET_TYPE_ARP,sizeof(struct arp_header));
}

void arp_table_entry_cpy(struct arp_table_entry * entry,const ip_address * ip_addr,const ethernet_address * ethernet_addr)
{
  entry->timeout = ARP_TABLE_ENTRY_TIMEOUT;
  entry->status = ARP_TABLE_ENTRY_STATUS_TIMEOUT;  
  memcpy(&entry->ip_addr,ip_addr,sizeof(ip_address));
  memcpy(&entry->ethernet_addr,ethernet_addr,sizeof(ethernet_address));
  
}

void arp_table_insert(const ip_address * ip_addr,const ethernet_address * ethernet_addr)
{
  uint16_t min_timeout = ARP_TABLE_ENTRY_TIMEOUT;
  struct arp_table_entry * entry;
  struct arp_table_entry * min_entry = 0;
  FOREACH_ARP_ENTRY(entry)
  {
      /* There is already arp table entry with ip_addr which is waiting for reply */
      if(entry->status == ARP_TABLE_ENTRY_STATUS_REQUEST && !memcmp(ip_addr,&entry->ip_addr,sizeof(ip_address)))
      {
	    arp_table_entry_cpy(min_entry,ip_addr,ethernet_addr);
	    return;
      }
      if(entry->timeout < min_timeout)
      {
	  min_timeout = entry->timeout;
	  min_entry = entry;
      }
  }
  if(min_entry == 0)
  {
      DEBUG_PRINT_COLOR(B_IRED,"error: arp_table_insert: min_entry == 0\n");
      return;
  }
  arp_table_entry_cpy(min_entry,ip_addr,ethernet_addr);
}

void arp_timer_tick(timer_t timer)
{
    if(timer != arp_timer)
      return;
    struct arp_table_entry * entry;
    FOREACH_ARP_ENTRY(entry)
    {
	if(entry->status != ARP_TABLE_ENTRY_STATUS_EMPTY)
	{
	  if(entry->timeout > 0)
	    entry->timeout--;
	  else
	    entry->status = ARP_TABLE_ENTRY_STATUS_EMPTY;
	}
    }
    timer_set(arp_timer,ARP_TIMER_TICK_MS);
}

uint8_t arp_get_mac(ip_address * ip_addr,ethernet_address * ethernet_addr)
{
    struct arp_table_entry * entry;
    struct arp_table_entry * empty;
    FOREACH_ARP_ENTRY(entry)
    {
	if(entry->status == ARP_TABLE_ENTRY_STATUS_EMPTY)
	{
	  empty = entry;
	  continue;
	}
	if(!memcmp(&entry->ip_addr,ip_addr,sizeof(ip_address)))
	{
	    switch(entry->status)
	    {
	      /* There is ethernet address in arp table */
	      case ARP_TABLE_ENTRY_STATUS_TIMEOUT:
		entry->timeout = ARP_TABLE_ENTRY_TIMEOUT;
		memcpy(ethernet_addr,&entry->ethernet_addr,sizeof(ethernet_address));
		return 1;
	      /* There is ip address but waiting for reply*/
	      case ARP_TABLE_ENTRY_STATUS_REQUEST:
		return 0;
	      default:
		continue;
	    }
	}
    }
    if(empty != 0)
    {
      memcpy(&empty->ip_addr,ip_addr,sizeof(ip_address));
      empty->status = ARP_TABLE_ENTRY_STATUS_REQUEST;
      empty->timeout = ARP_TABLE_ENTRY_REQ_TIME;
    }
    arp_send_request(ip_addr);
    return 0;
}

uint8_t arp_send_request(ip_address * ip_addr)
{
  struct arp_header * arp_request = (struct arp_header*)ethernet_get_buffer();
  
  /* Set protocol and hardware addresses type and length */
  arp_request->hardware_addr_len = ARP_HW_ADDR_SIZE_ETHERNET;
  arp_request->protocol_addr_len = ARP_PROTO_ADDR_SIZE_IP;
  arp_request->hardware_type = HTON16(ARP_HW_ADDR_TYPE_ETHERNET);
  arp_request->protocol_type = HTON16(ARP_PROTO_ADDR_TYPE_IP);
  
  /* Set sedner and target hardware and protocol addresses */
  memset(&arp_request->target_hardware_addr,0,sizeof(ethernet_address));
  memcpy(&arp_request->target_protocol_addr,ip_addr,sizeof(ip_address));
  memcpy(&arp_request->sender_hardware_addr,ethernet_get_mac(),sizeof(ethernet_address));
  memcpy(&arp_request->sender_protocol_addr,ip_get_addr(),sizeof(ip_address));
  
  /* Set operation code */
  arp_request->operation_code = HTON16(ARP_OPERATION_REQUEST);
  /* Send packet */
  return ethernet_send_packet(ETHERNET_ADDR_BROADCAST,ETHERNET_TYPE_ARP,sizeof(struct arp_header));
}