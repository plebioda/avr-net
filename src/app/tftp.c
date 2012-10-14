/*
 * Copyright (c) 2012 by Pawel Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "tftp.h"

#if APP_TFTP

#include <string.h>
#include <ctype.h>

#include "../debug.h"


enum tftp_operation
{
	/**
	 * RRQ - read request 
	 */
	tftp_op_rrq = 1,
	
	/**
	 *  WRQ - write request 
	 */
	tftp_op_wrq = 2,
	
	/**
	 * DATA 
	 */
	tftp_op_data = 3,
	
	/**
	 * ACK - acknowledgment 
	 */
	tftp_op_ack = 4,
	
	/**
	 * ERROR 
	 */
	tftp_op_error = 5
};

enum tftp_error
{
	/* Not defined, see error message (if any) */
	tftp_error_not_defined = 0,
	tftp_error_file_not_found = 1,
	tftp_error_access_violation = 2,
	/* Disk full or allocation exceeded */
	tftp_error_disk_full = 3,
	tftp_error_illegal_operation = 4,
	tftp_error_unknown_transfer_id = 5,
	tftp_error_file_already_exist = 6,
	tftp_error_no_such_user = 7
};

enum tftpd_state
{
	tftpd_state_listen,
	tftpd_state_receive,
	tftpd_state_sending,
	tftpd_state_ack_waiting
};

enum tftp_mode
{
	tftp_mode_none,
	tftp_mode_netascii,
	tftp_mode_octet,
	tftp_mode_mail
};

static struct tftp
{
	udp_socket_t socket;
	struct fat_dir_entry * wd;
	struct fat_file * file;
	enum tftpd_state state;
	enum tftp_mode mode;
	uint16_t last_block;
} tftpd;

void tftpd_callback(udp_socket_t socket,uint8_t * data,uint16_t length);


uint8_t tftpd_handle_wrq(uint8_t * data,uint16_t length);
uint8_t tftpd_handle_rrq(uint8_t * data,uint16_t length);
uint8_t tftpd_handle_data(uint8_t * data,uint16_t length);
uint8_t tftpd_handle_error(uint8_t * data,uint16_t length);
uint8_t tftpd_handle_ack(uint8_t * data,uint16_t length);

enum tftp_mode tftpd_get_mode(const char * mode);
uint8_t tftpd_send_error_P(enum tftp_error error,const prog_char * msg);
uint8_t tftpd_send_ack(uint16_t block_num);
uint8_t tftpd_send_block(void);

uint8_t tftpd_init(struct fat_dir_entry * wd)
{
	/* get socket */
	tftpd.socket = udp_socket_alloc(TFTP_UDP_PORT,tftpd_callback);
	if(tftpd.socket < 0)
		return 0;
	tftpd_reset();
	tftpd.wd = wd;
	return 1;
}

void tftpd_reset(void)
{
	tftpd.state = tftpd_state_listen;
	tftpd.mode = tftp_mode_none;
	tftpd.last_block = 0;
	udp_unbind_remote(tftpd.socket);
}

void tftpd_callback(udp_socket_t socket,uint8_t * data,uint16_t length)
{
	if(socket != tftpd.socket)
		return;
	uint16_t opcode = (uint16_t)data[0]<<8 | (uint16_t)data[1];
	DBG_INFO("opcode=%x\n",opcode);
	data+=2;
	length-=2;
	switch(opcode)
	{
	case tftp_op_rrq:
		DBG_INFO("Read Request\n");
		tftpd_handle_rrq(data, length);
	break;
	case tftp_op_wrq:
		if(tftpd_handle_wrq(data,length))
			tftpd.state = tftpd_state_receive;
	break;
	case tftp_op_error:
		tftpd_handle_error(data,length);
	break;
	case tftp_op_data:
		if(!tftpd_handle_data(data,length))
			tftpd_reset();
	break;
	case tftp_op_ack:
		tftpd_handle_ack(data, length);
	break;
	default:
		return;
	}
}

uint8_t tftpd_handle_ack(uint8_t * data,uint16_t length)
{
	if(tftpd.state != tftpd_state_sending)
		return 0;
	uint16_t acked_block = ntoh16(*((uint16_t*)data));
	if(acked_block != tftpd.last_block)
	{
		DBG_ERROR("acked_block != tftpd.last_block\n");
		return 0;
	}
	tftpd.last_block++;
	tftpd_send_block();
	return 0;
}

