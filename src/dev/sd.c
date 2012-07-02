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

#define SD_READ_TOKEN		0xfe

#define SD_POWERON_DELAY	10
#define SD_SEND_CMD_TIMEOUT 	16
#define SD_WAIT_READY_TIMEOUT	0x7fff
#define SD_TIMEOUT_GENERIC	0x1fff

#define SD_BLOCK_LEN_64		0x0040
#define SD_BLOCK_LEN_128	0x0080
#define SD_BLOCK_LEN_256	0x0100
#define SD_BLOCK_LEN_512	0x0200

#define SD_R1			0x00
#define SD_R1b			0x01
#define SD_R2			0x02
#define SD_R3			0x03
#define SD_NO_DATA		0x00
#define SD_MORE_DATA		0x80

#define SD_R1_IDLE_STATE	0
#define SD_R1_ERASE_RESET	1
#define SD_R1_ILLEGAL_CMD	2
#define SD_R1_COM_CRC_ERR	3
#define SD_R1_ERASE_SEQ_ERR	4
#define SD_R1_ADDRESS_ERR	5
#define SD_R1_PARAM_ERR		6

#define SD_R2_CARD_LOCKED	0
#define SD_R2_WP_ERASE_SKIP	1
#define SD_R2_LOCK_UNLOCK_CMD	1
#define SD_R2_ERROR		2
#define SD_R2_CC_ERROR		3
#define SD_R2_CARD_ECC_FAILED	4
#define SD_R2_WP_VIOLATION	5
#define SD_R2_ERASE_PARAM	6
#define SD_R2_OUT_OF_RANGE	7
#define SD_R2_CSD_OVERWRITE	7
#define SD_R2_IDLE_STATE	(8+SD_R1_IDLE_STATE)
#define SD_R2_ERASE_RESET	(8+SD_R1_ERASE_RESET)
#define SD_R2_ILLEGAL_CMD	(8+SD_R1_ILLEGAL_CMD)
#define SD_R2_COM_CRC_ERR	(8+SD_R1_COM_CRC_ERR)
#define SD_R2_ERASE_SEQ_ERR	(8+SD_R1_ERASE_SEQ_ERR)
#define SD_R2_ADDRESS_ERR	(8+SD_R1_ADDRESS_ERR)
#define SD_R2_PARAM_ERR		(8+SD_R1_PARAM_ERR)

#define SD_RES_MASK		0x7f

#define SD_BLOCK_MASK		~(SD_BLOCK_SIZE-1)


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
  sderr_CmdResp_Timeout,
  sderr_ReadBlock_Timeout,
  sderr_ReadBlock_CRC,
  sderr_CardNotInserted,
  sderr_InvalidCommand,
  sderr_ReadBlock,
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

#define SD_CRC_GO_IDLE_STATE	0x95
#define SD_NOCRC	0xff
#define SDCMD_TAB_SIZE		sizeof(sdcmd_tab)/sizeof(struct sdcmd_format)

struct sd_cid
{
  uint8_t	MID;		//Manufacturer ID - 8 bits
  char		OID[2];		//OEM/Application ID - 16 bits
  char 		PNM[5];		//Product Name - 40 bits = 5 ASCII characters
  uint8_t	PRV;		//Product Revision - 8 bits
  uint8_t	PSN[4];		//Serial Number - 32 bits
  uint16_t	MDT;		//Manufacture Date Code - 12 bits
  uint8_t	CRC7;		//Checksum - 7 bits
  uint16_t 	CRC;
};

struct sd_csd
{
  uint8_t	field[16];
  uint16_t 	CRC;
};


static struct
{
  sd_callback 	callback;
  uint8_t 	status;
  
  uint8_t	OCR[4];
  
  struct sd_cid cid;
  struct sd_csd csd;

#define SD_BLOCK_STATE_INVALID	0
#define SD_BLOCK_STATE_VALID	1
  uint8_t 	block_state;
  uint16_t 	block_addr;
  uint8_t 	block[SD_BLOCK_SIZE];
  
