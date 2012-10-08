/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "ds1338.h"
#include "../arch/i2c.h"

//
#include "../debug.h"

uint8_t ds1338_init(void)
{
	uint8_t ctl;
	/* set control byte*/
	ctl= 	(1<<DS1338_CTL_OUT)|
		(0<<DS1338_CTL_OSF)|
		(0<<DS1338_CTL_SQWE)|
		(0<<DS1338_CTL_RS1)|
		(0<<DS1338_CTL_RS0);
	/* write control byte*/
	return i2c_write_mem(DS1338_I2C_ADDR,DS1338_REG_CONTROL,&ctl,1);
}

uint8_t ds1338_get_date_time(struct date_time * date_time)
{
	/* Receive all registers from ds1338 except format register */
	uint8_t ret = i2c_read_mem(DS1338_I2C_ADDR,0,(uint8_t*)date_time,sizeof(*date_time)-1);
	if(ret)
		return ret;
	/* convert ds1338 time format to RTC time format */
	if(date_time->hours&&(1<<DS1338_TIME_FORMAT))
	{
		/* 12h time format */
		/* clear 12/24h bit in hours byte */
		date_time->hours	&=~(1<<DS1338_TIME_FORMAT);
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
//	 uint8_t format = date_time->format;
	/* convert RTC time format to ds1338 time format */
//	 if(format&&(1<<RTC_FORMAT_12_24))
//	 {
//		 date_time->hours |= (1<<DS1338_TIME_FORMAT);
//		 if(format&&(1<<RTC_FORMAT_AM_PM))
//			 date_time->hours &= ~(1<<DS1338_TIME_PM_AM);
//		 else
//			 date_time->hours |= (1<<DS1338_TIME_PM_AM);
//	 }
	/* Send all registers to ds1338 except format register */
	return i2c_write_mem(DS1338_I2C_ADDR,0,(uint8_t*)date_time,sizeof(*date_time)-1);
}

uint8_t ds1338_start_stop(uint8_t start)
{
	uint8_t sec;
	uint8_t ret;
	/* read CH's register */
	ret = i2c_read_mem(DS1338_I2C_ADDR,DS1338_REG_CLOCK_HALT,&sec,1);
	if(ret) return ret;
	/* set or clear CH bit*/
	if(start)
		sec &= ~(1<<DS1338_CLOCK_HALT);
	else
		sec |= (1<<DS1338_CLOCK_HALT);
	/* write CH's register */
	return i2c_write_mem(DS1338_I2C_ADDR,DS1338_REG_CLOCK_HALT,&sec,1); 
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
	uint8_t sec;
	uint8_t ret;
	/* read time format register */
	ret = i2c_read_mem(DS1338_I2C_ADDR,DS1338_REG_TIME_FORMAT,&sec,1);
	if(ret) return ret;
	/* set time format register */
	if(format&(1<<RTC_FORMAT_12_24))
		sec |= (1<<DS1338_TIME_FORMAT);
	else
		sec &= ~(1<<DS1338_TIME_FORMAT);
	/* write time format register */
	return i2c_write_mem(DS1338_I2C_ADDR,DS1338_REG_TIME_FORMAT,&sec,1); 
}

#if DS1338_RAM
/*TODO*/
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