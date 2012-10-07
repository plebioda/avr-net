/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "rtc.h"
#include <stdint.h>

/* 
* RFC 868: The time is the number of seconds since 00:00 (midnight) 1 January 1900
*/

uint8_t make_bcd(uint16_t val)
{
	return (uint8_t)((val/10)<<4)|(uint8_t)(val%10);
}
uint8_t isleap(uint16_t year)
{
	return (((year%4) == 0 && (year%100) != 0) || (year%400) == 0);
}

void rtc_convert_date_time(uint32_t timeval,struct date_time * dti)
{
	uint16_t seconds,minutes,hours,date,month,year,day;
	
	/* number of days since 00:00 (midnight) 1 January 1900 GMT*/
//	 timeval += GMT_SECS_OFFSET;
	uint16_t days = timeval / (24 * 3600UL); 
	timeval -= days * (24 * 3600UL);
	
	year=1900;
	
	while(days>366)
	{
		if(isleap(year))
			days-=366;
		else
			days-=365;
		year++;
	}
	day=days%7;
	uint16_t cur_month=1;
	uint8_t leap = isleap(year);
	while(days>28)
	{
		if(cur_month==2)
		{
			days -= leap?29:28;
		}
		else if(cur_month <= 7)
		{
			if((cur_month & 1) == 1)
				days -= 31;
			else
				days -= 30;
		}
		else
		{
			if((cur_month & 1) == 1)
				days -= 30;
			else
				days -= 31;
		}
		cur_month++;
	}
	
	if(cur_month == 13)
		cur_month = 1;
	
	month = cur_month;
	
	date = days+1;
	
	
	hours = timeval / 3600UL;
	timeval -= hours * 3600UL;
	
	minutes = timeval / 60;
	timeval-= minutes*60;
	
	seconds = timeval;
	
	dti->day=day;
	dti->seconds = make_bcd(seconds);
	dti->minutes = make_bcd(minutes);
	dti->hours = make_bcd(hours);
	dti->date = make_bcd(date);
	dti->month = make_bcd(month);
	dti->year= make_bcd(year%100);
}
