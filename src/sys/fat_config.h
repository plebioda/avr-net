/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _FAT_CONFIG_H
#define _FAT_CONFIG_H

#define FAT_MAX_FS		1
#define FAT_MAX_DIR		3
#define FAT_MAX_FILE		3
#define FAT_MAX_DIRENT		3

/* Date and time support */
#define FAT_DATE_TIME_SUPPORT	1

/* Creation date and time support (matters only if FAT_DATE_TIME_SUPPOR = 1) */
#define FAT_DATE_TIME_SUPPORT_CREATION		1
#define FAT_DATE_TIME_SUPPORT_MODIFICATION	1
#define FAT_DATE_TIME_SUPPORT_LAST_ACCESS	1


/* Long file name entries*/
#define FAT_LFN_SUPPORT		0

#endif //_FAT_CONFIG_H
