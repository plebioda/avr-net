/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sd.h"
#include "../arch/spi.h"

#define DEBUG_MODE
#include "../debug.h"

#include <string.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
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




#define SD_GET_CSD_STRUCTURE(csd)		(((csd).field[0]>>6)&3)
#define SD_GET_CSD_TAAC(csd)			((csd).field[1])
#define SD_GET_CSD_NSAC(csd)			((csd).field[2])
#define SD_GET_CSD_TRANS_SPEED(csd)		((csd).field[3])
#define SD_GET_CSD_CCC(csd)			(((csd).field[4]<<4)|((csd).field[5]>>4))
#define SD_GET_CSD_READ_BL_LEN(csd)		((csd).field[5]&0xf)
#define SD_GET_CSD_READ_BL_PARTIAL(csd)		(((csd).field[6]>>7)&0x1)
#define SD_GET_CSD_WRITE_BLK_MISSALIGN(csd)	(((csd).field[6]>>6)&0x1)
#define SD_GET_CSD_READ_BLK_MISSALIGN(csd)	(((csd).field[6]>>5)&0x1)
#define SD_GET_CSD_DSR_IMP(csd)			(((csd).field[6]>>4)&0x1)
#define SD_GET_CSD_C_SIZE(csd)			(((uint16_t)(csd.field[6]&0x3)<<10)|(((uint16_t)csd.field[7])<<2)|(csd.field[8]&0x3))	
#define SD_GET_CSD_VDD_R_CURR_MIN(csd)		(((csd).field[8]>>3)&0x7)
#define SD_GET_CSD_VDD_R_CURR_MAX(csd)		(((csd).field[8])&0x7)
#define SD_GET_CSD_VDD_W_CURR_MIN(csd)		(((csd).field[9]>>5)&0x7)
#define SD_GET_CSD_VDD_W_CURR_MAX(csd)		(((csd).field[9]>>2)&0x7)
#define SD_GET_CSD_C_SIZE_MULT(csd)		((((csd).field[9]&0x3)<<1)|((csd).field[10]>>7))
#define SD_GET_CSD_ERASE_BLK_EN(csd)		(((csd).field[10]>>6)&0x1)
#define SD_GET_CSD_SECTOR_SIZE(csd)		((((csd).field[10]&0x3f)<<1)|((csd).field[11]>>7))
#define SD_GET_CSD_WP_GRP_SIZE(csd)		(((csd).field[11]&0x7f))
#define SD_GET_CSD_WP_GRP_ENABLE(csd)		(((csd).field[12]>>7)&0x1)
#define SD_GET_CSD_R2W_FACTOR(csd)		(((csd).field[12]>>2)&0x7)
#define SD_GET_CSD_WRITE_BL_LEN(csd)		((((csd).field[12]&0x3)<<2)|(((csd).field[13]>>6)&0x3))
#define SD_GET_CSD_WRITE_BL_PARTIAL(csd)	(((csd).field[13]>>5)&0x1)
#define SD_GET_CSD_FILE_FORMAT_GRP(csd)		(((csd).field[14]>>7)&0x1)
#define SD_GET_CSD_COPY(csd)			(((csd).field[14]>>6)&0x1)
#define SD_GET_CSD_PERM_WRITE_PROTECT(csd)	(((csd).field[14]>>5)&0x1)
#define SD_GET_CSD_TMP_WRITE_PROTECT(csd)	(((csd).field[14]>>4)&0x1)
#define SD_GET_CSD_FILE_FORMAT(csd)		(((csd).field[14]>>2)&0x3)
#define SD_GET_CSD_CRC(csd)			((csd).field[15]>>1)

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
  sderr_ReadBlock_CRC,
  sderr_CardNotInserted
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

