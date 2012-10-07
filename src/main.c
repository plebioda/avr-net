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


void print_partition(struct partition * p)
{
		DBG_INFO("Partition:\n");
		DBG_INFO("State: %s\n",(p->state==0x80 ? "ACTIVE" : (p->state==0 ? "INACTIVE":"?")));
		DBG_INFO("Type: %d\n",(p->type));
		DBG_INFO("Offset: %d\n",(p->offset));
		DBG_INFO("Length: %d\n",(p->length));
}
void print_fat_header(struct fat_header * fh)
{
		DBG_INFO("Fat header: \n");
		DBG_INFO("Size: %lx\n",fh->size);
		DBG_INFO("Fat offset: %lx\n",fh->fat_offset);
		DBG_INFO("Fat size: %lx\n",fh->fat_size);
		DBG_INFO("Sector size: %x\n",fh->sector_size);
		DBG_INFO("Cluster size: %x\n",fh->cluster_size);
		DBG_INFO("Root dir offset: %lx\n",fh->root_dir_offset);
		DBG_INFO("Cluster 0 offset: %lx\n",fh->cluster_zero_offset);
}
struct partition partition;
struct fat_fs fatfs;
struct fat_dir_entry fat_dir_entry;


void sdcallback(enum sd_event event)
{
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
			partition_open(&partition,sd_read,0,0);
			print_partition(&partition);
			fat_open(&fatfs,&partition);
			print_fat_header(&fatfs.header);
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

//uint8_t block[512];

void print_block(uint8_t * block,uint16_t len)
{
	uint16_t row,col;
	for(row=0;row<len/16;row++)
	{
		printf("%08x: ",row*16); 
		for(col=0;col<8;col++)
			printf("%02x ",block[row*16+col]);
		printf(" ");
		for(col=8;col<16;col++)
			printf("%02x ",block[row*16+col]);
//		 printf("|");
//		 for(col=0;col<16;col++)
//			 printf("%c",block[row*16+col]);
//		 printf("|");
	} 
		 
}

int main(void)
{
	/* constants */
	const ethernet_address my_mac = {'<','P','A','K','O','>'};
//	 const ip_address my_ip = {192,168,1,200};
	
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
	
	/* init utils */
	fifo_init();
	
	/* init dev */
//	 ds1338_init();
	enc28j60_init((uint8_t*)&my_mac); 
	
	/* init sys */
	timer_init();
//	 rtc_init((0<<RTC_FORMAT_12_24)|(0<<RTC_FORMAT_AM_PM));
	
	/* init net */
	 ethernet_init(&my_mac);
	 ip_init(0,0,0);
	 arp_init();
	 udp_init();
	 tcp_init();
	
	uint8_t ret = echod_start(echocallback);
	
	DBG_INFO("echo ret=%d\n",ret);
	
	stdout = DEBUG_FH;	
		
	sd_init(sdcallback);
	DBG_INFO("Hello AVR!\n");
	sd_interrupt();
	
	//timer_t sdtimer = timer_alloc(sd_timer_callback);
	//timer_set(sdtimer,3000);
//	 memset(block,0,512);
//	 int i;
//	 for(i=0;i<17;i++) 
//	 {
//		 sd_read(i<<9,block,512);
//		 print_block(block,512);
//	 }
//	 timer_t rtc_timer = timer_alloc(tcallback);
//	 timer_set(rtc_timer,1000);
		 
//	 uint8_t ret = dhcp_start(dhcpcallback);
//	 DBG_INFO("dhcp start ret=	%d\n",ret);
//	 tp_get_time(tpcallback);
//	 udp_socket_t udps;
//	 udps = udp_socket_alloc(12348,udp_callback);


	
//	 DBG_INFO("Hello ATMega128!\n");
//	 DBG_INFO("ENC28J60 rev %d\n",enc28j60_get_revision());

	
//	 memset(block,0,512);
//	 int i;
//	 for(i=0;i<17;i++) 
//	 {
//		 sd_read(i<<9,block,512);
//		 print_block(block,512);
//	 }

	
	//fat_get_dir_entry(&fatfs,&fat_dir_entry,"/ROOT/FOO/..");		
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
	DBG_ERROR("INT6_vect\n");
	sd_interrupt();
}
