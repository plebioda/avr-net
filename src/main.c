/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <avr/io.h>
#include <avr/interrupt.h>

#include <string.h>
#include <stdio.h>

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

struct partition partition;
struct fat_fs * fatfs;
struct fat_dir_entry * fat_root;
void ds1338_print_time(struct date_time * dt);

void tcallback(timer_t timer,void * arg)
{
	static struct date_time dt;
	DBG_INFO("ds1338_time: \n");
	ds1338_get_date_time(&dt);
	ds1338_print_time(&dt);
	timer_set(timer,30000);
}

void tpcallback(uint8_t status,uint32_t timeval)
{
	static struct date_time date_time;
	DBG_INFO("tp status = %d\n",status);
	if(status)
		return;
	DBG_INFO("tp time sync\n");
	rtc_convert_date_time(timeval,&date_time);
	ds1338_set_date_time(&date_time);
	ds1338_print_time(&date_time);

}

static char * days[7] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};

void ds1338_print_time(struct date_time * dt)
{
	DBG_INFO("%02x:%02x:%02x\n",dt->hours,dt->minutes,dt->seconds);
	DBG_INFO(" %s \n",days[dt->day-1]);
	DBG_INFO("%02x.%02x.%02x\n",dt->date,dt->month,dt->year);
	DBG_INFO(" format = %02x\n",dt->format);
}
/*
void dhcpcallback(enum dhcp_event event)
{
	DBG_INFO("[dhcp]: \n");
	switch(event)
	{
		case dhcp_event_lease_acquired:
			DBG_INFO("lease acquired\n");
			break;
		case dhcp_event_lease_denied:
			DBG_INFO("lease denied\n");
			break;
		case dhcp_event_lease_expired:
			DBG_ERROR("lease expired\n");
			break;
		case dhcp_event_lease_expiring:
			DBG_INFO("lease expiring\n");
			break;
		case dhcp_event_error:
		default:
			DBG_ERROR("error\n");
			break;
	}
}

void echocallback(enum echo_event event)
{
 DBG_INFO("Echo callback: event = %d\n",event); 
}
*/

// void netstat_callback(timer_t timer,void * arg)
// {
//	 netstat(stdout,NETSTAT_OPT_ALL);
//	 timer_set(timer,5000);
// }
// void sdtimer_callback(timer_t timer,void * arg)
// {
//	 sd_interrupt();
//	 timer_set(timer,2000);
// }

void sdcallback(enum sd_event event)
{
	uint8_t ret;
	DBG_INFO("[sd]: \n");
	switch(event)
	{
		case sd_event_inserted:
			DBG_INFO("card inserted\n");
			break;
		case sd_event_inserted_wp:
			DBG_INFO("card inserted\n");
			DBG_INFO(" (Write Protection)");
			break;
		case sd_event_removed:
			DBG_INFO("card removed\n");
			break;
		case sd_event_initialized:
			DBG_INFO("card initialized\n");
			ret = partition_open(&partition,sd_read,0,0);
			DBG_INFO("partition_open = 0x%x\n",ret);
			fat_close(fatfs);
			fatfs = fat_open(&partition);
			fat_root = fat_get_dir_entry(fatfs, "/");
			tftpd_init(fat_root);
			break;
		case sd_event_error:
			DBG_ERROR("error %x\n",sd_errno());
			break;
		default:
			DBG_ERROR("unknown event\n");
			break;
	}
	DBG_INFO("\n");
}

char buffer[512];

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
	interrupt_exint_init();
	
	/* init arch */
	uart_init();
	spi_init();
	i2c_init();
	
	DBG_INFO(B_IBLUE "avr-net ver %s build time: %s %s\n","1.0",__DATE__,__TIME__);
	
	/* init utils */
	fifo_init();
	
	/* init dev */
//	 ds1338_init();
	enc28j60_init((uint8_t*)&my_mac); 
	
	/* init sys */
	timer_init();
	fat_init();
//	 rtc_init((0<<RTC_FORMAT_12_24)|(0<<RTC_FORMAT_AM_PM));
	
	ethernet_init(&my_mac);
	ip_init(0,0,0);
	arp_init();
	udp_init();
	tcp_init();
	
	//uint8_t ret = echod_start(echocallback);
	//DBG_INFO("Echo server ret=%d\n",ret);
	
	stdout = DEBUG_FH;	
		
	sd_init(sdcallback);
	sd_interrupt();
	
	//timer_t sdtimer = timer_alloc(sd_timer_callback);
	//timer_set(sdtimer,3000);
//	 timer_t rtc_timer = timer_alloc(tcallback);
//	 timer_set(rtc_timer,1000);
		 
//	 uint8_t ret = dhcp_start(dhcpcallback);
//	 DBG_INFO("dhcp start ret=	%d\n",ret);
//	 tp_get_time(tpcallback);
//	 udp_socket_t udps;
//	 udps = udp_socket_alloc(12348,udp_callback);
	/*struct fat_dir_entry * wd = fat_get_dir_entry(fatfs,"/");
	if(NULL != wd )
	{	
		DBG_INFO(B_IYELLOW "NULL != dirent\n");
	}
	struct fat_file * file = fat_fopen(wd, "GPLV2");
	size_t read;
	uint8_t i = 0;
	if(NULL != file)
	{
		DBG_INFO(B_IYELLOW "File struct not NULL\n");
		for(i=0;i<7;i++)
		{
			if((read = fat_fread(file, buffer, 512)) > 0)
			{
				//print_block((uint8_t*)buffer, read);	
			}
		}
		if((read = fat_fread(file, buffer, 256)) > 0)
		{
			//print_block((uint8_t*)buffer, read);	
		}
		
		if((read = fat_fread(file, buffer, 512)) > 0)
		{
			print_block((uint8_t*)buffer, read);	
		}
		
	}*/
	DBG_INFO(B_IYELLOW "End\n");
	/* global interrupt enable */
	sei();
	for(;;)
	{
		
	}
	return 0;
}

ISR(TIMER0_COMP_vect)
{
	//1ms
	timer_tick();
}

ISR(INT7_vect)
{
	while(ethernet_handle_packet());
}

ISR(INT6_vect)
{
	sd_interrupt();
}