const uint8_t sdcmd_tab[23][4] PROGMEM = 
{
  {SDC_GO_IDLE_STATE,		0x95,	SD_R1,	SD_NO_DATA},
  {SDC_SEND_OP_COND,		0xF9,	SD_R1,	SD_NO_DATA},
  {SDC_SEND_CSD,		0xAF,	SD_R1,	SD_MORE_DATA},
  {SDC_SEND_CID,		0x1B,	SD_R1,	SD_MORE_DATA},
  {SDC_STOP_TRANSMISSION,	0xC3,	SD_R1,	SD_NO_DATA},
  {SDC_SEND_STATUS,		0xAF,	SD_R2,	SD_NO_DATA},
  {SDC_SET_BLOCKLEN,		0xFF,	SD_R1,	SD_NO_DATA},
  {SDC_READ_SINGLE_BLOCK,	0xFF,	SD_R1,	SD_MORE_DATA},
  {SDC_READ_MULTI_BLOCK,	0xFF,	SD_R1,	SD_MORE_DATA},
  {SDC_WRITE_SINGLE_BLOCK,	0xFF,	SD_R1,	SD_MORE_DATA},
  {SDC_WRITE_MULTI_BLOCK,	0xFF,	SD_R1,	SD_MORE_DATA},
  {SDC_TAG_SECTOR_START,	0xFF,	SD_R1,	SD_NO_DATA},
  {SDC_TAG_SECTOR_END,		0xFF,	SD_R1,	SD_NO_DATA},
  {SDC_UNTAG_SECTOR,		0xFF,	SD_R1,	SD_NO_DATA},
  {SDC_TAG_ERASE_GRP_START,	0xFF,	SD_R1,	SD_NO_DATA},
  {SDC_TAG_ERASE_GRP_END,	0xFF,	SD_R1,	SD_NO_DATA},
  {SDC_UNTAG_ERASE_GRP,		0xFF,	SD_R1,	SD_NO_DATA},
  {SDC_ERASE,			0xDF,	SD_R1b,	SD_NO_DATA},
  {SDC_LOCK_UNLOCK,		0x89,	SD_R1b,	SD_NO_DATA},
  {SDC_APP_OP_COND,		0xE5,	SD_R1,	SD_NO_DATA},
  {SDC_APP_CMD,			0x73,	SD_R1,	SD_NO_DATA},
  {SDC_READ_OCR,		0x25,	SD_R3,	SD_NO_DATA},
  {SDC_CRC_ON_OFF,		0x25,	SD_R1,	SD_NO_DATA}
};

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
  
  uint16_t errno;
} sd;

static uint8_t sd_init_card(void);
static void sd_enable(void);
static void sd_disable(void);
static void sd_delay(uint8_t n);
static uint8_t sd_send_cmd(uint8_t cmd,uint32_t addr);
static uint8_t sd_get_cmd(uint8_t sd_cmd);
static uint8_t sd_get_crc(uint8_t sd_cmd);
static uint8_t sd_get_resp(uint8_t sd_cmd);
static uint8_t sd_get_tdata(uint8_t sd_cmd);
void print_csd(void);
void print_cid(void);
uint8_t	sd_read_reg(uint8_t cmd);
#define sd_read_csd()	sd_read_reg(sdcmd_SEND_CSD)
#define sd_read_cid()	sd_read_reg(sdcmd_SEND_CID)
uint16_t sd_errno(void)
{
    return sd.errno;
}

uint8_t sd_init(sd_callback callback)
{
  if(!callback)
    return SD_ERR_CALLBACK;
  /* Init IO Ports*/
  /* SD CS output*/
  SD_CS_DDR |= (1<<SD_CS);
  /*Deselect SD*/
  SD_CS_INACTIVE();
  
  /* SD DET input*/
  SD_DETECT_DDR &= ~(1<<SD_DETECT);
  SD_DETECT_PORT |= (1<<SD_DETECT);
  
  /* SD WP input*/
  SD_WP_DDR &= ~(1<<SD_WP);
  SD_WP_PORT |= (1<<SD_WP);
  

  
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
  sd.errno=0;
  if(SD_DETECT_PIN & (1<<SD_DETECT))
  {
    sd.status = 0;
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
    uint8_t ret = sd_init_card();
    if(ret)
    {
      /*TODO*/
      sd.errno = ((uint16_t)ret<<8)|SD_ERR_INIT;
      sd.callback(sd_event_error);
    }
    else
    {
      sd.callback(sd_event_initialized);
      print_cid();
      print_csd();
    }
  }
}

