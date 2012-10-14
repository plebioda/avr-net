/*
 * Copyright (c) 2012 by Pawel Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fat.h"

/**
 * @addtogroup sys
 * @{
 * @addtogroup fat
 * @{
 * @file fat.c
 * @author Pawel Lebioda <pawel.lebioda89@gmail.com>
 * @brief This file contains definitions of all functions and structures in
 * FAT module
 */ 
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

#if FAT_LFN_SUPPORT
	#define FAT_NAME_LEN	32
#else
	#define FAT_NAME_LEN	8
#endif

/**
 * Cluster index
 */ 
typedef uint16_t cluster_t;

/**
 * Offset in file
 */
typedef uint32_t offset_t;

/**
 * FAT header
 */ 
struct fat_header
{ 
        uint32_t        size;
        
        offset_t        fat_offset;
        uint32_t        fat_size;
        
        uint16_t        sector_size;
        uint16_t        cluster_size;
                
        offset_t        cluster_zero_offset;
        offset_t        root_dir_offset;
};

struct fat_fs
{
        struct partition * partition;
        struct fat_header header;
};

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

struct fat_file
{
	struct fat_fs * fs;
	struct fat_dir_entry dir_entry;
	
	cluster_t current_cluster;
	offset_t file_offset;	
	offset_t cluster_offset;
	cluster_t file_cluster;
};

struct fat_dir
{
	struct fat_fs * fs;
	struct fat_dir_entry dir_entry;
	
	cluster_t current_cluster;
	offset_t file_offset;	
	offset_t cluster_offset;
};

static struct fat_fs fat_fss[FAT_MAX_FS];
static struct fat_dir_entry fat_dir_entries[FAT_MAX_DIRENT];
static struct fat_dir fat_dirs[FAT_MAX_DIR];
static struct fat_file fat_files[FAT_MAX_FILE];

static cluster_t fat_get_next_cluster(struct fat_fs * fs,cluster_t cluster);
static offset_t fat_cluster_offset(struct fat_fs * fs,cluster_t cluster);
static uint8_t fat_read_header(struct fat_fs * fs);
static uint8_t _fat_get_dir_entry(struct fat_fs * fs,struct fat_dir_entry * dir_entry,const char * path);
static uint8_t _fat_dir_open(struct fat_dir * fat_dir,struct fat_dir_entry * dir_entry);
static uint8_t _fat_file_open(struct fat_file * file, struct fat_dir_entry * dir_entry);


#ifdef DEBUG_MODE
void print_fat_header(struct fat_header * fh);
void print_fat_root_dir(struct fat_root_dir * frd);
void print_fat_boot_sector(struct fat_boot_sector * fbs);
#endif

static uint8_t	fat_read_dir_entry_from_offset(
	struct fat_fs * fs,
	struct fat_dir_entry * dir_entry,
	offset_t offset,
	uint32_t length,
	uint8_t skip_volume);

uint8_t fat_init(void)
{
	memset(fat_fss, 0, sizeof(fat_fss));
	memset(fat_dir_entries, 0, sizeof(fat_dir_entries));
	memset(fat_files, 0, sizeof(fat_files));
	memset(fat_dirs, 0, sizeof(fat_dirs));
	return 1;
}

struct fat_fs * fat_open(struct partition * partition)
{
	if(!partition)
		return NULL;
	uint8_t i;
	for(i = 0 ; i < FAT_MAX_FS ; i++) 
	{
		if(NULL == fat_fss[i].partition)
			break;
	}
	if(i < 0 || i >= FAT_MAX_FS) 
		return NULL;
	struct fat_fs * fs = &fat_fss[i];
	fs->partition = partition;
	if(!fat_read_header(fs))
		return NULL;
	return fs;
}

uint8_t	fat_close(struct fat_fs * fs)
{
	if(!fs)
		return 0;
	fs->partition = 0;
	return 1;
}

void fat_release_dir_entry(struct fat_dir_entry * dir_entry)
{
	if(!dir_entry)
		return;
	memset(dir_entry, 0 ,sizeof(*dir_entry));
}


