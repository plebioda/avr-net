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

void i2c_init(void);
uint8_t i2c_write(uint8_t addr,uint8_t * data,uint16_t len);
uint8_t i2c_read(uint8_t addr,uint8_t * data,uint16_t len);
uint8_t i2c_probe(void);
#endif //_I2C_H