uint8_t sd_init_card(void)
{
  if((sd.status&(1<<SD_STATUS_INSERTED))==0)
    return sderr_CardNotInserted;
  memset(&sd.cid,0,sizeof(sd.cid));
  memset(&sd.csd,0,sizeof(sd.csd));
  uint32_t i;
  uint8_t retval;
  sd_enable();
  _delay_ms(100);
  sd_delay(10);
  _delay_ms(10); //10
  retval = sd_send_cmd(sdcmd_GO_IDLE_STATE,0x0);
  if(retval != 0)
    goto init_error;
  if(sd.R1 != 0x01)
  {
    retval = sderr_CardInit_NotInIdleState;
    goto init_error;
  }
  i = SD_IDLE_WAIT_MAX;
  do
  {
//     retval = sd_send_cmd(sdc,sdcmd_APP_CMD,0x00);
    retval = sd_send_cmd(sdcmd_SEND_OP_COND,0x00);
    i--;
    if(retval)
      i = 0;
  }while((sd.R1 && 0x01) != 0 && i > 0);
  if(i==0)
  {
    retval = sderr_CardInit_Timeout;
    goto init_error;
  }
  i = SD_IDLE_WAIT_MAX;
  do
  {
    retval = sd_send_cmd(sdcmd_READ_OCR,0x0);
    i--;
    if(retval)
      i=0;
  }while(((sd.R1 && 0x01) != 0  && i > 0));
  if(i==0)
  {
    retval = sderr_CardInit_Timeout;
    goto init_error;
  }
  if(retval)
    retval = sderr_CardInit_ReadOCR;
  retval = sd_read_cid();
  retval = sd_read_csd();
  if(retval)
    goto init_error;
  sd.status |= (1<<SD_STATUS_INITIALIZED);
init_error:
  sd_disable();
  return retval;
}
uint8_t sd_get_cmd(uint8_t sd_cmd)
{
    return pgm_read_byte(&sdcmd_tab[sd_cmd][0]);
}
uint8_t sd_get_crc(uint8_t sd_cmd)
{
    return pgm_read_byte(&sdcmd_tab[sd_cmd][1]);
}
uint8_t sd_get_resp(uint8_t sd_cmd)
{
    return pgm_read_byte(&sdcmd_tab[sd_cmd][2]);
}
uint8_t sd_get_tdata(uint8_t sd_cmd)
{
    return pgm_read_byte(&sdcmd_tab[sd_cmd][3]);
}
uint8_t sd_send_cmd(uint8_t cmd,uint32_t addr)
{
    uint8_t resp_type;
    uint8_t resp;
    uint32_t timeout;
    resp_type = sd_get_resp(cmd);
    SD_CS_ACTIVE();
    spi_write(sd_get_cmd(cmd));
    spi_write_block((uint8_t*)&addr,4);
    spi_write(sd_get_crc(cmd));
    timeout = SD_CMD_RTX;
    do
    {
	resp = spi_read(0xff);
	timeout--;
    }while((resp&0x80)!=0 && timeout > 0);
    if(timeout == 0)
      return sderr_CardReadCmd_Timeout;
    switch(resp_type)
    {
      case SD_R1:
	sd.R1 = resp;
	break;
      case SD_R1b:
	sd.R1 = resp;
	break;
      case SD_R2:
	sd.R1 = resp;
	sd.R2 = spi_read(0xff);
	break;
      case SD_R3:
	sd.OCR[0] = resp;
	spi_read_block(&sd.OCR[1],3,0xff);
	sd.R1 = spi_read(0xff);
	break;
    }
    sd_delay(2);
    return 0;
}

