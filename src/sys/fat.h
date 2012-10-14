/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _FAT_H
#define _FAT_H
/**
 * @addtogroup sys
 * @{
 * @addtogroup fat
 * @{
 * @file fat.h
 * @author Pawel Lebidoa <pawel.lebioda89@gmail.com>
 * @brief This header file contains declarations of public methods for fat
 * file system module
 */ 
#include "fat_config.h"

#include <stdint.h>

#include "../debug.h"

#include "partition.h"
#include <string.h>

/**
 * FAT file system handle struct
 */ 
struct fat_fs;

/**
 * FAT file handle struct
 */ 
struct fat_file;

/**
 * FAT directory handle struct
 */ 
struct fat_dir;

/**
 * FAT directory entry struct
 */ 
struct fat_dir_entry;

/**
 * Inits FAT module
 * @return One if succesfully initialized, otherwise 0
 */ 
uint8_t fat_init(void);

/**
 * Opens FAT file system from specified partition
 * @param [in] partition Partition from which file system should be opened
 * @return FAT fs handle or NULL if any error occured
 */ 
struct fat_fs * fat_open(struct partition * partition);

/**
 * Closes FAT file system
 * @param [in] fs FAT file system struct
 */ 
uint8_t	fat_close(struct fat_fs * fs);

/**
 * Returns directory entry from specified file system and specified path.
 * This struct defines location in file system. It is used with other functions
 * which take file path as an argument. Using this struct we can pass relative
 * file path to those functions.
 * @param [in] fs FAT file system struct
 * @param [in] path Absolute path to directory entry
 * @return Directory entry struct pointer which defines location in file
 * system
 */  
struct fat_dir_entry * fat_get_dir_entry(struct fat_fs * fs, const char * path);

/**
 * Changes directory of directory entry struct.
 * @param [in] dir_entry Directory entry struct
 * @param [in] path Relative or absolute path of new directory
 * @return One if directory has been succesfully changed, otherwise 0
 */ 
uint8_t fat_cd(struct fat_dir_entry * dir_entry,const char * path);

/**
 * Releases file system's location struct
 * @param [in] dir_entry Directory entry
 */ 
void fat_release_dir_entry(struct fat_dir_entry * dir_entry);

/**
 * Opens dir of specified path from specified location
 * @param [in] wd Working directory struct
 * @param [in] path Absolute or relative path to wd
 * @return Directory handle or NULL if any error occured
 */  
struct fat_dir * fat_dir_open(const struct fat_dir_entry * wd,const char * path);

/**
 * Closes directory
 * @param [in] fat_dir Directory handle
 * @return One if directory has been closed succesfully, otherwise zero
 */ 
uint8_t fat_dir_close(struct fat_dir * fat_dir);

/**
 * Reads directory entry struct from specivied directory
 * @param [in] fat_dir Directory handle
 * @param [out] dir_entry Directory entry struct
 * @return One if directory entry has been read succesfully, otherwise zero
 */ 
uint8_t fat_read_dir(struct fat_dir * fat_dir,struct fat_dir_entry * dir_entry);

/**
 * Open file of specified name from specified location
 * @param [in] wd Working directory location
 * @param [in] path Absolute or relative path to file
 * @return File handle, or NULL if any error occured.
 */ 
struct fat_file * fat_fopen(const struct fat_dir_entry * wd, const char * path);

/**
 * Closes file
 * @param [in] file File handle
 * @return One if file has been closed succesfully, otherwise zero
 */ 
uint8_t fat_fclose(struct fat_file * file);

/**
 * Reads specified amount of bytes from file to specified buffer
 * @param [in] file File handle
 * @param [out] ptr Pointer to buffer
 * @param [in] Maximum number of bytes which can be placed in buffer
 * @return Number of bytes read
 */ 
size_t fat_fread(struct fat_file * file, void * ptr, size_t size);

#if FAT_DATE_TIME_SUPPORT
uint16_t fat_get_year(uint16_t fat_date);
uint8_t	fat_get_month(uint16_t fat_date);
uint8_t fat_get_day(uint16_t fat_date);
uint8_t fat_get_hours(uint16_t fat_time);
uint8_t	fat_get_minutes(uint16_t fat_time);
uint8_t fat_get_seconds(uint16_t fat_time);
#endif
/**
 * @}
 * @}
 */ 
#endif //_FAT_H
