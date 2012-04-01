/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _RTC_H
#define _RTC_H

#include <stdint.h>



#define RTC_FORMAT_12_24	0x1
#define RTC_FORMAT_AM_PM	0x2

struct date_time 
{
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
  uint8_t day;
  uint8_t date;
  uint8_t month;
  uint8_t year;
  uint8_t format;
};

#define rtc_get_date_time(x)		rtc_hal_get_date_time(x)
#define rtc_set_date_time(x)		rtc_hal_set_date_time(x)
#define rtc_stop()			rtc_hal_stop()
#define rtc_start()			rtc_hal_start()
#define rtc_set_format(format) 		rtc_hal_set_format(format)
#define rtc_init(format)		rtc_hal_set_format(format)

void rtc_convert_date_time(uint32_t timeval,struct date_time * dti);

#include "rtc_config.h"

#endif //_RTC_H