struct fat_dir_entry * fat_get_dir_entry(struct fat_fs * fs, const char * path)
{
	if(!fs || !path || 0 == strlen(path))
		return NULL;
	uint8_t i;
	for(i=0;i<FAT_MAX_DIRENT;i++)
	{
		if(NULL == fat_dir_entries[i].fs)
 			break;
	}
	if(i < 0 || i >= FAT_MAX_DIRENT)
		return NULL;
	struct fat_dir_entry * dir_entry = &fat_dir_entries[i];

	if(!_fat_get_dir_entry(fs, dir_entry, path)) 
	{
		memset(dir_entry, 0, sizeof(*dir_entry));
		return NULL;		
	}
	
	return dir_entry;
}

uint8_t fat_cd(struct fat_dir_entry * dir_entry,const char * path)
{
	DBG_INFO("path=%s\n",path);
	if(!dir_entry || !dir_entry->fs || !path || 0 == strlen(path) ||
		!(dir_entry->attributes == FAT_ATTR_DIR || dir_entry->attributes == FAT_ATTR_VOLUME))
	{
		DBG_INFO("return 0\n");
		return 0;
	}
	struct fat_dir_entry new_dir_entry;
	memcpy(&new_dir_entry, dir_entry, sizeof(new_dir_entry));
	
	if(_fat_get_dir_entry(dir_entry->fs, &new_dir_entry, path) && new_dir_entry.attributes == FAT_ATTR_DIR)
	{
		memcpy(dir_entry, &new_dir_entry, sizeof(new_dir_entry));
		return 1;
	}
	
	return 0;
}

uint8_t fat_dir_close(struct fat_dir * fat_dir)
{
	memset(fat_dir, 0, sizeof(*fat_dir));
	return 1;
}

struct fat_dir * fat_dir_open(const struct fat_dir_entry * wd, const char * path)
{
	if(!wd || !wd->fs || !path || 0 == strlen(path))
		return NULL;
	struct fat_dir_entry dirent;
	memcpy(&dirent, wd, sizeof(*wd));
	if(!_fat_get_dir_entry(dirent.fs, &dirent, path))
		return NULL;
	uint8_t i;
	for(i=0;i<FAT_MAX_DIR;i++)
	{	
		if(NULL == fat_dirs[i].fs)
			break;
	}
	if(i < 0 || i >= FAT_MAX_DIR)
		return NULL;
	struct fat_dir * dir = &fat_dirs[i];
	if(!_fat_dir_open(dir,&dirent))
	{
		memset(dir, 0, sizeof(*dir));
		return NULL;
	}
	
	return dir;
}

struct fat_file * fat_fopen(const struct fat_dir_entry * wd, const char * path)
{
	if(!wd || !wd->fs || !path || 0 == strlen(path))
		return NULL;
	DBG_INFO("path=%s\n",path);
	struct fat_dir_entry dirent;
	memcpy(&dirent, wd, sizeof(*wd));
	if(!_fat_get_dir_entry(dirent.fs, &dirent, path))
		return NULL;
	uint8_t i;
	for(i=0;i<FAT_MAX_FILE;i++)
	{
		if(NULL == fat_files[i].fs)
			break;
	}	
	if(i < 0 || i >= FAT_MAX_FILE)
		return NULL;
	struct fat_file * file = &fat_files[i];
	if(!_fat_file_open(file, &dirent))
	{
		memset(file, 0, sizeof(*file));
		return NULL;
	}
	
	return file;
}

uint8_t fat_fclose(struct fat_file * file)
{
	if(!file)
		return 0;
	memset(file, 0, sizeof(*file));
	return 1;
}

