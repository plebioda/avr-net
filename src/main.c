/*
 * Copyright (c) 2012 by Pawel Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>

#define DEBUG_MODE
#include "debug.h"

#include "arch/interrupts.h"
#include "arch/exmem.h"
#include "arch/spi.h"
#include "arch/uart.h"
#include "arch/i2c.h"

#include "util/fifo.h"

#include "dev/ds1338.h"
#include "dev/enc28j60.h"
#include "dev/sd.h"

#include "sys/timer.h"
#include "sys/rtc.h"
#include "sys/partition.h"
#include "sys/fat.h"


#include "net/hal.h"
#include "net/ethernet.h"
#include "net/ip.h"
#include "net/arp.h"
#include "net/udp.h"
#include "net/tcp.h"

#include "app/app_config.h"
#include "app/tftp.h"
#include "app/tp.h"
#include "app/dhcp.h"
#include "app/echod.h"
#include "app/netstat.h"
#include "app/httpd.h"

struct partition partition;
struct fat_fs * fatfs;
struct fat_dir_entry * fat_root;

/*void time_callback(uint8_t event,uint32_t time)
{
	static struct date_time dt;
	rtc_convert_date_time(time, &dt);
	DBG_INFO("[time]: %02d:%02d:%02d\n",
			dt.hours,
			dt.minutes,
			dt.seconds
		);
}*/

void dhcp_callback(enum dhcp_event event)
{	
	switch(event)
	{
	case dhcp_event_timeout:
		DBG_INFO("[dhcp]: timeout\n");
		const ip_address ip = NET_IP_ADDRESS;
		const ip_address gw = NET_IP_GATEWAY;
		const ip_address nm = NET_IP_NETMASK;
		ip_init(&ip, &nm, &gw);		
		netstat(stdout, NETSTAT_OPT_ALL);
		break;
	case dhcp_event_lease_acquired:
		DBG_INFO("[dhcp]: lease acquired\n");
		netstat(stdout, NETSTAT_OPT_ALL);
		break;
	case dhcp_event_lease_expiring:
		DBG_INFO("[dhcp]: lease expiring\n");
		break;
	case dhcp_event_lease_expired:
		DBG_INFO("[dhcp]: lease expired\n");
		break;
	case dhcp_event_lease_denied:
		DBG_INFO("[dhcp]: lease denied\n");
		break;
	case dhcp_event_error:
		DBG_ERROR("[dhcp]: error\n");
		break;
	default:
		DBG_ERROR("[dhcp]: unknown event (0x%x)\n", event);
		break;
	}	
}

void sdcallback(enum sd_event event)
{
	uint8_t ret;
	DBG_INFO("[sd]: ");
	switch(event)
	{
		case sd_event_inserted:
			printf("card inserted\n");
			break;
		case sd_event_inserted_wp:
			printf("card inserted (WP!)\n");
			break;
		case sd_event_removed:
			printf("card removed\n");
			break;
		case sd_event_initialized:
			printf("card initialized\n");
			ret = partition_open(&partition,sd_read,0,0);
			fat_close(fatfs);
			fatfs = fat_open(&partition);
			fat_root = fat_get_dir_entry(fatfs, "/");
			tftpd_init(fat_root);
			//httpd_chroot(fat_root);
			break;
		case sd_event_error:
			DBG_ERROR("error %x\n",sd_errno());
			break;
		default:
			DBG_ERROR("unknown event\n");
			break;
	}
}

void timer_callback(timer_t timer, void * data)
{
	static struct date_time dt;
//	ds1338_get_date_time(&dt);
/*	DBG_INFO("[time]: %02d:%02d:%02d\n",
			dt.hours,
			dt.minutes,
			dt.seconds
		);*/
	netstat(stdout, NETSTAT_OPT_ALL);
}


int main(void)
{
	/* constants */
	const ethernet_address my_mac = {'<','P','A','K','O','>'};
	
	/* init io */
	DDRB = 0xff;
	PORTB = 0xff;
	DDRE &= ~(1<<7);
	PORTE |= (1<<7);

	/* init interrupts */
	interrupt_timer0_init();
	interrupt_timer1_init();
	interrupt_exint_init();
	
	/* init arch */
	uart_init();
	spi_init();
	i2c_init();
	timer_init();

	DBG_INFO("\n\n");
	DBG_INFO(B_IBLUE "avr-net ver %s build time: %s %s\n","1.0",__DATE__,__TIME__);
	
	/* init utils */
	fifo_init();
	
	/* init dev */
	uint8_t ret = ds1338_init();
	DBG_INFO("ds1338_init ret = 0x%x\n",ret);
	enc28j60_init((uint8_t*)&my_mac); 
	
	/* init sys */
//	fat_init();	
	//rtc_init((0<<RTC_FORMAT_12_24)|(0<<RTC_FORMAT_AM_PM));
	
	ethernet_init(&my_mac);
	const ip_address ip = NET_IP_ADDRESS;
	const ip_address gw = NET_IP_GATEWAY;
	const ip_address nm = NET_IP_NETMASK;
	ip_init(&ip, &nm, &gw);		
//	ip_init(0,0,0);
	arp_init();
	udp_init();
//	tcp_init();
	
	
//	echod_start();
	
	stdout = DEBUG_FH;	
	
//	sd_init(sdcallback);
//	sd_interrupt();
//	httpd_start();	
	
	dhcp_start(dhcp_callback);
//	netstat(stdout, NETSTAT_OPT_ALL);
	
//	timer_t timer = timer_alloc(timer_callback);
//	timer_set(timer, 5000, TIMER_MODE_PERIODIC);
	
//	DBG_INFO("Before sei\n");
	/* global interrupt enable */
	sei();
	for(;;)
	{
	}
	return 0;
}

ISR(TIMER0_COMP_vect)
{
	timer_tick();
}

ISR(TIMER0_OVF_vect)
{
}

ISR(TIMER1_OVF_vect)
{
}

ISR(INT7_vect)
{
	while(ethernet_handle_packet());
}

ISR(INT6_vect)
{
//	sd_interrupt();
}
