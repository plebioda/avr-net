/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/**
* \addtogroup arch
* @{
*/
/**
* \addtogroup i2c
* @{
*/
/**
* \file i2c implementation
* \author Paweł Lebioda <pawel.lebioda89@gmail.com>
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

/**
* \name Basic functions
* @{
*/
/**
* Sends start condition
*/
#define i2c_send_start()	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)
/**
* Waiting for i2c Interrupt indicating that latest job is completed
*/
#define i2c_wait()		while(!(TWCR & (1<<TWINT)))
/**
* Gets status byte of i2c module
*/
#define i2c_get_status()	(TWSR & 0xf8)
/**
* Sends one byte via i2c bus
*/
#define i2c_send_byte(x)	TWDR = (x); TWCR = (1<<TWINT)|(1<<TWEN)
/**
* Sends stop condition
*/
#define i2c_send_stop()		TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO)
/**
* Receives one byte from i2c bus and acknlowlegdes
*/
#define i2c_rcv_byte_ack()	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA)
/**
* Receives one byte from i2c bus and does not acknlowlegde
*/
#define i2c_rcv_byte_nack()	TWCR = (1<<TWINT)|(1<<TWEN)
/**
* i2c data register
*/ 
#define i2c_get_byte()		(TWDR)
/**
* @}
*/
#define DEBUG_MODE
#include "../debug.h"

/**
* Initializes i2c interface
* \returns void
*/
void i2c_init(void)
{
  I2C_PORT |= (1<<I2C_SDA)|(1<<I2C_SCL);
  TWSR &= (~0x03);
  TWSR |= (I2C_PRESCALER&0x3);
  TWBR = I2C_TWBR;
}
/**
* Reads number of bytes from i2c device's memory
*\param[in] dev_addr Address of i2c device 
*\param[in] mem_addr Address of first byte to read from memory
*\param[out] ptr Pointer to data block
*\param[in] len Length of data block
*\returns 0 for success or error number 
*\see Error numbers
* \note dev_addr is always write address of i2c device
*/
uint8_t i2c_read_mem(uint8_t dev_addr,uint8_t mem_addr,uint8_t * ptr,uint16_t len)
{
  i2c_send_start();
  i2c_wait();
  if(!(i2c_get_status() == I2C_SR_START || i2c_get_status() ==I2C_SR_REP_START))
    return (I2C_ERR_WRITE|I2C_ERR_START);
  i2c_send_byte((dev_addr));
  i2c_wait();
  if(i2c_get_status() != I2C_SR_SLAW_ACK)
    return (I2C_ERR_WRITE|I2C_ERR_SLA_ACK); 
  i2c_send_byte(mem_addr);
  i2c_wait();
  if(i2c_get_status() != I2C_SR_DATAW_ACK)
    return (I2C_ERR_WRITE|I2C_ERR_ACK);
  i2c_send_stop();
  i2c_send_start();
  i2c_wait();
  if(!(i2c_get_status() == I2C_SR_START || i2c_get_status() ==I2C_SR_REP_START))
    return (I2C_ERR_WRITE|I2C_ERR_START);
  i2c_send_byte((dev_addr|1));
  i2c_wait();
  if(i2c_get_status() != I2C_SR_SLAR_ACK)
    return (I2C_ERR_WRITE|I2C_ERR_SLA_ACK); 
  while(--len)
  {
      i2c_rcv_byte_ack();
      i2c_wait();
      if(i2c_get_status() != I2C_SR_DATAR_ACK)
	return (I2C_ERR_READ|I2C_ERR_ACK);  
      *(ptr++) = i2c_get_byte();
  }
  i2c_rcv_byte_nack();
  i2c_wait();
  if(i2c_get_status() != I2C_SR_DATAR_NACK)
    return (I2C_ERR_READ|I2C_ERR_NACK);  
  *(ptr++) = i2c_get_byte();
  i2c_send_stop();
  return 0;
}
/**
* Writes number of bytes to i2c device's memory
*\param[in] dev_addr Address of i2c device 
*\param[in] mem_addr Address of first byte to read from memory
*\param[in] ptr Pointer to data block
*\param[in] len Length of data block
* \note dev_addr is always write address of i2c device
*/
uint8_t i2c_write_mem(uint8_t dev_addr,uint8_t mem_addr,uint8_t * ptr,uint16_t len)
{
  i2c_send_start();
  i2c_wait();
  if(!(i2c_get_status() == I2C_SR_START || i2c_get_status() ==I2C_SR_REP_START))
    return (I2C_ERR_WRITE|I2C_ERR_START);
  i2c_send_byte((dev_addr));
  i2c_wait();
  if(i2c_get_status() != I2C_SR_SLAW_ACK)
    return (I2C_ERR_WRITE|I2C_ERR_SLA_ACK); 
  i2c_send_byte(mem_addr);
  i2c_wait();
  if(i2c_get_status() != I2C_SR_DATAW_ACK)
    return (I2C_ERR_WRITE|I2C_ERR_ACK);
  while(len--)
  {
    i2c_send_byte(*(ptr++));
    i2c_wait();
    if(i2c_get_status() != I2C_SR_DATAW_ACK)
      return (I2C_ERR_WRITE|I2C_ERR_ACK);
  }
  i2c_send_stop();
  return 0;
}