size_t fat_fread(struct fat_file * file, void * ptr, size_t size)
{
	if(!file || !ptr)
		return size;
	struct fat_fs * fs = file->fs;
	struct fat_header * header = &fs->header;
	uint32_t file_size = file->dir_entry.file_size;
	DBG_INFO("current_cluster=%d\n",file->current_cluster);
	DBG_INFO("file_offset=%d\n",file->file_offset);
	DBG_INFO("cluster_offset=%d\n",file->cluster_offset);
	DBG_INFO("size=%d\n",size);
	DBG_INFO("cluster_size=%d\n",header->cluster_size);
	DBG_INFO("file_size=%d\n",file_size);
	if(file->file_offset >= file_size)
	{
		DBG_INFO("EOF\n");
		file->file_offset = file_size;
		return 0;
	}
	if(file->file_offset + size > file->dir_entry.file_size)
	{
		size = file->dir_entry.file_size - file->file_offset;
		DBG_INFO("new size=%d\n",size);
	}
	
	size_t read = 0;
	size_t read_size = 0;
	DBG_INFO("while{\n");
	while(read < size)
	{
		DBG_INFO("\tcurrent_cluster=%d\n",file->current_cluster);
		DBG_INFO("\tfile_offset=%d\n",file->file_offset);
		DBG_INFO("\tcluster_offset=%d\n",file->cluster_offset);
		DBG_INFO("\tsize=%d\n",size);
		DBG_INFO("\tread=%d\n",read);
		read_size = size-read;
		DBG_INFO("\tread_size=%d\n",read_size);
		if(file->cluster_offset >= header->cluster_size)
		{
			file->cluster_offset = 0;
			file->current_cluster = fat_get_next_cluster(fs, file->current_cluster);
			file->file_cluster++;
			DBG_INFO("\tnext cluster=%d\n",file->current_cluster);
			if(file->current_cluster < 2)
			{
				DBG_ERROR("\tfile->current_cluster < 2\n");
				return 0;
			}
		}
		if(file->cluster_offset + read_size > header->cluster_size)
		{	
			read_size = header->cluster_size - file->cluster_offset;
			DBG_INFO("\tnew read_size=%d\n",read_size);
		}
		read_size = fs->partition->device_read(
			fat_cluster_offset(fs, file->current_cluster) + file->cluster_offset,
			ptr,
			read_size);
		DBG_INFO("device_read=%d\n",read_size);
		file->cluster_offset += read_size;
		file->file_offset += read_size;
		read += read_size;
		ptr += read_size;
	}
	DBG_INFO("} while\n");
	DBG_INFO("current_cluster=%d\n",file->current_cluster);
	DBG_INFO("file_offset=%d\n",file->file_offset);
	DBG_INFO("cluster_offset=%d\n",file->cluster_offset);
	DBG_INFO("size=%d\n",size);
	DBG_INFO("read=%d\n",read);

	return size;
}

uint8_t fat_fseek(struct fat_file * file,int32_t offset, uint8_t whence)
{
	if(!file)
		return 0;
	DBG_INFO("current_cluster=%d\n",file->current_cluster);
	DBG_INFO("file_offset=%d\n",file->file_offset);
	DBG_INFO("cluster_offset=%d\n",file->cluster_offset);
	DBG_INFO("offset=%d\n");
	uint32_t file_offset = file->file_offset;
	uint32_t file_size = file->dir_entry.file_size;
	switch(whence)
	{
	case SEEK_SET:
		DBG_INFO("SEEK_SET\n");
		file_offset = offset;
	break;
	case SEEK_CUR:
		DBG_INFO("SEEK_CUR\n");
		file_offset += offset;
	break;
	case SEEK_END:
		DBG_INFO("SEEK_END\n");
		if(offset < 0)
			return 0;
		file_offset = file_size-offset;
	break;
	default:
		return 0;
	}
	uint16_t cluster_num = file_offset / file->fs->header.cluster_size;
	offset_t cluster_offset = file_offset % file->fs->header.cluster_size;
	cluster_t cluster = file->dir_entry.first_cluster;
	if(cluster_num >= file->file_cluster)
	{
		cluster = file->current_cluster;
		cluster_num -= file->file_cluster;
	}
	DBG_INFO("first_cluster=%d\n",cluster);
	DBG_INFO("num_clusters=%d\n",cluster_num);
	DBG_INFO("cluster_offset=%d\n",cluster_offset);
	while(cluster_num--)
	{
		cluster = fat_get_next_cluster(file->fs, cluster);
		file->file_cluster++;
	}
	if(cluster < 2)
	{
		return 0;
	}
	file->current_cluster = cluster;
	file->cluster_offset = cluster_offset;
	file->file_offset = file_offset;
	return 1;
}

size_t fat_fsize(struct fat_file * file)
{
	if(!file)
		return -1;
	return file->dir_entry.file_size;
}

