/*
 * Copyright (c) 2012 by Pawel Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "netstat.h"
#include "../net/net.h"
#include "../net/ethernet.h"
#include "../net/ip.h"
#include "../net/arp.h"
#include "../net/udp.h"
#include "../net/tcp.h"

#define NETSTAT_NDASHES		70

static void netstat_dashes(FILE * fh,uint16_t len)
{
	while(len--)
		fprintf_P(fh,PSTR("-"));
	fprintf_P(fh,PSTR("\n"));
}

uint8_t netstat(FILE * fh,uint8_t args)
{
	if(!args)
		return 1;
	fprintf_P(fh,PSTR("netstat:\n"));
	netstat_dashes(fh,NETSTAT_NDASHES);
	if(args&(1<<NETSTAT_OPT_IFACE))
	{ 
		const struct ethernet_stats * eth_stat = ethernet_get_stats();
		fprintf_P(fh,PSTR("HW addr: "));
		fprintf(fh,"%s\n",ethernet_addr_str(ethernet_get_mac()));
		fprintf_P(fh,PSTR("MTU    : "));
		fprintf(fh,"%d\n",ETHERNET_MAX_PACKET_SIZE);
		fprintf_P(fh,PSTR("RX pack: "));
		fprintf(fh,"%u\n",(unsigned int)eth_stat->rx_packets);
		fprintf_P(fh,PSTR("TX pack: "));
		fprintf(fh,"%u\n",(unsigned int)eth_stat->tx_packets);
		netstat_dashes(fh,NETSTAT_NDASHES);
	}
	if(args&(1<<NETSTAT_OPT_IP))
	{ 
		fprintf_P(fh,PSTR("IP addr: "));
		fprintf(fh,"%s\n",ip_addr_str(ip_get_addr()));
		fprintf_P(fh,PSTR("Bcast  : "));
		fprintf(fh,"%s\n",ip_addr_str(ip_get_broadcast()));
		fprintf_P(fh,PSTR("Mask   : "));
		fprintf(fh,"%s\n",ip_addr_str(ip_get_netmask()));
		fprintf_P(fh,PSTR("Gateway: "));
		fprintf(fh,"%s\n",ip_addr_str(ip_get_gateway()));
		netstat_dashes(fh,NETSTAT_NDASHES);
	}
	if(args&(1<<NETSTAT_OPT_ARP))
	{ 
		fprintf_P(fh,PSTR("ARP table:\n"));
		arp_print_stat(fh);
		fprintf_P(fh,PSTR("\n"));
		netstat_dashes(fh,NETSTAT_NDASHES);
	}
	if(args&(1<<NETSTAT_OPT_UDP) || args&(1<<NETSTAT_OPT_TCP))
	{
		fprintf_P(fh,PSTR("%-5S %5S %5S %-21S %-21S %S\n"),
			PSTR("Proto"),
			PSTR("Rx-Q"),
			PSTR("Tx-Q"),
			PSTR("Local Address"),
			PSTR("Remote Address"),
			PSTR("State"));
		if(args&(1<<NETSTAT_OPT_TCP))
		{ 
			tcp_print_stat(fh);
		}
		if(args&(1<<NETSTAT_OPT_UDP))
		{ 
			udp_print_stat(fh);
		}		
		netstat_dashes(fh,NETSTAT_NDASHES);
	}

	return 0;
}
