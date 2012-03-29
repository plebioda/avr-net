/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "ds1338.h"
#include "../arch/i2c.h"

#define DEBUG_MODE
#include "../debug.h"

#ifdef DEBUG_MODE

#include <avr/pgmspace.h>

static char * days[7] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};

void ds1338_print_time(struct date_time * dt)
{
  DEBUG_PRINT("ds1338 time: %02x:%02x:%02x",dt->hours,dt->minutes,dt->seconds);
  DEBUG_PRINT(" %s ",days[dt->day-1]);
  DEBUG_PRINT("%02x.%02x.%02x",dt->date,dt->month,dt->year);
  DEBUG_PRINT(" format = %02x\n",dt->format);
}

#define DEBUG_PRINT_TIME(dt)
#else
#define DEBUG_PRINT_TIME(dt)
#endif //DEBUG_MODE

uint8_t ds1338_init(void)
{
  uint8_t addr_ctl[2];
  //control byte address
  addr_ctl[0]=7;
  addr_ctl[1]= 	(1<<DS1338_CTL_OUT)|
		(0<<DS1338_CTL_OSF)|
		(0<<DS1338_CTL_SQWE)|
		(0<<DS1338_CTL_RS1)|
		(0<<DS1338_CTL_RS0);
  return i2c_write(DS1338_I2C_ADDR,addr_ctl,2);
}

uint8_t ds1338_get_date_time(struct date_time * date_time)
{
  uint8_t addr=0;
  uint8_t ret;
  ret = i2c_write(DS1338_I2C_ADDR,&addr,1);
  if(ret) return ret;
  ret = i2c_read(DS1338_I2C_ADDR,(uint8_t*)date_time,sizeof(struct date_time)-1);
  if(ret) return ret;
  if(date_time->hours&&(1<<DS1338_TIME_FORMAT))
  {
      date_time->hours&=~(1<<DS1338_TIME_FORMAT);
      date_time->format |= (1<<RTC_FORMAT_12_24);
      if(date_time->hours&&(1<<DS1338_TIME_PM_AM))
	date_time->format &= ~(1<<RTC_FORMAT_AM_PM);
      else
	date_time->format |= (1<<RTC_FORMAT_AM_PM);
      date_time->hours &= ~(1<<DS1338_TIME_PM_AM);
  }
  return 0;
}

uint8_t ds1338_set_date_time(struct date_time * date_time)
{
  uint8_t addr=0;
  uint8_t ret;
  ret = i2c_write(DS1338_I2C_ADDR,&addr,1);
  if(ret) return ret;
  uint8_t format = date_time->format;
  if(format&&(1<<RTC_FORMAT_12_24))
  {
    date_time->hours |= (1<<DS1338_TIME_FORMAT);
    if(format&&(1<<RTC_FORMAT_AM_PM))
      date_time->hours &= ~(1<<DS1338_TIME_PM_AM);
    else
      date_time->hours |= (1<<DS1338_TIME_PM_AM);
  }
  ret = i2c_write(DS1338_I2C_ADDR,(uint8_t*)date_time,sizeof(struct date_time)-1);
  if(ret) return ret;
  return 0;
}

uint8_t ds1338_start_stop(uint8_t start)
{
  uint8_t addr_sec[2];
  uint8_t ret;
  addr_sec[0]=DS1338_REG_CLOCK_HALT;
  ret = i2c_write(DS1338_I2C_ADDR,addr_sec,1);
  if(ret) return ret;
  ret = i2c_read(DS1338_I2C_ADDR,&addr_sec[1],1);
  if(ret) return ret;
  if(start)
    addr_sec[1] &= ~(1<<DS1338_CLOCK_HALT);
  else
    addr_sec[1] |= (1<<DS1338_CLOCK_HALT);
  return i2c_write(DS1338_I2C_ADDR,addr_sec,2); 
}

uint8_t ds1338_stop()
{
  return ds1338_start_stop(0);
}

uint8_t ds1338_start()
{
  return ds1338_start_stop(1);
}

uint8_t ds1338_set_format(uint8_t format)
{
  uint8_t addr_sec[2];
  uint8_t ret;
  addr_sec[0]=DS1338_REG_TIME_FORMAT;
  ret = i2c_write(DS1338_I2C_ADDR,addr_sec,1);
  if(ret) return ret;
  ret = i2c_read(DS1338_I2C_ADDR,&addr_sec[1],1);
  if(ret) return ret;
  if(format&(1<<RTC_FORMAT_12_24))
    addr_sec[1] |= (1<<DS1338_TIME_FORMAT);
  else
    addr_sec[1] &= ~(1<<DS1338_TIME_FORMAT);
  return i2c_write(DS1338_I2C_ADDR,addr_sec,2); 
}
