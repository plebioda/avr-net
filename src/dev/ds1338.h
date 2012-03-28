/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DS1338_H
#define _DS1338_H

#include <stdint.h>
#include "../sys/rtc.h"

#define DS1338_I2C_ADDR			0xd0
#define DS1338_I2C_ADDR_RD		0xd1
#define DS1338_I2C_ADDR_WR		0xd0

#define DS1338_REG_SECONDS		0x00
#define DS1338_REG_CLOCK_HALT		0x00
#define DS1338_REG_MINUTES		0x01
#define DS1338_REG_HOURS		0x02
#define DS1338_REG_TIME_FORMAT		0x02
#define DS1338_REG_DAY			0x03
#define DS1338_REG_DATE			0x04
#define DS1338_REG_MONTH		0x05
#define DS1338_REG_YEAR			0x06
#define DS1338_REG_CONTROL		0x07	/* The control register controls the operation of the SQW/OUT pin and provides oscillator status. */

#define DS1338_CLOCK_HALT		7
#define DS1338_GET_SEC_BCD(x)		((x)&0x7f)
#define DS1338_GET_10SEC(x)		(DS1338_GET_SEC_BCD(x)>>4)
#define DS1338_GET_SEC(x)		(DS1338_GET_SEC_BCD(x)&0xf)
#define DS1338_GET_MIN_BCD(x)		(x)
#define DS1338_GET_10MIN(x)		(DS1338_GET_MIN_BCD(x)>>4)
#define DS1338_GET_MIN(x)		(DS1338_GET_MIN_BCD(x)&0xf)
#define DS1338_GET_HOUR_BCD(x)		((x)&0x3f)
#define DS1338_GET_10HOUR(x)		(DS1338_GET_HOUR_BCD(x)>>4)
#define DS1338_GET_HOUR(x)		(DS1338_GET_HOUR_BCD(x)&0xf)
#define DS1338_TIME_FORMAT		6
#define DS1338_GET_DATE_BCD(x)		(x)
#define DS1338_GET_10DATE(x)		(DS1338_GET_DATE_BCD(x)>>4)
#define DS1338_GET_DATE(x)		(DS1338_GET_DATE_BCD(x)&0xf)
#define DS1338_GET_MONTH_BCD(x)		(x)
#define DS1338_GET_10MONTH(x)		(DS1338_GET_MONTH_BCD(x)>>4)
#define DS1338_GET_MONTH(x)		(DS1338_GET_MONTH_BCD(x)&0xf)
#define DS1338_GET_YEAR_BCD(x)		(x)
#define DS1338_GET_10YEAR(x)		(DS1338_GET_YEAR_BCD(x)>>4)
#define DS1338_GET_YEAR(x)		(DS1338_GET_YEAR_BCD(x)&0xf)
#define DS1338_CTL_OUT			7	/* Output Control */
#define DS1338_CTL_OSF			5	/* Oscillator Stop Flag */
#define DS1338_CTL_SQWE			4	/* Square-Wave Enable */
#define DS1338_CTL_RS1			1	/* Rate Select */
#define DS1338_CTL_RS0			0

/*
*	CONTROL REGISTER (07h)
*	+-------+------+------+------+------+------+------+------+------+
*	| Bit # |   7  |   6  |   5  |   4  |   3  |   2  |   1  |   0  |
*	+-------+------+------+------+------+------+------+------+------+
*	| Name  | OUT  |   0  | OSF  | SQWE |   0  |   0  |  RS1 |  RS2 |
*	+-------+------+------+------+------+------+------+------+------+
*	|  POR  |   1  |   0  |   1  |   1  |   0  |   0  |   1  |   1  |
*	+-------+------+------+------+------+------+------+------+------+
*
*
*	+-----+-----+-----+------------+------+
*	| OUT | RS1 | RS2 | SQW OUTPUT | SQWE |
*	+-----+-----+-----+------------+------+
*	|  X  |  0  |  0  |     1Hz    |   1  |
*	+-----+-----+-----+------------+------+
*	|  X  |  0  |  1  |  4.096kHz  |   1  |
*	+-----+-----+-----+------------+------+
*	|  X  |  1  |  0  |  8.192kHz  |   1  |
*	+-----+-----+-----+------------+------+
*	|  X  |  1  |  1  | 32.768kHz  |   1  |
*	+-----+-----+-----+------------+------+
*	|  0  |  X  |  X  |     0      |   0  |
*	+-----+-----+-----+------------+------+
*	|  1  |  X  |  X  |     1      |   0  |
*	+-----+-----+-----+------------+------+
*/

uint8_t ds1338_init(uint8_t format);
uint8_t ds1338_get_date_time(struct date_time * datetime);
uint8_t ds1338_set_date_time(struct date_time * datetime);
uint8_t ds1338_stop(void);
uint8_t ds1338_start(void);
uint8_t ds1338_set_format(uint8_t format);

#endif //_DS1338_H


