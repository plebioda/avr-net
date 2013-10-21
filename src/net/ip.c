/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/**
 * @addtogroup net
 * @{
 * @addtogroup ip
 * @{
 * @file ip.c
 * @author Pawel Lebioda <pawel.lebioda89@gmail.com>
 * @brief This file contains definition of Internet Protocol layer functions
 */
#include <string.h>
#include <stdio.h>

#include "ethernet.h"
#include "ip.h"
#include "net.h"
#include "arp.h"
#include "icmp.h"
#include "udp.h"
#include "tcp.h"

#include "../debug.h"

/**
 * Internet Protocol version 4
 */
#define IP_V4		0x4

#define IP_VIHL_HL_MASK	0xf
/**
 * Internet Protocol Header
 */
struct ip_header
{
	/**
 	 * Version and header length
	 */
	union
	{
		/**
		 * Version - 4 bits
		 */
		uint8_t version; 

		/**
 		 * Header length in 4 bytes words - 4 bits
		 */
		uint8_t header_length;
	} vihl;

	/**
	 * Type of service
	 */
	uint8_t 	type_of_service;
	
	/**
	 * Data length
	 */
	uint16_t 	length;

	/**
	 * Packet ID
	 */
	uint16_t 	id;
	
	/**
	 * Flags and fragment offset
	 */
	union
	{
		/**
		 * Flags
		 */
		uint16_t flags;
		
		/**
		 * Fragment offset
 		 */
		uint16_t fragment_offset;
	} ffo;
	
	/**
	 * More fragments
	 */
	#define IP_FLAGS_MORE_FRAGMENTS 0x01

	/**
	 * Do not fragment
	 */
	#define IP_FLAGS_DO_NOT_FRAGMENT 0x02

	/**
	 * Time to live
	 */
	uint8_t 	ttl;
	
	/**
	 * Protocol
	 */
	uint8_t 	protocol;
	
	/**
	 * Checksum
	 */
	uint16_t 	checksum;

	/**
	 * Source IP Address
	 */
	ip_address	src;

	/**
	 * Destination IP Address
	 */
	ip_address 	dst;
};

/**
 * IP Address
 */
static ip_address ip_addr = NET_IP_ADDRESS;

/**
 * Netmask
 */
static ip_address ip_netmask = NET_IP_NETMASK;

/**
 * Gateway address
 */
static ip_address ip_gateway = NET_IP_GATEWAY;

/**
 * Broadcast address
 */
static ip_address ip_broadcast;

/*		A.B.C.D = 3*4 + 3 dots +	: + port number + NULL*/
static char ip_addr_char[3*sizeof(ip_address) + 3 + 1 + 5 + 1];


/**
 * Checks if specified ip address is the broadcast address
 * @param [in] ip IP Address
 * @return 1 if specified address is broadcast, otherwise 0
 */
uint8_t ip_is_broadcast(const ip_address * ip);

/**
 * Checks if specified ip address belongs to the same subnet
 * @param [in] ip_remote IP Address
 * @return 1 if specified address belongs to the same subnet, otherwise 0
 */
uint8_t ip_in_subnet(const ip_address * ip_remote);
const ip_address * ip_get_addr(void)		{return (const ip_address*)&ip_addr;} 
const ip_address * ip_get_netmask(void) 	{return (const ip_address*)&ip_netmask;} 
const ip_address * ip_get_broadcast(void)	{return (const ip_address*)&ip_broadcast;} 
const ip_address * ip_get_gateway(void)		{return (const ip_address*)&ip_gateway;} 

/**
 *
 */
static void ip_set_broadcast(void);

/**
 *
 */
const char * ip_addr_str(const ip_address * addr)
{
	sprintf(ip_addr_char,"%d.%d.%d.%d",*((uint8_t*)addr+0),*((uint8_t*)addr+1),*((uint8_t*)addr+2),*((uint8_t*)addr+3));
	return (const char*)ip_addr_char;
}

/**
 *
 */
const char * ip_addr_port_str(const ip_address * addr,uint16_t portno)
{
	sprintf(ip_addr_char,
		"%d.%d.%d.%d:%u",
		*((uint8_t*)addr+0),
		*((uint8_t*)addr+1),
		*((uint8_t*)addr+2),
		*((uint8_t*)addr+3),
		portno);
	return (const char*)ip_addr_char;
}


/**
 *
 */
uint8_t ip_is_broadcast(const ip_address * ip)
{
	return (*((uint32_t*)ip) == 0xffffffff || memcmp(ip,ip_broadcast,4) == 0);
}

/**
 *
 */
