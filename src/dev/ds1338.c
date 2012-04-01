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

uint8_t ds1338_init(void)
{
  uint8_t addr_ctl[2];
  /* set control byte address */
  addr_ctl[0]=7;
  /* set control byte*/
  addr_ctl[1]= 	(1<<DS1338_CTL_OUT)|
		(0<<DS1338_CTL_OSF)|
		(0<<DS1338_CTL_SQWE)|
		(0<<DS1338_CTL_RS1)|
		(0<<DS1338_CTL_RS0);
  /* write control byte*/
  return i2c_write(DS1338_I2C_ADDR,addr_ctl,2);
}

uint8_t ds1338_get_date_time(struct date_time * date_time)
{
  /* set address of first register */
  uint8_t addr=0;
  uint8_t ret;
  /* write address of first register */
  ret = i2c_write(DS1338_I2C_ADDR,&addr,1);
  if(ret) return ret;
  /* read all registers except control register */
  ret = i2c_read(DS1338_I2C_ADDR,(uint8_t*)date_time,sizeof(struct date_time)-1);
  if(ret) return ret;
  /* clear rtc format byte */
  date_time->format=0;
  /* convert ds1338 time format to RTC time format */
  if(date_time->hours&&(1<<DS1338_TIME_FORMAT))
  {
    /* 12h time format */
    /* clear 12/24h bit in hours byte */
    date_time->hours  &=~(1<<DS1338_TIME_FORMAT);
    /* set proper bit in format byte */
    date_time->format |= (1<<RTC_FORMAT_12_24);
    /* check AM/PM */
    if(date_time->hours&&(1<<DS1338_TIME_PM_AM))
      /* if PM clear proper bit in rtc format byte */
      date_time->format &= ~(1<<RTC_FORMAT_AM_PM);
    else
      /* if AM set proper bit in rtc format byte */
      date_time->format |= (1<<RTC_FORMAT_AM_PM);
    /* clear AM/PM bit in hours byte */
    date_time->hours &= ~(1<<DS1338_TIME_PM_AM);
  }
  return 0;
}

uint8_t ds1338_set_date_time(struct date_time * date_time)
{
  /* set address of first register */
  uint8_t addr=0;
  uint8_t ret;
  /* write address of first register */
  ret = i2c_write(DS1338_I2C_ADDR,&addr,1);
  if(ret) return ret;
  uint8_t format = date_time->format;
  /* convert RTC time format to ds1338 time format */
  if(format&&(1<<RTC_FORMAT_12_24))
  {
    date_time->hours |= (1<<DS1338_TIME_FORMAT);
    if(format&&(1<<RTC_FORMAT_AM_PM))
      date_time->hours &= ~(1<<DS1338_TIME_PM_AM);
    else
      date_time->hours |= (1<<DS1338_TIME_PM_AM);
  }
  /* write all ds1338 registers except control register */
  ret = i2c_write(DS1338_I2C_ADDR,(uint8_t*)date_time,sizeof(struct date_time)-1);
  if(ret) return ret;
  return 0;
}

uint8_t ds1338_start_stop(uint8_t start)
{
  uint8_t addr_sec[2];
  uint8_t ret;
  /* set address of CH bit's register*/
  addr_sec[0]=DS1338_REG_CLOCK_HALT;
  /* write address of first register to read*/
  ret = i2c_write(DS1338_I2C_ADDR,addr_sec,1);
  if(ret) return ret;
  /* read CH's register */
  ret = i2c_read(DS1338_I2C_ADDR,&addr_sec[1],1);
  if(ret) return ret;
  /* set or clear CH bit*/
  if(start)
    addr_sec[1] &= ~(1<<DS1338_CLOCK_HALT);
  else
    addr_sec[1] |= (1<<DS1338_CLOCK_HALT);
  /* write CH's register */
  return i2c_write(DS1338_I2C_ADDR,addr_sec,2); 
}

uint8_t ds1338_stop()
{
  /* stop ds1338 */
  return ds1338_start_stop(0);
}

uint8_t ds1338_start()
{
  /* start ds1338 */
  return ds1338_start_stop(1);
}

uint8_t ds1338_set_format(uint8_t format)
{
  uint8_t addr_sec[2];
  uint8_t ret;
  /* set address of time format register */
  addr_sec[0]=DS1338_REG_TIME_FORMAT;
  /* write address of time format register */
  ret = i2c_write(DS1338_I2C_ADDR,addr_sec,1);
  if(ret) return ret;
  /* read time format register */
  ret = i2c_read(DS1338_I2C_ADDR,&addr_sec[1],1);
  if(ret) return ret;
  /* set time format register */
  if(format&(1<<RTC_FORMAT_12_24))
    addr_sec[1] |= (1<<DS1338_TIME_FORMAT);
  else
    addr_sec[1] &= ~(1<<DS1338_TIME_FORMAT);
  /* write time format register */
  return i2c_write(DS1338_I2C_ADDR,addr_sec,2); 
}

#if DS1338_RAM

#define DS1338_RAM_SIZE		56
#define DS1338_RAM_OFFSET	8

int16_t ds1338_ram_write(uint16_t addr,uint8_t * data,int16_t len)
{
  if(addr >= DS1338_RAM_SIZE || len < 0)
    return -1; 
  int16_t ret = 0;
  if(addr + len > DS1338_RAM_SIZE)
  {
    len = DS1338_RAM_SIZE - addr;
    ret = len;
  }
  addr = DS1338_RAM_OFFSET + (addr);
  uint8_t a = (uint8_t)(addr&0xff);
  i2c_write(DS1338_I2C_ADDR,&a,1);
  i2c_write(DS1338_I2C_ADDR,data,len);
  return ret;
  
}
int16_t ds1338_ram_read(uint16_t addr,uint8_t * data,int16_t len)
{
  if(addr >= DS1338_RAM_SIZE || len < 0)
    return -1; 
  int16_t ret = 0;
  if(addr + len > DS1338_RAM_SIZE)
  {
    len = DS1338_RAM_SIZE - addr;
    ret = len;
  }
  addr = DS1338_RAM_OFFSET + (addr);
  uint8_t a = (uint8_t)(addr&0xff);
  DEBUG_PRINT("ds1338 read ram addr = %x, len = %d\n",a,len);
  i2c_write(DS1338_I2C_ADDR,&a,1);
  i2c_read(DS1338_I2C_ADDR,data,len);
  return ret;
}

#endif 