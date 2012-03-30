/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _TP_CONFIG_H
#define _TP_CONFIG_H

#define TP_USE_TCP	0

/* local and remote UDP ports */
#define TP_REMOTE_PORT		37
/* ip address of time server */
#define TP_SERVER_IP		/*{192,168,1,11}*/{192,53,103,108}
/* waiting for connection and data timeout */
#define TP_RTX_TIMEOUT		100
/* number of max retransmissions */
#define TP_RTX_MAX		4
/* arp request timeout */
#define TP_ARP_TIMEOUT		20
#endif //_TP_CONFIG_H