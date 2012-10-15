/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _TFTP_H
#define _TFTP_H
/**
 * @addtogroup app
 * @{
 * @addtogroup tftp
 * @{
 * @file tftp.h
 * @author Pawel Lebioda <pawel.lebioda89@gmail.com>
 * @brief This file contains declarations of function used by TFTP server
 * module.
 */ 
#include "app_config.h"

#if APP_TFTP

#include <stdint.h>

#include "../sys/fat.h"

#define TFTP_UDP_PORT 	69
#define TFTP_BLOCK_SIZE	512

/**
 * Initializes TFTP server
 * @param [in] wd Working directory
 * @return One if succesfully initialized, otherwise zero
 */ 
uint8_t tftpd_init(struct fat_dir_entry * wd);

/**
 * Resets TFTP server
 */ 
void tftpd_reset(void);

#endif //APP_TFTP
/**
 * @}
 * @}
 */ 
#endif //_TFTP_H
