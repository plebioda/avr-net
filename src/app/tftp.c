/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "tftp.h"

#if APP_TFTP

#include <string.h>
#include <ctype.h>

#include "../arch/exmem.h"

//
#include "../debug.h"


enum	tftp_operation
{
	/* RRQ - read request */
	tftp_op_rrq = 1,
	/* WRQ - write request */
	tftp_op_wrq = 2,
	/* DATA */
	tftp_op_data = 3,
	/* ACK - acknowledgment */
	tftp_op_ack = 4,
	/* ERROR */
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
	tftpd_state_send,
	tftpd_state_ack_waiting
};

enum tftp_mode
{
	tftp_mode_none,
	tftp_mode_netascii,
	tftp_mode_octet,
	tftp_mode_mail
};

struct tftp
{
	udp_socket_t socket;
	enum tftpd_state state;
	enum tftp_mode mode;
	uint16_t last_block;
};

static struct tftp tftpd; // EXMEM

void tftpd_callback(udp_socket_t socket,uint8_t * data,uint16_t length);


uint8_t tftpd_handle_wrq(uint8_t * data,uint16_t length);
uint8_t tftpd_handle_data(uint8_t * data,uint16_t length);
uint8_t tftpd_handle_error(uint8_t * data,uint16_t length);

enum tftp_mode tftpd_get_mode(const char * mode);
uint8_t tftpd_send_error_P(enum tftp_error error,const prog_char * msg);
uint8_t tftpd_send_ack(uint16_t block_num);

uint8_t tftpd_init(void)
{
	/* get socket */
	tftpd.socket = udp_socket_alloc(TFTP_UDP_PORT,tftpd_callback);
	tftpd_reset();
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
	data+=2;
	length-=2;
	switch(opcode)
	{
		case tftp_op_rrq:
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
			break;
		default:
			return;
	}
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
		case tftpd_state_send:
		case tftpd_state_ack_waiting:
			break;
			
	}
	return 1;
}

#endif //APP_TFTP