// uint8_t i2c_write_byte_ns(uint8_t addr,uint8_t data)
// {
//   i2c_send_start();
//   i2c_wait();
//   if(!(i2c_get_status() == I2C_SR_START || i2c_get_status() ==I2C_SR_REP_START))
//     return (I2C_ERR_WRITE|I2C_ERR_START);
//   i2c_send_byte((addr));
//   i2c_wait();
//   if(i2c_get_status() != I2C_SR_SLAW_ACK)
//     return (I2C_ERR_WRITE|I2C_ERR_SLA_ACK); 
//   i2c_send_byte(data);
//   i2c_wait();
//   if(i2c_get_status() != I2C_SR_DATAW_ACK)
//     return (I2C_ERR_WRITE|I2C_ERR_ACK);
//   
//   return 0;
// }
// 
// uint8_t i2c_write(uint8_t addr,uint8_t * data,uint16_t len)
// {
//   i2c_send_start();
//   i2c_wait();
//   if(!(i2c_get_status() == I2C_SR_START || i2c_get_status() ==I2C_SR_REP_START))
//     return (I2C_ERR_WRITE|I2C_ERR_START);
//   i2c_send_byte((addr));
//   i2c_wait();
//   if(i2c_get_status() != I2C_SR_SLAW_ACK)
//     return (I2C_ERR_WRITE|I2C_ERR_SLA_ACK);
//   while(len--)
//   {
// //     DEBUG_PRINT("sending %x\n",*data);
//       i2c_send_byte(*(data++));
//       i2c_wait();
//       if(i2c_get_status() != I2C_SR_DATAW_ACK)
// 	return (I2C_ERR_WRITE|I2C_ERR_ACK);
//   }
//   i2c_send_stop();
//   return 0;
// }
// uint8_t i2c_read(uint8_t addr,uint8_t * data,uint16_t len)
// {
//   i2c_send_start();
//   i2c_wait();
//   if(!(i2c_get_status() == I2C_SR_START || i2c_get_status() ==I2C_SR_REP_START))
//     return (I2C_ERR_READ|I2C_ERR_START);  
//   i2c_send_byte((addr|1));
//   i2c_wait();
//   if(i2c_get_status() != I2C_SR_SLAR_ACK)
//     return (I2C_ERR_READ|I2C_ERR_SLA_ACK);  
//   while(--len)
//   {
//       i2c_rcv_byte_ack();
//       i2c_wait();
//       if(i2c_get_status() != I2C_SR_DATAR_ACK)
// 	return (I2C_ERR_READ|I2C_ERR_ACK);  
//       *(data++) = i2c_get_byte();
//   }
//   i2c_rcv_byte_nack();
//   i2c_wait();
//   if(i2c_get_status() != I2C_SR_DATAR_NACK)
//     return (I2C_ERR_READ|I2C_ERR_NACK);  
//   *(data++) = i2c_get_byte();
//   i2c_send_stop();
//   return 0;
// }
/**
* @}
*/
/**
* @}
*/