uint8_t tftpd_handle_rrq(uint8_t * data,uint16_t length)
{
	if(tftpd.state != tftpd_state_listen)
		return 0;
	char * filename = (char*)data;
	uint8_t filename_len = strlen(filename);
	char * mode = (char*)&data[1+filename_len];
	uint8_t mode_len = strlen(mode);
	uint8_t i;
	for(i=0;i<mode_len;i++)
		mode[i] = (char)tolower(mode[i]);
	tftpd.mode = tftpd_get_mode(mode);
	if(tftpd.mode == tftp_mode_none)
	{
		tftpd_send_error_P(tftp_error_illegal_operation,PSTR("invalid mode"));
		return 0;
	}	
	DBG_INFO("Filename: %s\n",filename);
	DBG_INFO("Mode: %s\n",mode);	
	tftpd.file = fat_fopen((const struct fat_dir_entry *)tftpd.wd, filename);
	if(NULL != tftpd.file)
	{
		DBG_INFO("File found\n");
		tftpd.state = tftpd_state_sending;
		tftpd.last_block = 1;
		tftpd_send_block();	
	} 
	else
	{
		DBG_ERROR("File not found\n");
		tftpd.state = tftpd_state_listen;
		tftpd_send_error_P(tftp_error_file_not_found,PSTR("File not found :( !\n"));
	}
	return 1;
}


uint8_t tftpd_handle_wrq(uint8_t * data,uint16_t length)
{
	if(tftpd.state != tftpd_state_listen)
		return 0;
	char * filename = (char*)data;
	uint8_t filename_len = strlen(filename);
	char * mode = (char*)&data[1+filename_len];
	uint8_t mode_len = strlen(mode);
	uint8_t i;
	for(i=0;i<mode_len;i++)
		mode[i] = (char)tolower(mode[i]);
	tftpd.mode = tftpd_get_mode(mode);
	if(tftpd.mode == tftp_mode_none)
	{
		tftpd_send_error_P(tftp_error_illegal_operation,PSTR("invalid mode"));
		return 0;
	}
	return tftpd_send_ack(0);
}
uint8_t tftpd_send_ack(uint16_t block_num)
{
	uint16_t * data = (uint16_t*)udp_get_buffer();
	*data = hton16(tftp_op_ack);
	data++;
	*data = hton16(block_num);
	return udp_send(tftpd.socket,4);
}

uint8_t tftpd_send_block(void)
{
	uint16_t * ptr = (uint16_t*)udp_get_buffer();
	*(ptr++) = hton16((uint16_t)tftp_op_data);
	*(ptr++) = hton16(tftpd.last_block);
	uint8_t * buffer = (uint8_t*)ptr;
	size_t read;
	if((read=fat_fread(tftpd.file, buffer, TFTP_BLOCK_SIZE)) > 0)
	{
		udp_send(tftpd.socket, 4 + read);		
	}
}

enum tftp_mode tftpd_get_mode(const char * mode)
{
	if(!strcmp_P(mode,PSTR("netascii")))
		return tftp_mode_netascii;
	if(!strcmp_P(mode,PSTR("octet")))
		return tftp_mode_octet;
	if(!strcmp_P(mode,PSTR("mail")))
		return tftp_mode_mail;
	return tftp_mode_none;
}

uint8_t tftpd_send_error_P(enum tftp_error error,const prog_char * msg)
{
	uint16_t len = 5;
	uint16_t * field = (uint16_t*)udp_get_buffer();
	*(field++) = hton16(tftp_op_error);
	*(field++) = hton16(error);
	char * data = (char*)field;
	strcpy_P(data,msg);
	len += strlen_P(msg);
	return udp_send(tftpd.socket,len);
}

uint8_t tftpd_handle_data(uint8_t * data,uint16_t length)
{
	if(tftpd.state != tftpd_state_receive || length > 514)
		return 0;
	uint16_t block = (uint16_t)data[0]<<8 | (uint16_t)data[1];
	if(block <= tftpd.last_block)
		return 0;
	data +=2;
	length -=2;
	uint16_t i;
	for(i=0;i<length;i++)
	{
		switch(tftpd.mode)
		{
			case tftp_mode_netascii:
				DEBUG_PRINT("%c",data[i]);
				break;
			case tftp_mode_octet:
				DEBUG_PRINT("%02x",data[i]);
				break;
			default:
				return 0;
		}
	}
	DEBUG_PRINT("\n");
	if(!tftpd_send_ack(block))
		return 0;
	if(length < 512)
		return 0;
	tftpd.last_block = block;
	return 1;
}

uint8_t tftpd_handle_error(uint8_t * data,uint16_t length)
{
	switch(tftpd.state)
	{
		case tftpd_state_receive:
			tftpd_reset();
			break;
		case tftpd_state_listen:
		case tftpd_state_sending:
		case tftpd_state_ack_waiting:
			break;
			
	}
	return 1;
}

#endif //APP_TFTP