  uint16_t errno;
} sd;

#define sd_block_addr(addr)		(addr & SD_BLOCK_MASK)
#define sd_block_offset(addr)		(addr & ~SD_BLOCK_MASK)
#define sd_valid_block()		(sd.block_state == SD_BLOCK_STATE_VALID)
#define sd_valid_block_addr(addr)	(sd_block_addr((addr)) == sd.block_addr)


static uint8_t sd_init_card(void);
static void sd_delay(uint8_t n);

uint8_t sd_send_cmd_r1_crc(uint8_t cmd,uint32_t arg,uint8_t crc);
uint16_t sd_send_cmd_r2_crc(uint8_t cmd,uint32_t arg,uint8_t crc);
#define sd_send_cmd_r1(cmd,arg)		sd_send_cmd_r1_crc(cmd,arg,SD_NOCRC)
#define sd_send_cmd_r2(cmd,arg)		sd_send_cmd_r2_crc(cmd,arg,SD_NOCRC)
static uint8_t _sd_read(uint8_t cmd,uint32_t arg,uint8_t * reg,uint16_t size);
static uint8_t sd_read_block(uint32_t addr);
#define sd_read_cid()	_sd_read(SDC_SEND_CID,0,(uint8_t*)&sd.cid,sizeof(sd.cid))
#define sd_read_csd()	_sd_read(SDC_SEND_CSD,0,(uint8_t*)&sd.csd,sizeof(sd.csd))


#define sd_select()		SD_CS_ACTIVE()
#define sd_unselect()		SD_CS_INACTIVE()

#define sd_card_removed()	(SD_DETECT_PIN & (1<<SD_DETECT))
#define sd_card_inserted()	(!sd_card_removed())

#define sd_card_protected() 	(SD_WP_PIN & (1<<SD_WP))


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
static void print_csd(void);
static void print_cid(void);
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

uint16_t sd_errno(void)
{
    return sd.errno;
}

void sd_delay(uint8_t n)
{
    while(n--)
      spi_write(0xff);
}

