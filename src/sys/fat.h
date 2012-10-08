/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _FAT_H
#define _FAT_H

#include "fat_config.h"

#include <stdint.h>

//
#include "../debug.h"

#include "partition.h"
#include <string.h>

/* Cluster entries meanings in FAT table */
#define FAT16_CLUSTER_FREE		0x0000
#define FAT16_CLUSTER_BAD		0xfff7
#define FAT16_CLUSTER_RESERVED_MIN	0xfff0
#define FAT16_CLUSTER_RESERVED_MAX	0xfff6
#define FAT16_CLUSTER_LAST_MIN		0xfff8
#define FAT16_CLUSTER_LAST_MAX		0xfff8

/* File attributes masks */
#define FAT_ATTR_READONLY		(1 << 0)
#define FAT_ATTR_HIDDEN			(1 << 1)
#define FAT_ATTR_SYSTEM			(1 << 2)
#define FAT_ATTR_VOLUME			(1 << 3)
#define FAT_ATTR_DIR			(1 << 4)
#define FAT_ATTR_ARCHIVE		(1 << 5)
#define FAT_ATTR_LFN			0xf

#define FAT_DIRENTRY_DELETED		0xe5

typedef uint16_t cluster_t;
typedef uint32_t offset_t;
// typedef uint32_t size_t;

struct fat_boot_sector
{
	uint8_t		JumpNOP[3];		//Jump Code + NOP
	uint8_t 	OEM[8];			//OEM Name
	uint16_t	SectorSize;		//Bytes per sector
	uint8_t 	ClusterSize;		//Sectors per cluster
	uint16_t	ResSectors;		//Reserved sectors
	uint8_t 	NumFATCop;		//Number of copies of fat
	uint16_t	MaxRootDirEntr;		//Maximum Root Directory Entries
	uint16_t	NumSec32MB;		//Number of Sectors in Partition Smaller than 32MB
	uint8_t		MediaDesc;		//Media descriptor
	uint16_t	SecPerFAT;		//Sectors per fat
	uint16_t	SecPerTrack;		//Sectors per track
	uint16_t	NumHeads;		//Number of heads
	uint32_t	NumHiddenSec;		//Number of Hidden Sectors in Partition
	uint32_t	NumSec;			//Number of sectors in partition
	uint16_t	LogicDriveNum;		//Logical drive number of partition
	uint8_t		extendedSig;		//Extended signature - 29h
	uint32_t 	SN;			//Serial number of partition
	uint8_t		VolName[11];		//Volume name of partition
	uint8_t		FATName[8];		//Fat name - e.x. FAT16
};


struct fat_root_dir
{
	char 		filename[8];
	char		extension[3];
	uint8_t		attributes;
	uint8_t		reserved0;
	uint8_t		creation_time_ms;
	uint16_t	creation_time;
	uint16_t 	creation_date;
	uint16_t	last_access_date;
	uint16_t	reserved1;
	uint16_t	modification_time;
	uint16_t	modification_date;
	cluster_t	first_cluster;
	uint32_t	file_size;
};

#if FAT_LFN_SUPPORT
struct fat_lfn
{
	
};
#endif

struct fat_header
{
	uint32_t 	size;
	
	offset_t 	fat_offset;
	uint32_t 	fat_size;
	
	uint16_t	sector_size;
	uint16_t 	cluster_size;
		
	offset_t	cluster_zero_offset;
	offset_t	root_dir_offset;
};

struct fat_fs
{
	struct partition * partition;
	struct fat_header header;
};

uint8_t	fat_open(struct fat_fs * fs,struct partition * partition);
uint8_t	fat_close(struct fat_fs * fs);
uint8_t	fat_read_header(struct fat_fs * fs);
cluster_t fat_get_next_cluster(struct fat_fs * fs,cluster_t cluster);
offset_t fat_cluster_offset(struct fat_fs * fs,cluster_t cluster);

#if FAT_LFN_SUPPORT
	#define FAT_NAME_LEN	32
 #else
	#define FAT_NAME_LEN	8
 #endif

struct fat_dir_entry
{
	struct fat_fs * fs;
	
	/* File name */
	char filename[FAT_NAME_LEN];
	
	/* File's attributes */
	uint8_t attributes;
	
	/* First cluster of file */
	cluster_t first_cluster;
	
	/* Total size of file in bytes*/
	uint32_t file_size;
	
	/* Total disk offset in bytes of this entry*/
	offset_t entry_offset;
	
	/* Date and time of file's creation */
#if FAT_DATE_TIME_SUPPORT && FAT_DATE_TIME_SUPPORT_CREATION
	uint16_t creation_time;
	uint16_t creation_date;
#endif

	/* Date and time of last file's modification */
#if FAT_DATE_TIME_SUPPORT && FAT_DATE_TIME_SUPPORT_MODIFICATION
	uint16_t modification_time;
	uint16_t modification_date;
#endif

	/* Date of last access to the file */
#if FAT_DATE_TIME_SUPPORT && FAT_DATE_TIME_SUPPORT_LAST_ACCESS
	uint16_t last_access_date;
#endif
};
uint8_t	fat_read_dir_entry_from_offset(struct fat_fs * fs,struct fat_dir_entry * dir_entry,offset_t offset,uint32_t length,uint8_t skip_volume);
uint8_t fat_get_dir_entry(struct fat_fs * fs,struct fat_dir_entry * dir,const char * path);

struct fat_dir
{
	struct fat_fs * fs;
	struct fat_dir_entry dir_entry;
	
	cluster_t current_cluster;
	offset_t file_offset;	
	offset_t cluster_offset;
};

uint8_t fat_dir_open(struct fat_dir * fat_dir,struct fat_dir_entry * dir_entry);
uint8_t fat_dir_close(struct fat_dir * fat_dir);

uint8_t fat_read_dir(struct fat_dir * fat_dir,struct fat_dir_entry * dir_entry);

struct fat_file
{
	
};


#if FAT_DATE_TIME_SUPPORT
uint16_t fat_get_year(uint16_t fat_date);
uint8_t	fat_get_month(uint16_t fat_date);
uint8_t fat_get_day(uint16_t fat_date);
uint8_t fat_get_hours(uint16_t fat_time);
uint8_t	fat_get_minutes(uint16_t fat_time);
uint8_t fat_get_seconds(uint16_t fat_time);
#endif

#endif //_FAT_H
