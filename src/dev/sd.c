/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sd.h"

#define DEBUG_MODE
#include "../debug.h"

#include <string.h>

#define SD_CS_ACTIVE()		(SD_CS_PORT&=~(1<<SD_CS))
#define SD_CS_INACTIVE()	(SD_CS_PORT|=(1<<SD_CS))

#define SDC_GOOD_CMD		0x00
#define SDC_ILLEGAL_CMD		0x04
#define SDC_FLOATING_BUS	0xff
#define SDC_BAD_RESPONSE	SDC_FLOATING_BUS
#define SDC_READ_TOKEN		0xfe

#define SD_BLOCK_LEN_64		0x0040
#define SD_BLOCK_LEN_128	0x0080
#define SD_BLOCK_LEN_256	0x0100
#define SD_BLOCK_LEN_512	0x0200

#define SD_R1			0x00
#define SD_R1b			0x01
#define SD_R2			0x02
#define SD_R3			0x03
#define SD_NO_DATA		0x00
#define SD_MORE_DATA		0x01


#define SD_CMD_TIMEOUT		10
#define SD_IDLE_WAIT_MAX	0xfff
#define SD_READ_CSDCID_TIMEOUT	0x2ff
#define SD_READ_BLK_TIMEOUT	0x2fff

#define SD_GET_CSD_STRUCTURE(sdc)		(((sdc)->CSD.field[0]>>6)&3)
#define SD_GET_CSD_TAAC(sdc)			((sdc)->CSD.field[1])
#define SD_GET_CSD_NSAC(sdc)			((sdc)->CSD.field[2])
#define SD_GET_CSD_TRANS_SPEED(sdc)		((sdc)->CSD.field[3])
#define SD_GET_CSD_CCC(sdc)			(((sdc)->CSD.field[4]<<4)|((sdc)->CSD.field[5]>>4))
#define SD_GET_CSD_READ_BL_LEN(sdc)		((sdc)->CSD.field[5]&0xf)
#define SD_GET_CSD_READ_BL_PARTIAL(sdc)		(((sdc)->CSD.field[6]>>7)&0x1)
#define SD_GET_CSD_WRITE_BLK_MISSALIGN(sdc)	(((sdc)->CSD.field[6]>>6)&0x1)
#define SD_GET_CSD_READ_BLK_MISSALIGN(sdc)	(((sdc)->CSD.field[6]>>5)&0x1)
#define SD_GET_CSD_DSR_IMP(sdc)			(((sdc)->CSD.field[6]>>4)&0x1)
#define SD_GET_CSD_C_SIZE(sdc)			(((uint16_t)(sdc->CSD.field[6]&0x3)<<10)|(((uint16_t)sdc->CSD.field[7])<<2)|(sdc->CSD.field[8]&0x3))	
#define SD_GET_CSD_VDD_R_CURR_MIN(sdc)		(((sdc)->CSD.field[8]>>3)&0x7)
#define SD_GET_CSD_VDD_R_CURR_MAX(sdc)		(((sdc)->CSD.field[8])&0x7)
#define SD_GET_CSD_VDD_W_CURR_MIN(sdc)		(((sdc)->CSD.field[9]>>5)&0x7)
#define SD_GET_CSD_VDD_W_CURR_MAX(sdc)		(((sdc)->CSD.field[9]>>2)&0x7)
#define SD_GET_CSD_C_SIZE_MULT(sdc)		((((sdc)->CSD.field[9]&0x3)<<1)|((sdc)->CSD.field[10]>>7))
#define SD_GET_CSD_ERASE_BLK_EN(sdc)		(((sdc)->CSD.field[10]>>6)&0x1)
#define SD_GET_CSD_SECTOR_SIZE(sdc)		((((sdc)->CSD.field[10]&0x3f)<<1)|((sdc)->CSD.field[11]>>7))
#define SD_GET_CSD_WP_GRP_SIZE(sdc)		(((sdc)->CSD.field[11]&0x7f))
#define SD_GET_CSD_WP_GRP_ENABLE(sdc)		(((sdc)->CSD.field[12]>>7)&0x1)
#define SD_GET_CSD_R2W_FACTOR(sdc)		(((sdc)->CSD.field[12]>>2)&0x7)
#define SD_GET_CSD_WRITE_BL_LEN(sdc)		((((sdc)->CSD.field[12]&0x3)<<2)|(((sdc)->CSD.field[13]>>6)&0x3))
#define SD_GET_CSD_WRITE_BL_PARTIAL(sdc)	(((sdc)->CSD.field[13]>>5)&0x1)
#define SD_GET_CSD_FILE_FORMAT_GRP(sdc)		(((sdc)->CSD.field[14]>>7)&0x1)
#define SD_GET_CSD_COPY(sdc)			(((sdc)->CSD.field[14]>>6)&0x1)
#define SD_GET_CSD_PERM_WRITE_PROTECT(sdc)	(((sdc)->CSD.field[14]>>5)&0x1)
#define SD_GET_CSD_TMP_WRITE_PROTECT(sdc)	(((sdc)->CSD.field[14]>>4)&0x1)
#define SD_GET_CSD_FILE_FORMAT(sdc)		(((sdc)->CSD.field[14]>>2)&0x3)
#define SD_GET_CSD_CRC(sdc)			((sdc)->CSD.field[15]>>1)