void ip_init(const ip_address * addr,const ip_address * netmask,const ip_address * gateway)
{
	if(addr)
	{
		memcpy(&ip_addr,addr,sizeof(ip_address));
	}
	if(netmask)
	{
		memcpy(&ip_netmask,netmask,sizeof(ip_address));
	}
	if(gateway)
	{
		memcpy(&ip_gateway,gateway,sizeof(ip_address));
	}
	
	ip_set_broadcast();
}


/**
 *
 */
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
		const ip_address * arp_target;
		/* check if remote host is in the same subnet */
		if(ip_in_subnet(ip_dst))
		{
			/* if so we will request for remote's host mac address */
			arp_target=ip_dst;
		}
		else
		{
			/* otherwise request for gateway's	mac address */
			arp_target=(const ip_address*)&ip_gateway;
		}

		if(!arp_get_mac(arp_target,&mac))
		{
			/* if there is no mac in arp table
			 the request for this mac is send
			 but we can't send this packet at this time
			 so we return 0 which means that packet was not send
			*/
			return 0;
		}
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


/**
 *
 */
uint8_t ip_handle_packet(struct ip_header * header, uint16_t packet_len,const ethernet_address * mac )
{	
	if(packet_len < sizeof(struct ip_header))
	{
		return 0;
	}

	/* Check IP version */
	if((header->vihl.version>>4) != IP_V4)
	{
		return 0;
	}

	/* get header length */
	uint8_t header_length = (header->vihl.header_length & IP_VIHL_HL_MASK)*4;
	
	/* get packet length */
	uint16_t packet_length = ntoh16(header->length);
	
	/* check packet length */
	if(packet_length > packet_len)
	{		
		return 0;
	}

	/* do not support fragmentation */
	if(ntoh16(header->ffo.flags) & (IP_FLAGS_MORE_FRAGMENTS << 13) || ntoh16(header->ffo.fragment_offset) & 0x1fff)
	{
		return 0;
	}

	/* check destination ip address */
	if(memcmp(&header->dst,ip_get_addr(),sizeof(ip_address)))
	{
		/* check if this is broadcast packet */
		if(	header->dst[0] != 0xff ||
			header->dst[1] != 0xff ||
			header->dst[2] != 0xff ||
			header->dst[3] != 0xff)
		{
			return 0;
		}
	}

	/* check checksum */
	if(ntoh16(header->checksum) != (~net_get_checksum(0,(const uint8_t*)header,header_length,10)))
	{
		return 0;
	}

	/* add to arp table if ip does not exist */
	arp_table_insert((const ip_address*)&header->src,mac);
	
	/* redirect packet to the proper upper layer */
	switch(header->protocol)
	{
// #if NET_ICMP		
		case IP_PROTOCOL_ICMP:
			icmp_handle_packet(
				(const ip_address*)&header->src,
				(const struct icmp_header*)((const uint8_t*)header + header_length),
				packet_length-header_length);
			break;
// #endif //NET_ICMP
#if NET_UDP
		case IP_PROTOCOL_UDP:
			udp_handle_packet(
				(const ip_address*)&header->src,
				(const struct udp_header*)((const uint8_t*)header + header_length),
				packet_length-header_length);
			break;
#endif //NET_UDP			
		case IP_PROTOCOL_TCP:
			tcp_handle_packet(
				(const ip_address*)&header->src,
				(const struct tcp_header*)((const uint8_t*)header + header_length),
				packet_length-header_length);
			break;
		default:
			break;
	}

	return 0;
}

/**
 *
 */
uint8_t ip_in_subnet(const ip_address * ip_remote)
{
	uint8_t i;
	for(i=0;i<sizeof(ip_address);i++)
	{
		if(
			(
				((uint8_t*)ip_remote)[i] 
				& 
				((uint8_t*)ip_netmask)[i]
			) 
			!= 
			(
				((uint8_t*)ip_addr)[i] 
				& 
				((uint8_t*)ip_netmask)[i]
			)
		)
		{
			return 0;
		}
	}

	return 1;
}

/**
 *
 */
void ip_set_broadcast(void)
{
	uint8_t i;
	if((ip_netmask[0] | ip_netmask[1] | ip_netmask[2] | ip_netmask[3]) == 0)
	{
		memset(&ip_broadcast,0,sizeof(ip_broadcast));
	}
	else
	{
		for(i=0;i<sizeof(ip_address);i++)
		{
			ip_broadcast[i] = (ip_addr[i] | ~ip_netmask[i]);
		}
	}
}

/**
 * @}
 * @}
 */