uint8_t	sd_read_reg(uint8_t cmd)
{
    uint8_t retval;
    uint32_t rtx;
    uint8_t * ret;
    rtx = SD_IDLE_WAIT_MAX;
    do
    {
      retval = sd_send_cmd(cmd,0x00);
      rtx--;
    }while(sd.R1 != 0 && rtx > 0);
    if(retval)
      return sderr_ReadCSDCID;
    rtx = SD_READ_CSDCID_RTX;
    SD_CS_ACTIVE();
    do
    {
      retval = spi_read(0xff);
      rtx--;
    }while(retval == SDC_FLOATING_BUS && rtx > 0);
    if(rtx==0)
      return sderr_ReadCSDCID_Timeout;
    if(retval != 0xFE)
      return sderr_ReadCSDCID_BadToken;
    if(cmd == sdcmd_SEND_CID)
      ret = (uint8_t*)&sd.cid;
    else
      ret = (uint8_t*)&sd.csd;
    spi_read_block(ret,16,0xff);
    sd_delay(2);
    return 0;
}
uint8_t sd_read_block(uint32_t addr)
{
//     uint8_t retval;
//     uint16_t i;	
// #if SD_CRC_SUPPORT    
//     uint16_t	crc;
// #endif
//     if((sd.status&SD_STATUS_INITIALIZED)==0)
//     {
// 	retval = sderr_Inactive;
// 	goto sd_read_blk_err;
//     }
//     retval = sd_send_cmd(sdcmd_READ_SNGLE_BLOCK,addr);
//     if(retval)
//      goto sd_read_blk_err;
//     i = SD_READ_BLK_RTX;
//     SD_CS_ACTIVE();
//     do
//     {
//       retval = spi_read(0xff);
//       i--;
//     }while((retval != SDC_READ_TOKEN) && (i>0));
//     sd.R1 = retval;
//     if(i==0)
//     {
// 	retval = sderr_ReadBlock_Timeout;
// 	goto sd_read_blk_err;
//     }
//     for(i=0;i<512;i++)
//       sd.block[i] = spi_read(0xff);
// #if SD_CRC_SUPPORT
//     crc = sd_spi_read();
//     crc = crc<<8;
//     crc = crc | sp_spi_read();
//     if(sd_check_crc(crc))
//     {
// 	retval = sderr_ReadBlock_CRC;
// 	g_sd_context.block_state = SD_BLOCK_STATE_INVALID;
//     }
//     else
//     {
// 	retval = 0;
// 	g_sd_context.block_addr = addr;
// 	g_sd_context.block_state = SD_BLOCK_STATE_VALID;
//     }
// #else
//     sd_spi_read();
//     sd_spi_read();
//     g_sd_context.block_addr = addr;
//     g_sd_context.block_state = SD_BLOCK_STATE_VALID;
//     retval = 0;
// #endif
// sd_read_blk_err:
//     sd_delay(2);
//     return retval;
return 0;
}
uint8_t sd_status(void)
{
  return sd.status;
}

void sd_enable(void)
{
  SD_CS_ACTIVE();
}

void sd_disable(void)
{
  SD_CS_INACTIVE();
}

void sd_delay(uint8_t n)
{
    SD_CS_INACTIVE();
    while(n--)
      spi_write(0xff);
}

void print_cid(void)
{
    DEBUG_PRINT("MID = %02x\n",sd.cid.MID);
    DEBUG_PRINT("OID = %c%c\n",sd.cid.OID[0],sd.cid.OID[1]);
    DEBUG_PRINT("Product Revision = %02x\n",sd.cid.MID);
    DEBUG_PRINT("PSN = %02x%02x%02x%02x\n",sd.cid.PSN[0],sd.cid.PSN[1],sd.cid.PSN[2],sd.cid.PSN[3]);
    DEBUG_PRINT("PNM = %c%c%c%c%c\n",sd.cid.PNM[0],sd.cid.PNM[1],sd.cid.PNM[2],sd.cid.PNM[3],sd.cid.PNM[4]);
    DEBUG_PRINT("MDT = %d.20%d%d\n",(sd.cid.MDT>>8)&0xf,(sd.cid.MDT)&0xf,(sd.cid.MDT>>4)&0xf);
}

