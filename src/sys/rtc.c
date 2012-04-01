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

uint8_t tp_make_bcd(uint16_t val)
{
  return (uint8_t)((val/10)<<4)|(uint8_t)(val%10);
}

void rtc_convert_date_time(uint32_t timeval,struct date_time * dti)
{
    uint16_t year,month,day,hours,minutes,seconds;
    uint16_t days = timeval / (24 * 3600UL);
    timeval -= days * (24 * 3600UL);

    uint16_t years_leap = days / (366 + 3 * 365);
    days -= years_leap * (366 + 3 * 365);
    year = 1900 + years_leap * 4;
    uint8_t leap = 0;
    if(days >= 366)
    {
        days -= 366;
        ++year;

        if(days >= 365)
        {
            days -= 365;
            ++year;
        }
        if(days >= 365)
        {
            days -= 365;
            ++year;
        }
    }
    else
    {
        leap = 1;
    }

    month = 1;
    while(1)
    {
        uint8_t month_days;
        if(month == 2)
        {
            month_days = leap ? 29 : 28;
        }
        else if(month <= 7)
        {
            if((month & 1) == 1)
                month_days = 31;
            else
                month_days = 30;
        }
        else
        {
            if((month & 1) == 1)
                month_days = 30;
            else
                month_days = 31;
        }

        if(days >= month_days)
        {
            ++month;
            days -= month_days;
        }
        else
        {
            break;
        }
    }

    day = days + 1;

    hours= timeval / 3600;
    timeval -= hours * 3600UL;

    minutes = timeval / 60;
    timeval -= minutes * 60;

    seconds = timeval;
    
    dti->seconds = tp_make_bcd(seconds);
    dti->minutes = tp_make_bcd(minutes);
    dti->hours = tp_make_bcd(hours);
    dti->date = tp_make_bcd(day);
    dti->month = tp_make_bcd(month);
    dti->year= tp_make_bcd(year);   
}