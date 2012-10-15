/*
 * Copyright (c) 2012 by Pawel Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/**
 * @addtogroup app
 * @{
 * @addtogroup http
 * @{
 */ 
#ifndef _HTTPD_H
#define _HTTPD_H

#include "httpd_conf.h"

#include <stdint.h>

#include "../sys/fat.h"

uint8_t httpd_start(struct fat_dir_entry * root_dir);
uint8_t httpd_restart(void);
uint8_t httpd_stop(void);


#endif //_HTTPD_H

/**
 * @}
 * @}
 */ 
