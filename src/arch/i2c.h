/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _I2C_H
#define _I2C_H
/**
* \addtogroup arch
* @{
*/
/**
* \addtogroup i2c
* @{
*/
/**
* \file i2c header
* \author Paweł Lebioda <pawel.lebioda89@gmail.com>
*/

#include "i2c_config.h"
#include <stdint.h>

/**
* \name Error numbers
* @{
*/
/**
* i2c error: start condition
*/
#define I2C_ERR_START		0x01
/**
* i2c error: ack of data
*/
#define I2C_ERR_ACK		0x02
/**
* i2c error: ack of slave address
*/
#define I2C_ERR_SLA_ACK		0x06
/**
* i2c error: not ack of las byte
*/
#define I2C_ERR_NACK		0x04
/**
* i2c error: write
*/
#define I2C_ERR_WRITE		0x10
/**
* i2c error: read
*/
#define I2C_ERR_READ		0x20
/**
* @}
*/

void i2c_init(void);
// uint8_t i2c_write(uint8_t addr,uint8_t * data,uint16_t len);
// uint8_t i2c_write_byte_ns(uint8_t addr,uint8_t data);
// uint8_t i2c_read(uint8_t addr,uint8_t * data,uint16_t len);
uint8_t i2c_write_mem(uint8_t dev_addr,uint8_t mem_addr,uint8_t * ptr,uint16_t len);
uint8_t i2c_read_mem(uint8_t dev_addr,uint8_t mem_addr,uint8_t * ptr,uint16_t len);
/**
* @}
*/
/**
* @}
*/
#endif //_I2C_H