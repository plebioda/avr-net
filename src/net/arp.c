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


struct arp_table_entry
{
    ip_address 		ip_addr;
    ethernet_address 	ethernet_addr;
    uint8_t		timeout;
};

static struct arp_table_entry 	arp_table[ARP_TABLE_SIZE] EXMEM;
static timer_t arp_timer;

void arp_table_insert(const ip_address * ip_addr,const ethernet_address * ethernet_addr);
uint8_t arp_send_reply(const struct arp_header * header);
void arp_timer_tick(timer_t timer);

uint8_t arp_init(void)
{
    /* Clear arp table */
    memset(arp_table, 0 ,sizeof(arp_table));
    /* get system timer */
    arp_timer = timer_alloc((timer_callback)arp_timer_tick);
    /* Check timer */
    if(arp_timer < 0)
      return 0;
    /* Set timer */
    timer_set(arp_timer,ARP_TIMER_TICK_MS);
    return 1;
}

uint8_t arp_handle_packet(struct arp_header * header,uint16_t packet_length)
{
    DEBUG_PRINT_COLOR(U_CYAN,"arp_handle_packet: packet_len = %d\n",packet_length);
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
    DEBUG_PRINT_COLOR(B_CYAN,"arp_handle_packet: op = %x\n",NTOH16(header->operation_code));
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

void arp_table_insert(const ip_address * ip_addr,const ethernet_address * ethernet_addr)
{
    
}

void arp_timer_tick(timer_t timer)
{
    if(timer != arp_timer)
      return;
    DEBUG_PRINT_COLOR(B_IMAGENTA,"ARP Timer tick\n");
    struct arp_table_entry * entry;
    for(entry = &arp_table[0] ; entry < &arp_table[ARP_TABLE_SIZE] ; entry++)
    {
	if(entry->timeout > 0)
	  entry->timeout--;
    }
    timer_set(arp_timer,ARP_TIMER_TICK_MS);
}