struct sd_scr
{
  uint8_t	reserved[6];
  uint8_t	field[2];
};

#define SD_BUS_WIDTHS(x)		(((x)->field[0])&0xf)
#define SD_SECURITY(x)			((((x)->field[0])>>4)&0x7)
#define SD_DATA_STAT_AFTER_ERASE(x)	((((x)->field[0])>>7)&0x1)
#define SD_SPEC(x)			(((x)->field[1])&0xf)	
#define SD_SCR_STRUCTURE(x)		((((x)->field[0])>>4)&0xf)

enum
{
  sderr_Inactive = 1,
  sderr_BusError,
  sderr_CardInit_NotInIdleState,
  sderr_CardInit_Timeout,
  sderr_CardInit_ReadOCR,
  sderr_ReadCSDCID,
  sderr_ReadCSDCID_Timeout,
  sderr_ReadCSDCID_BadToken,
  sderr_CardReadCmd_Timeout,
  sderr_ReadBlock_Timeout,
  sderr_ReadBlock_CRC
} sd_err;


//Commands

#define SD_CMD(cmd)	(((cmd)&0x3f) | 0x40)
#define SD_ACMD(cmd)	SD_CMD(cmd)

#define SDC_GO_IDLE_STATE	SD_CMD(0)	
#define SDC_SEND_OP_COND	SD_CMD(1)
#define SDC_SEND_CSD		SD_CMD(9)
#define SDC_SEND_CID		SD_CMD(10)
#define SDC_STOP_TRANSMISSION	SD_CMD(12)
#define SDC_SEND_STATUS		SD_CMD(13)
#define SDC_SET_BLOCKLEN	SD_CMD(16)
#define SDC_READ_SINGLE_BLOCK	SD_CMD(17)
#define SDC_READ_MULTI_BLOCK	SD_CMD(18)
#define SDC_WRITE_SINGLE_BLOCK	SD_CMD(24)
#define SDC_WRITE_MULTI_BLOCK	SD_CMD(25)
#define SDC_TAG_SECTOR_START	SD_CMD(32)
#define SDC_TAG_SECTOR_END	SD_CMD(33)
#define SDC_UNTAG_SECTOR	SD_CMD(34)
#define SDC_TAG_ERASE_GRP_START	SD_CMD(35)
#define SDC_TAG_ERASE_GRP_END	SD_CMD(36)
#define SDC_UNTAG_ERASE_GRP	SD_CMD(37)
#define SDC_ERASE		SD_CMD(38)
#define SDC_APP_OP_COND		SD_ACMD(41)
#define SDC_LOCK_UNLOCK		SD_ACMD(49)
#define SDC_APP_CMD		SD_CMD(55)
#define SDC_READ_OCR		SD_CMD(58)
#define SDC_CRC_ON_OFF		SD_CMD(59)