uint8_t fat_read_dir(struct fat_dir * fat_dir,struct fat_dir_entry * dir_entry)
{
	if(!fat_dir || !dir_entry)
		return 0;
	uint8_t ret;
	struct fat_fs * fs = fat_dir->fs;
	struct fat_header * header = &fs->header;
	cluster_t cluster_num = fat_dir->current_cluster;
	uint32_t cluster_size;
	/* Root directory */
	if(cluster_num == 0)
		cluster_size = header->cluster_zero_offset-header->root_dir_offset;
	else
		cluster_size = header->cluster_size;	 
	uint32_t data_length = sizeof(struct fat_root_dir);
	uint8_t dir_entry_found = 0;
	while(!dir_entry_found)
	{
		if(fat_dir->cluster_offset + data_length > cluster_size)
		{
			DBG_INFO("cluster: %x\n",cluster_num);
			cluster_num = fat_get_next_cluster(fs,cluster_num);
			DBG_INFO("next cluster: %x\n",cluster_num);
		}
		if(cluster_num==1)
		{
			DBG_INFO("!cluster_num: Return1\n");
			return 0;
		}
	
		offset_t read_offset = fat_dir->cluster_offset;
		if(cluster_num == 0)
			read_offset += fs->header.root_dir_offset; 
		else
			read_offset += fat_cluster_offset(fs,cluster_num);

	
		fat_dir->file_offset += data_length;
		fat_dir->cluster_offset += data_length;
		fat_dir->current_cluster = cluster_num;	
	
		DBG_INFO("Read offset %lx\n",read_offset);
		if((ret=fat_read_dir_entry_from_offset(fs,dir_entry,read_offset,data_length,1)))
			dir_entry_found = 1;
	// 	DBG_INFO("fat_read_root_dir ret = %d\n",ret);
	}
	DBG_INFO("Return2\n");
	return 1;
}

uint8_t _fat_file_open(struct fat_file * file, struct fat_dir_entry * dir_entry)
{
	DBG_INFO("fat_file=%x dir_entry=%x,attr=%x\n",file,dir_entry,dir_entry->attributes);
	if(!file || !dir_entry || dir_entry->attributes == FAT_ATTR_LFN )
		return 0;
	file->fs = dir_entry->fs;
	memcpy(&file->dir_entry, dir_entry, sizeof(*dir_entry));
	file->current_cluster = dir_entry->first_cluster;	
	file->file_offset = 0;	
	file->cluster_offset = 0;
	file->file_cluster = 0;
	return 1;	
}

uint8_t _fat_dir_open(struct fat_dir * fat_dir,struct fat_dir_entry * dir_entry)
{
	DBG_INFO("fat_dir=%x dir_entry=%x,attr=%x\n",fat_dir,dir_entry,dir_entry->attributes);
	if(!fat_dir || !dir_entry || !(dir_entry->attributes == FAT_ATTR_DIR || dir_entry->attributes == FAT_ATTR_VOLUME))
		return 0;
	fat_dir->fs = dir_entry->fs;
	memcpy(&fat_dir->dir_entry,dir_entry,sizeof(*dir_entry));
	fat_dir->current_cluster = dir_entry->first_cluster;
	fat_dir->file_offset = 0;	 
	fat_dir->cluster_offset = 0;
	return 1;
}

uint8_t	fat_read_dir_entry_from_offset(struct fat_fs * fs,
					struct fat_dir_entry * dir_entry,
					offset_t offset,
					uint32_t length,
					uint8_t skip_volume)
{
	DBG_INFO("offset=0x%lx len=%d skip volume=%d\n",offset,length,skip_volume);
	struct fat_root_dir root_dir;
	fs->partition->device_read(offset,(uint8_t*)&root_dir,length);
#ifdef DEBUG_MODE
	print_fat_root_dir(&root_dir);
#endif
	/* skip deleted of empty entries*/
	if(root_dir.filename[0] == FAT_DIRENTRY_DELETED
		|| !root_dir.filename[0]
#if !FAT_LFN_SUPPORT		
		|| root_dir.attributes == FAT_ATTR_LFN
#endif		
		|| (skip_volume && root_dir.attributes == FAT_ATTR_VOLUME)
		)
	{
		return 0;
	}
	dir_entry->attributes = root_dir.attributes;
	dir_entry->first_cluster = root_dir.first_cluster;
	dir_entry->file_size = root_dir.file_size;
	dir_entry->entry_offset = offset;
	dir_entry->fs = fs;
	memset(&dir_entry->filename,0,sizeof(root_dir.filename));
	const char * space = strchr(root_dir.filename,' ');
	uint8_t name_len;
	if(space)
	{
		name_len = space - root_dir.filename;
	}
	else
	{
		name_len = strlen(root_dir.filename);
	}
	memcpy(&dir_entry->filename,root_dir.filename,name_len);

#if FAT_DATE_TIME_SUPPORT && FAT_DATE_TIME_SUPPORT_CREATION
	dir_entry->creation_time = root_dir.creation_time;
	dir_entry->creation_date = root_dir.creation_date;
#endif

#if FAT_DATE_TIME_SUPPORT && FAT_DATE_TIME_SUPPORT_MODIFICATION
	dir_entry->modification_time = root_dir.modification_time;
	dir_entry->modification_date = root_dir.modification_date;
#endif

#if FAT_DATE_TIME_SUPPORT && FAT_DATE_TIME_SUPPORT_LAST_ACCESS
	dir_entry->last_access_date = root_dir.last_access_date;
#endif	
	return 1;
}