void print_csd(void)
{
    DEBUG_PRINT("CSD:\n");
    DEBUG_PRINT("CSD structure: %x\n",SD_GET_CSD_STRUCTURE(sd.csd));
    DEBUG_PRINT("TAAC: %x\n",SD_GET_CSD_TAAC(sd.csd));
    DEBUG_PRINT("NSAC: %x\n",SD_GET_CSD_NSAC(sd.csd));
    DEBUG_PRINT("TRANS_SPEED: %x\n",SD_GET_CSD_TRANS_SPEED(sd.csd));
    DEBUG_PRINT("CCC: %x\n",SD_GET_CSD_CCC(sd.csd));
    DEBUG_PRINT("READ_BL_LEN: %x\n",SD_GET_CSD_READ_BL_LEN(sd.csd));
    DEBUG_PRINT("READ_BL_PARTIAL: %x\n",SD_GET_CSD_READ_BL_PARTIAL(sd.csd));
    DEBUG_PRINT("WRITE_BLK_MISSALIGN: %x\n",SD_GET_CSD_WRITE_BLK_MISSALIGN(sd.csd));
    DEBUG_PRINT("READ_BLK_MISSALIGN: %x\n",SD_GET_CSD_READ_BLK_MISSALIGN(sd.csd));
    DEBUG_PRINT("DSR_IMP: %x\n",SD_GET_CSD_DSR_IMP(sd.csd));
    DEBUG_PRINT("C_SIZE: %x\n",SD_GET_CSD_C_SIZE(sd.csd));
    DEBUG_PRINT("VDD_R_CURR_MIN: %x\n",SD_GET_CSD_VDD_R_CURR_MIN(sd.csd));
    DEBUG_PRINT("VDD_R_CURR_MAX: %x\n",SD_GET_CSD_VDD_R_CURR_MAX(sd.csd));
    DEBUG_PRINT("VDD_W_CURR_MIN: %x\n",SD_GET_CSD_VDD_W_CURR_MIN(sd.csd));
    DEBUG_PRINT("VDD_W_CURR_MAX: %x\n",SD_GET_CSD_VDD_W_CURR_MAX(sd.csd));
    DEBUG_PRINT("C_SIZE_MULT: %x\n",SD_GET_CSD_C_SIZE_MULT(sd.csd));
    DEBUG_PRINT("ERASE_BLK_EN: %x\n",SD_GET_CSD_ERASE_BLK_EN(sd.csd));
    DEBUG_PRINT("SECTOR_SIZE: %x\n",SD_GET_CSD_SECTOR_SIZE(sd.csd));
    DEBUG_PRINT("WP_GRP_SIZE: %x\n",SD_GET_CSD_WP_GRP_SIZE(sd.csd));
    DEBUG_PRINT("WP_GRP_ENABLE: %x\n",SD_GET_CSD_WP_GRP_ENABLE(sd.csd));
    DEBUG_PRINT("R2W_FACTOR: %x\n",SD_GET_CSD_R2W_FACTOR(sd.csd));
    DEBUG_PRINT("WRITE_BL_LEN: %x\n",SD_GET_CSD_WRITE_BL_LEN(sd.csd));
    DEBUG_PRINT("WRITE_BL_PARTIAL: %x\n",SD_GET_CSD_WRITE_BL_PARTIAL(sd.csd));
    DEBUG_PRINT("FILE_FORMAT_GRP: %x\n",SD_GET_CSD_FILE_FORMAT_GRP(sd.csd));
    DEBUG_PRINT("COPY: %x\n",SD_GET_CSD_COPY(sd.csd));
    DEBUG_PRINT("PERM_WRITE_PROTECT: %x\n",SD_GET_CSD_PERM_WRITE_PROTECT(sd.csd));
    DEBUG_PRINT("TMP_WRITE_PROTECT: %x\n",SD_GET_CSD_TMP_WRITE_PROTECT(sd.csd));
    DEBUG_PRINT("FILE_FORMAT: %x\n",SD_GET_CSD_FILE_FORMAT(sd.csd));
    DEBUG_PRINT("CRC: %x\n",SD_GET_CSD_CRC(sd.csd));   
}
