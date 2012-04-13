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
  sd.errno=0;
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
    uint8_t ret = sd_init_card();
    if(ret)
    {
      /*TODO*/
      sd.errno = SD_ERR_INIT;
      sd.callback(sd_event_error);
    }
    else
      sd.callback(sd_event_initialized);
  }
}

uint8_t sd_init_card(void)
{
//   uint32_t i;
//   uint8_t retval;
//   g_sd_context.state = SD_STATE_INACTIVE;
//   sd_spi_init();
//   SD_SPI_ENABLE();
//   SD_CS_INACTIVE();
//   _delay_ms(100);
//   sd_delay(10);
//     _delay_ms(10); //10
//     retval = sd_send_cmd(sdcmd_GO_IDLE_STATE,0x0);
//     if(retval != 0)
//     {
//      goto init_error;
//     }
// //     DEBUG(fprintf_P(fRS,PSTR("GO_IDLE_STATE R1 = %x\n"),sdc->R1));
//     if(g_sd_context.R1 != 0x01)
//     {
// 	retval = sderr_CardInit_NotInIdleState;
// 	goto init_error;
//     }
//     i = SD_IDLE_WAIT_MAX;
//     do
//     {
// // 	retval = sd_send_cmd(sdc,sdcmd_APP_CMD,0x00);
// 	retval = sd_send_cmd(sdcmd_SEND_OP_COND,0x00);
// 	i--;
// 	if(retval)
// 	  i = 0;
//     }while((g_sd_context.R1 && 0x01) != 0 && i > 0);
// //     DEBUG_PRINT("sdcmd_APP_CMD retval = %x, R1 = %x\n",retval,sdc->R1);
//     //     DEBUG(fprintf_P(fRS,PSTR("SEND_OP_COND R1 = %x\n"),sdc->R1));  
//     if(i==0)
//     {
// 	retval = sderr_CardInit_Timeout;
// 	goto init_error;
//     }
//     i = SD_IDLE_WAIT_MAX;
//     do
//     {
// 	retval = sd_send_cmd(sdcmd_READ_OCR,0x0);
// 	i--;
// 	if(retval)
// 	  i=0;
//     }while(((g_sd_context.R1 && 0x01) != 0  && i > 0));
// //     DEBUG(fprintf_P(fRS,PSTR("SEND_OP_COND R1 = %x\n"),sdc->R1));  
//     if(i==0)
//     {
// 	retval = sderr_CardInit_Timeout;
// 	goto init_error;
//     }    
//     if(retval)
//       retval = sderr_CardInit_ReadOCR;
//     retval = sd_read_cid();
//     retval = sd_read_csd();
//     if(retval)
//       goto init_error;
//     g_sd_context.state = SD_STATE_ACTIVE;
// init_error:
//     SD_CS_INACTIVE();
//     return retval;
  return 0;
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
    uint8_t i;
    uint32_t timeout;
    resp_type = sd_get_resp(cmd);
    SD_CS_ACTIVE();
    spi_write(sd_get_cmd(cmd));
    spi_write(addr>>24);
    spi_write(addr>>16);
    spi_write(addr>>8);
    spi_write(addr);
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
	for(i=1;i<4;i++)
	  sd.OCR[i] = spi_read(0xff);
	sd.R1 = spi_read(0xff);
	break;
    }
    sd_delay(2);
    return 0;
}

uint8_t sd_status(void)
{
  return sd.status;
}

void sd_enable(void)
{
  SPI_ENABLE();
  SD_CS_ACTIVE();
}

void sd_disable(void)
{
  SD_CS_INACTIVE();
  SPI_DISABLE();
}

void sd_delay(uint8_t n)
{
    SD_CS_INACTIVE();
    while(n--)
      spi_write(0xff);
}