uint8_t sd_init(sd_callback callback)
{
  if(!callback)
    return SD_ERR_CALLBACK;
  /* Init IO Ports*/
  /* SD CS output*/
  SD_CS_DDR |= (1<<SD_CS);
  /*Deselect SD*/
  sd_unselect();
  
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

uint8_t sd_send_cmd_r1_crc(uint8_t cmd,uint32_t arg,uint8_t crc)
{
  uint8_t res;
  int16_t i = SD_SEND_CMD_TIMEOUT;
  /* wait 8 clock cycles */
  sd_delay(1);
  /* send command code */
  spi_write(cmd);
  /* send argument */
  spi_write((arg>>24) & 0xff);
  spi_write((arg>>16) & 0xff);
  spi_write((arg>> 8) & 0xff);
  spi_write((arg>> 0) & 0xff);
  /* send crc */
  spi_write(crc);
  /* get response */
  do
  {
      res = spi_read(0xff);
  }while(res==0xff && --i>0);
  return res;
}

uint16_t sd_send_cmd_r2_crc(uint8_t cmd,uint32_t arg,uint8_t crc)
{
  uint16_t res = sd_send_cmd_r1_crc(cmd,arg,crc);
  res <<= 8;
  res |= spi_read(0xff);
  return res;
}

uint8_t sd_init_card(void)
{
  int16_t i;
  uint8_t ret=0;
  sd_unselect();
  if(!sd_card_inserted())
    return sderr_CardNotInserted;
  /* wait for the card being powered */
  _delay_ms(SD_POWERON_DELAY);
  /* set low freq of spi bus */
  spi_low_frequency();
  /* card needs at least 74 clock cycles for inititalization */
  sd_delay(32);
  /* select card */
  sd_select();
  /* reset card; (go to idle state)*/
  i=SD_IDLE_WAIT_MAX;
  do
  {
    /* 
    * after power on card is in SDbus state so we have to send proper crc
    */
    ret = sd_send_cmd_r1_crc(SDC_GO_IDLE_STATE,0,SD_CRC_GO_IDLE_STATE);
  /* wait until card is not in idle state */
  }while(ret != (1<<SD_R1_IDLE_STATE) && --i>0);
  if(i==0) goto out;
  /* wait for card */
  i=SD_WAIT_READY_TIMEOUT;
  do
  {
    ret = sd_send_cmd_r1(SDC_SEND_OP_COND,0);
  /* wait until card is in idle state */
  }while((ret & (1<<SD_R1_IDLE_STATE)) && --i>0);  
  if(i==0) goto out;
  /* set block size [SD_BLOCK_SIZE]*/
  ret = sd_send_cmd_r1(SDC_SET_BLOCKLEN,SD_BLOCK_SIZE);
    /*TODO: reading cid and csd here? if yes fix selecting/unselecting card*/
  ret = sd_read_cid();
  ret = sd_read_csd();
  /* 
   * read first block (of addr 0x0000) becouse it is the most probably
   * that user will read this block at first (eg. MBR)
   */
  ret = sd_read_block(0);
out:
  sd_unselect();
  sd_delay(2);
  spi_high_frequency();
  //print_cid();
  //print_csd();
  return ret;
}
static uint8_t sd_read_block(uint32_t addr)
{
  uint8_t ret;
  addr &= SD_BLOCK_MASK;
  if(sd_valid_block() && sd_valid_block_addr(addr))
    return 0;
  ret = _sd_read(SDC_READ_SINGLE_BLOCK,addr,sd.block,sizeof(sd.block));
  if(!ret)
  {
    sd.block_state = SD_BLOCK_STATE_VALID;
    sd.block_addr=0;
  }
  else
    sd.block_state = SD_BLOCK_STATE_INVALID;
  return ret;
}
static uint8_t	_sd_read(uint8_t cmd,uint32_t arg,uint8_t * reg,uint16_t size)
{
  uint8_t ret;
  int16_t i;
  if(!sd_card_inserted())
    return sderr_CardNotInserted;
  sd_select();
  /* send cmd */
  if((ret=sd_send_cmd_r1(cmd,arg))) goto out;
  /* wait for data token */
  i=SD_TIMEOUT_GENERIC;
  do
  {
    ret = spi_read(0xff);
  }while(ret != SD_READ_TOKEN && --i>0);
  if(i==0) goto out;
  spi_read_block(reg,size,0xff);
  ret = 0;
out:
  sd_unselect();
  sd_delay(2);
  return ret;
}

uint32_t sd_read(uint32_t addr,uint8_t * buff,uint32_t length)
{
  uint32_t read_len=length;
  uint32_t tmp;
  uint32_t offset;
  if(!sd_card_inserted()) 
  {
    sd.errno = sderr_CardNotInserted;
    return 0;
  }
  while(read_len>0)
  {
    /* check if cached block is valid and one that we need */
    if(!sd_valid_block() || !sd_valid_block_addr(addr))
    {
      if(sd_read_block(addr))
      {
	sd.errno = sderr_ReadBlock;
      /* return number of bytes already read */
	return (length-read_len);
      }
    }
    /* there we have wanted block cached */
    /* get offset in cached block */
    offset = sd_block_offset(addr);
    tmp = read_len;
    if(offset + tmp > SD_BLOCK_SIZE)
      tmp = SD_BLOCK_SIZE - offset;
    memcpy(buff,&sd.block[offset],tmp);
    addr += tmp;
    buff += tmp;
    read_len -= tmp;
  }
  return length;
}

void sd_interrupt(void)
{
  if(!sd.callback)
    return;
  sd.errno=0;
  if(sd_card_removed())
  {
    sd.status = 0;
    sd.callback(sd_event_removed);
  }
  else
  {
    sd.status |= (1<<SD_STATUS_INSERTED);
    if(sd_card_protected())
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