uint8_t _fat_get_dir_entry(struct fat_fs * fs,struct fat_dir_entry * dir_entry,const char * path)
{
	if(!fs || !path || path[0] == '\0' || !dir_entry)
		return 0;
	struct fat_dir dir;
	
	if(path[0] == '/')
	{
		DBG_INFO("begin with the root directory\n");
		/* begin with the root directory */
		memset(dir_entry,0,sizeof(*dir_entry));
		dir_entry->fs = fs;
		dir_entry->filename[0] = '/';
		dir_entry->attributes = FAT_ATTR_VOLUME;
		dir_entry->first_cluster = 0;
		dir_entry->file_size = fs->header.cluster_zero_offset - fs->header.root_dir_offset;
		path++;
	}
	while(1)
	{
		DBG_INFO("Path = %s\n",path);
		if(path[0] == '\0')
			return 1;
		const char * sub_path = strchr(path,'/');
		DBG_INFO("subPath = %s\n",sub_path);
		uint8_t length_to_sep;
		if(sub_path)
		{
			DBG_INFO("Sub path != 0\n");
			length_to_sep = sub_path - path;
			++sub_path;
		}
		else
		{
			DBG_INFO("Sub path == 0\n");
			length_to_sep = strlen(path);
			sub_path = path + length_to_sep;
		}
		DBG_INFO("Opened dir %s\n",dir_entry->filename);
		DBG_INFO("Dir entry first_cluster: %x\n",dir_entry->first_cluster);
		DBG_INFO("Dir entry size: %x\n",dir_entry->file_size);
		DBG_INFO("Dir entry offset: %lx\n",dir_entry->entry_offset);
		DBG_INFO("Dir entry name: %s\n",dir_entry->filename);
		DBG_INFO("Dir entry attr = %x\n",dir_entry->attributes);
		if(!_fat_dir_open(&dir,dir_entry))
			return 0;
		while(fat_read_dir(&dir,dir_entry))
		{
			DBG_INFO("File name = %s, length = %d\n",dir_entry->filename,strlen(dir_entry->filename));
			if(strlen(dir_entry->filename) != length_to_sep
				|| strncmp(path,dir_entry->filename,length_to_sep))
				continue;
			DBG_INFO("File found\n");			
			fat_dir_close(&dir);
			
			if(path[length_to_sep] == '\0')
			{
				DBG_INFO("Found name = %s\n",dir_entry->filename);				
				/* we parsed whole path and found the file */
				return 1;
			}
			
			if(dir_entry->attributes == FAT_ATTR_DIR)
			{
				DBG_INFO("we found parent dir\n");
				path = sub_path;
				break;
			}
			DBG_INFO("Return\n");
			return 0;
		}
		fat_dir_close(&dir);
		if(path != sub_path)
			break;
	}
	return 0;
}

