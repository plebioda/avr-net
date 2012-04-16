/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _SD_H
#define _SD_H

#include "sd_config.h"

#include <stdint.h>

enum sd_event
{
  sd_event_inserted,
  sd_event_inserted_wp,
  sd_event_removed,
  sd_event_initialized,
  sd_event_error
};

typedef void (*sd_callback)(enum sd_event event);

uint8_t sd_init(sd_callback callback);
void sd_interrupt(void);
uint8_t sd_status(void);
uint16_t sd_errno(void);
uint32_t sd_read(uint32_t addr,uint8_t * buff,uint32_t length);

#define SD_STATUS_INSERTED	0
#define SD_STATUS_WP		1
#define SD_STATUS_INITIALIZED	2

#define SD_ERR_CALLBACK		(1<<0)
#define SD_ERR_INIT		(1<<1)
  

#endif //_SD_H