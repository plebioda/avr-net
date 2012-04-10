/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _I2C_H
#define _I2C_H

#include "i2c_config.h"

#include <stdint.h>

#define I2C_ERR_START		0x01
#define I2C_ERR_ACK		0x02
#define I2C_ERR_SLA_ACK		0x06
#define I2C_ERR_NACK		0x04

#define I2C_ERR_WRITE		0x10
#define I2C_ERR_READ		0x20



void i2c_init(void);
// uint8_t i2c_write(uint8_t addr,uint8_t * data,uint16_t len);
// uint8_t i2c_write_byte_ns(uint8_t addr,uint8_t data);
// uint8_t i2c_read(uint8_t addr,uint8_t * data,uint16_t len);
uint8_t i2c_write_mem(uint8_t dev_addr,uint8_t mem_addr,uint8_t * ptr,uint16_t len);
uint8_t i2c_read_mem(uint8_t dev_addr,uint8_t mem_addr,uint8_t * ptr,uint16_t len);

#endif //_I2C_H