uint8_t	fat_read_header(struct fat_fs * fs)
{
	struct fat_boot_sector fatbs;
	struct partition * partition;
	struct fat_header * fh;
	if(!fs)
		return 0;
	partition = fs->partition;
	
	offset_t partition_offset = (offset_t)(partition->offset << 9);
	fh = &fs->header;
	
	partition->device_read(partition_offset,(uint8_t*)&fatbs,sizeof(fatbs));
#ifdef DEBUG_MODE
	print_fat_boot_sector(&fatbs);
#endif
	uint32_t sector_count;
	if(fatbs.NumSec32MB == 0)
	{
		if(fatbs.NumSec == 0)
			return 0;
		sector_count = fatbs.NumSec;
	}
	else
	{
		sector_count = fatbs.NumSec32MB;
	}
	uint16_t sector_size = fatbs.SectorSize;
	uint16_t cluster_size = fatbs.ClusterSize;
	uint16_t reserved_sectors = fatbs.ResSectors;
	uint8_t fat_copies = fatbs.NumFATCop;
	uint16_t sectors_per_fat = fatbs.SecPerFAT;
	uint32_t max_root_entries_bytes = fatbs.MaxRootDirEntr;
	max_root_entries_bytes <<= 5;
	uint32_t data_sector_count = sector_count
				- reserved_sectors
				- (uint32_t) sectors_per_fat * fatbs.NumFATCop
				//integer number of sectors
				- ((max_root_entries_bytes + sector_size - 1) / sector_size );
	uint32_t data_cluster_count = data_sector_count / cluster_size;
	memset((uint8_t*)fh,sizeof(struct fat_header),0);
	fh->sector_size = sector_size;
	fh->size = (uint32_t) sector_size * sector_count;
	fh->cluster_size = (uint16_t) sector_size * cluster_size; 
	
	fh->fat_offset =	/* jump to partition */
				(offset_t) partition_offset 
				/* jump to fat */
				+ (offset_t) reserved_sectors * sector_size;
				
	/* the first 2 clusters are reserved*/
	fh->fat_size = ( data_cluster_count + 2 ) * sizeof(cluster_t);
	
	fh->sector_size = sector_size;
	fh->cluster_size = (uint16_t) sector_size * cluster_size;
	
	fh->root_dir_offset = fh->fat_offset 
		+(offset_t)fat_copies * sectors_per_fat * sector_size;
	fh->cluster_zero_offset = fh->root_dir_offset + (offset_t)max_root_entries_bytes;
	return 1;
}

cluster_t fat_get_next_cluster(struct fat_fs * fs,cluster_t cluster)
{
	if(!fs || cluster < 2)
		return 1;
	
	offset_t cluster_offset = fs->header.fat_offset + (offset_t)cluster*sizeof(cluster);
	cluster_t next_cluster;
	
	if(!fs->partition->device_read(cluster_offset,(uint8_t*)&next_cluster,sizeof(next_cluster)))
		return 1;
	
	
	if(next_cluster == FAT16_CLUSTER_FREE ||
		next_cluster == FAT16_CLUSTER_BAD || 
		(next_cluster >= FAT16_CLUSTER_RESERVED_MIN && next_cluster <= FAT16_CLUSTER_RESERVED_MAX) || 
		(next_cluster >= FAT16_CLUSTER_LAST_MIN && next_cluster <= FAT16_CLUSTER_LAST_MAX))
		return 1;
	
	return next_cluster;
}

offset_t fat_cluster_offset(struct fat_fs * fs,cluster_t cluster)
{
	if(!fs || cluster < 2)
		return 0;
	
	return ( fs->header.cluster_zero_offset 
	/* substract 2 becouse first two clusters are reserved and cluster_zero_offset points to 2 cluster*/
	+ (offset_t)(cluster-2)*fs->header.cluster_size);
}

#if FAT_DATE_TIME_SUPPORT
uint16_t fat_get_year(uint16_t fat_date)
{
	/* 1980 + most significant 7 bits*/
	return (1980 + ((fat_date>>9)&0x7f));
}
uint8_t	fat_get_month(uint16_t fat_date)
{
	/*4 bits from 5 to 8*/
	return ((uint8_t)((fat_date>>5)&0xf));
}
uint8_t fat_get_day(uint16_t fat_date)
{
	/*least significant 5 bits*/
	return ((uint8_t)(fat_date&0x1f));
}
uint8_t fat_get_hours(uint16_t fat_time)
{
	return ((uint8_t)((fat_time>>11)&0x1f));
}
uint8_t	fat_get_minutes(uint16_t fat_time)
{
	return ((uint8_t)((fat_time>>5)&0x3f));	
}
uint8_t fat_get_seconds(uint16_t fat_time)
{
	return ((uint8_t)(fat_time&0x1f)<<1);	
}
#endif

#ifdef DEBUG_MODE