enum
{
  sdcmd_GO_IDLE_STATE,
  sdcmd_SEND_OP_COND,
  sdcmd_SEND_CSD,
  sdcmd_SEND_CID,
  sdcmd_STOP_TRANSMISSION,
  sdcmd_SEND_STATUS,
  sdcmd_SET_BLOCKLEN,
  sdcmd_READ_SNGLE_BLOCK,
  sdcmd_READ_MULTI_BLOCK,
  sdcmd_WRITE_SINGLE_BLOCK,
  sdcmd_WRITE_MULTI_BLOCK,
  sdcmd_TAG_SECTOR_START,
  sdcmd_TAG_SECTOR_END,
  sdcmd_UNTAG_SECTOR,
  sdcmd_TAG_ERASE_GRP_START,
  sdcmd_TAG_ERASE_GRP_END,
  sdcmd_UNTAG_ERASE_GRP,
  sdcmd_ERASE,
  sdcmd_LOCK_UNLOCK,
  sdcmd_APP_OP_COND,
  sdcmd_APP_CMD,
  sdcmd_READ_OCR,
  sdcmd_CRC_ON_OFF
} sccmd;


struct sd_cid
{
  uint8_t	MID;		//Manufacturer ID - 8 bits
  char		OID[2];		//OEM/Application ID - 16 bits
  char 		PNM[5];		//Product Name - 40 bits = 5 ASCII characters
  uint8_t	PRV;		//Product Revision - 8 bits
  uint8_t	PSN[4];		//Serial Number - 32 bits
  uint16_t	MDT;		//Manufacture Date Code - 12 bits
  uint8_t	CRC7;		//Checksum - 7 bits
};

struct sd_csd
{
  uint8_t	field[16];
};

struct sd_csd
{
  uint8_t	field[16];
};

static struct
{
  sd_callback 	callback;
  uint8_t 	status;
  
  uint8_t 	R1;
  uint8_t	R2;
  uint8_t	OCR[4];
  
  struct sd_cid cid;
  struct sd_csd csd;
  
  uint16_t 	block_addr;
  uint8_t 	block[SD_BLOCK_SIZE];
} sd;

static uint8_t sd_init_card(void);

uint8_t sd_init(sd_callback callback)
{
  if(!callback)
    return SD_ERR_CALLBACK;
  /* Init IO Ports*/
  /* SD CS output*/
  SD_CS_DDR |= (1<<SD_CS);
  
  /* SD DET input, no pull-up*/
  SD_DETECT_DDR &= ~(1<<SD_DETECT);
  SD_DETECT_PORT &= ~(1<<SD_DETECT);
  
  /* SD WP input, no pull-up*/
  SD_WP_DDR &= ~(1<<SD_WP);
  SD_WP_PORT &= ~(1<<SD_WP);
  
  /*Deselect SD*/
  SD_CS_INACTIVE();
  
  /* Clear sd struct */
  memset(&sd,0,sizeof(sd));
  
  /* Init sd struct */
  sd.callback = callback;
  return 0;
  
}

void sd_interrupt(void)
{
  if(!sd.callback)
    return;
  if(SD_DETECT_PIN & (1<<SD_DETECT))
  {
    sd.status &= ~(1<<SD_STATUS_INSERTED) & ~(1<<SD_STATUS_WP);
    sd.callback(sd_event_removed);
  }
  else
  {
    sd.status |= (1<<SD_STATUS_INSERTED);
    if(SD_WP_PIN & (1<<SD_WP))
    {
      sd.status |= (1<<SD_STATUS_WP);
      sd.callback(sd_event_inserted_wp);
    }
    else
    {
      sd.callback(sd_event_inserted);
    }
  }
}

uint8_t sd_init_card(void)
{
  
}
uint8_t sd_status(void)
{
  return sd.status;
}
