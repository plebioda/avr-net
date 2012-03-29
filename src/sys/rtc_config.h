/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _RTC_CONFIG_H
#define _RTC_CONFIG_H

#include "../dev/ds1338.h"

#define rtc_hal_get_date_time(x)  	ds1338_get_date_time((x))	
#define rtc_hal_set_date_time(x)  	ds1338_set_date_time((x))
#define rtc_hal_stop()			ds1338_stop();
#define rtc_hal_start()			ds1338_start();
#define rtc_hal_set_format(format)	ds1338_set_format((format));

#endif //_RTC_CONFIG_H