void print_fat_header(struct fat_header * fh)
{
	DBG_INFO("Fat header: \n");
	DBG_INFO("Size: %lx\n",fh->size);
	DBG_INFO("Fat offset: %lx\n",fh->fat_offset);
	DBG_INFO("Fat size: %lx\n",fh->fat_size);
	DBG_INFO("Sector size: %x\n",fh->sector_size);
	DBG_INFO("Cluster size: %x\n",fh->cluster_size);
	DBG_INFO("Root dir offset: %lx\n",fh->root_dir_offset);
	DBG_INFO("Cluster 0 offset: %lx\n",fh->cluster_zero_offset);
}

void print_fat_root_dir(struct fat_root_dir * frd)
{
	DBG_INFO("Filename     : %s\n",frd->filename);
	DBG_INFO("Extension    : %s\n",frd->extension);
	DBG_INFO("Attributes   : %x: ",frd->attributes);
	if(frd->attributes==FAT_ATTR_LFN) {
		DEBUG_PRINT("LFN");
	} else {
		if(frd->attributes&FAT_ATTR_READONLY) 
			DEBUG_PRINT("READONLY ");
		if(frd->attributes&FAT_ATTR_HIDDEN) 
			DEBUG_PRINT("HIDDEN ");
		if(frd->attributes&FAT_ATTR_SYSTEM) 
			DEBUG_PRINT("SYSTEM ");
		if(frd->attributes&FAT_ATTR_VOLUME) 
			DEBUG_PRINT("VOLUME ");
		if(frd->attributes&FAT_ATTR_DIR) 
			DEBUG_PRINT("DIR ");
		if(frd->attributes&FAT_ATTR_ARCHIVE) 
			DEBUG_PRINT("ARCHIVE ");
	}
	DEBUG_PRINT("\n");
	DBG_INFO("First Cluster: %d\n",frd->first_cluster);	
	DBG_INFO("Size         : %d\n",frd->file_size);
}

void print_fat_boot_sector(struct fat_boot_sector * fbs)
{
	DBG_INFO("JumpNOP: %x%x%x\n",fbs->JumpNOP[0],fbs->JumpNOP[1],fbs->JumpNOP[2]);
	DBG_INFO("OEM: %c%c%c%c%c%c%c%c\n",fbs->OEM[0],fbs->OEM[1],fbs->OEM[2],fbs->OEM[3],
					fbs->OEM[4],fbs->OEM[5],fbs->OEM[6],fbs->OEM[7]);
	DBG_INFO("Sector size: %d\n",fbs->SectorSize);
	DBG_INFO("Cluster size: %d\n",fbs->ClusterSize);
	DBG_INFO("Reserved sec: %d\n",fbs->ResSectors);
	DBG_INFO("Fat copies: %d\n",fbs->NumFATCop);
	DBG_INFO("Max root dir entry: %d\n",fbs->MaxRootDirEntr);
	DBG_INFO("Num sec < 32MB: %d\n",fbs->NumSec32MB);
	DBG_INFO("Media desc: %d\n",fbs->MediaDesc);
	DBG_INFO("Sec per FAT: %d\n",fbs->SecPerFAT);
	DBG_INFO("Sec per track: %d\n",fbs->SecPerTrack);
	DBG_INFO("Num of heads: %d\n",fbs->NumHeads);
	DBG_INFO("Hidden sec: %d\n",fbs->NumHiddenSec);
	DBG_INFO("Sectors: %d\n",fbs->NumSec);
	DBG_INFO("Logic Drive num: %d\n",fbs->LogicDriveNum);
	DBG_INFO("Extended sig(0x29): %02x\n",fbs->extendedSig);
	DBG_INFO("SN: %x\n",fbs->SN);
	DBG_INFO("Vol name: %c%c%c%c%c%c%c%c%c%c%c\n",fbs->VolName[0],fbs->VolName[1],
				fbs->VolName[2],fbs->VolName[3],fbs->VolName[4],fbs->VolName[5],
				fbs->VolName[6],fbs->VolName[7],fbs->VolName[8],fbs->VolName[9],
				fbs->VolName[10]);
	DBG_INFO("Fat name: %c%c%c%c%c%c%c%c\n",fbs->FATName[0],fbs->FATName[1],
				fbs->FATName[2],fbs->FATName[3],fbs->FATName[4],fbs->FATName[5],
				fbs->FATName[6],fbs->FATName[7]);
}

#endif
