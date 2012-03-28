/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "i2c.h"
#include <avr/io.h>

#define I2C_SR_START		0x08
#define I2C_SR_REP_START	0x10

#define I2C_SR_SLAW_ACK		0x18
#define I2C_SR_SLAR_ACK		0x40
#define I2C_SR_SLAW_NACK	0x20
#define I2C_SR_SLAR_NACK	0x48
#define I2C_SR_DATAW_ACK	0x28
#define I2C_SR_DATAW_NACK	0x30
#define I2C_SR_DATAR_ACK	0x50
#define I2C_SR_DATAR_NACK	0x58
#define I2C_SR_SLAW_ARB_LOST	0x38
#define I2C_SR_SLAR_ARB_LOST	0x38


#define i2c_send_start()	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)
#define i2c_wait()		while(!(TWCR & (1<<TWINT)))
#define i2c_get_status()	(TWSR & 0xf8)
#define i2c_send_byte(x)	TWDR = (x); TWCR = (1<<TWINT)|(1<<TWEN)
#define i2c_send_stop()		TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO)
#define i2c_rcv_byte_ack()	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA)
#define i2c_rcv_byte_nack()	TWCR = (1<<TWINT)|(1<<TWEN)
#define i2c_get_byte()		(TWDR)

void i2c_init(void)
{
  I2C_PORT |= (1<<I2C_SDA)|(1<<I2C_SCL);
  TWSR &= (~0x03);
  TWSR |= (I2C_PRESCALER&0x3);
  TWBR = I2C_TWBR;
}

uint8_t i2c_write_byte_ns(uint8_t addr,uint8_t data)
{
  i2c_send_start();
  i2c_wait();
  if(!(i2c_get_status() == I2C_SR_START || i2c_get_status() ==I2C_SR_REP_START))
    return (I2C_ERR_WRITE|I2C_ERR_START);
  i2c_send_byte((addr));
  i2c_wait();
  if(i2c_get_status() != I2C_SR_SLAW_ACK)
    return (I2C_ERR_WRITE|I2C_ERR_SLA_ACK); 
  i2c_send_byte(data);
  i2c_wait();
  if(i2c_get_status() != I2C_SR_DATAW_ACK)
    return (I2C_ERR_WRITE|I2C_ERR_ACK);
}

uint8_t i2c_write(uint8_t addr,uint8_t * data,uint16_t len)
{
  i2c_send_start();
  i2c_wait();
  if(!(i2c_get_status() == I2C_SR_START || i2c_get_status() ==I2C_SR_REP_START))
    return (I2C_ERR_WRITE|I2C_ERR_START);
  i2c_send_byte((addr));
  i2c_wait();
  if(i2c_get_status() != I2C_SR_SLAW_ACK)
    return (I2C_ERR_WRITE|I2C_ERR_SLA_ACK);
  while(len--)
  {
      i2c_send_byte(*(data++));
      i2c_wait();
      if(i2c_get_status() != I2C_SR_DATAW_ACK)
	return (I2C_ERR_WRITE|I2C_ERR_ACK);
  }
  i2c_send_stop();
  return 0;
}
uint8_t i2c_read(uint8_t addr,uint8_t * data,uint16_t len)
{
  i2c_send_start();
  i2c_wait();
  if(!(i2c_get_status() == I2C_SR_START || i2c_get_status() ==I2C_SR_REP_START))
    return (I2C_ERR_READ|I2C_ERR_START);  
  i2c_send_byte((addr|1));
  i2c_wait();
  if(i2c_get_status() != I2C_SR_SLAR_ACK)
    return (I2C_ERR_READ|I2C_ERR_SLA_ACK);  
  while(--len)
  {
      i2c_rcv_byte_ack();
      i2c_wait();
      if(i2c_get_status() != I2C_SR_DATAR_ACK)
	return (I2C_ERR_READ|I2C_ERR_ACK);  
      *(data++) = i2c_get_byte();
  }
  i2c_rcv_byte_nack();
  i2c_wait();
  if(i2c_get_status() != I2C_SR_DATAR_NACK)
    return (I2C_ERR_READ|I2C_ERR_NACK);  
  *(data++) = i2c_get_byte();
  i2c_send_stop();
  return 0;
}