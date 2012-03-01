/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "tcp.h"

#include "../arch/exmem.h"

#define DEBUG_MODE
#include "../debug.h"

#include <string.h>

/* TCP Flags:
* URG:  Urgent Pointer field significant
* ACK:  Acknowledgment field significant
* PSH:  Push Function
* RST:  Reset the connection
* SYN:  Synchronize sequence numbers
* FIN:  No more data from sender
*/
#define TCP_FLAG_URG	0x20
#define TCP_FLAG_ACK	0x10
#define TCP_FLAG_PSH	0x08
#define TCP_FLAG_RST	0x04
#define TCP_FLAG_SYN	0x02
#define TCP_FLAG_FIN	0x01

/* TCP Options
* Kind     Length    Meaning
* ----     ------    -------
*  0         -       End of option list.
*  1         -       No-Operation.
*  2         4       Maximum Segment Size.
*  3         3       Window Scale (RFC 1323)
*  4         2       Selective Acknowledgement
*  8        10       Timestamps (RFC 1323)
*/
#define TCP_OPT_EOL		0
#define TCP_OPT_NOP		1
#define TCP_OPT_MSS		2
#define TCP_OPT_WS		3
#define TCP_OPT_SACK		4
#define TCP_OPT_TS		8

#define TCP_OPT_LENGTH_MSS 	4
#define TCP_OPT_LENGTH_WS	3
#define TCP_OPT_LENGTH_SACK	2
#define TCP_OPT_LENGTH_TS	10



/* TCP Header
*    0                   1                   2                   3
*    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |          Source Port          |       Destination Port        |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |                        Sequence Number                        |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |                    Acknowledgment Number                      |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |  Data |           |U|A|P|R|S|F|                               |
*   | Offset| Reserved  |R|C|S|S|Y|I|            Window             |
*   |       |           |G|K|H|T|N|N|                               |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |           Checksum            |         Urgent Pointer        |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |                    Options                    |    Padding    |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |                             data                              |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct tcp_header
{
  uint16_t port_source;
  uint16_t port_destination;
  uint32_t seq;
  uint32_t ack;
  uint8_t offset;
  uint8_t flags;
  uint16_t window;
  uint16_t checksum;
  uint16_t urgent;
};

enum tcp_state
{
    tcp_state_unused = 0,
    tcp_state_listen,
    tcp_state_syn_sent,
    tcp_state_syn_received,
    tcp_state_established,
    tcp_state_fin_wait_1,
    tcp_state_fin_wait_2,
    tcp_state_close_wait,
    tcp_state_closing,
    tcp_state_last_ack,
    tcp_state_time_wait,
    tcp_state_closed
};

/* TCP Connection State Diagram:
*
*                              +---------+ ---------\      active OPEN
*                              |  CLOSED |            \    -----------
*                              +---------+<---------\   \   create TCB
*                                |     ^              \   \  snd SYN
*                   passive OPEN |     |   CLOSE        \   \
*                   ------------ |     | ----------       \   \
*                    create TCB  |     | delete TCB         \   \
*                                V     |                      \   \
*                              +---------+            CLOSE    |    \
*                              |  LISTEN |          ---------- |     |
*                              +---------+          delete TCB |     |
*                   rcv SYN      |     |     SEND              |     |
*                  -----------   |     |    -------            |     V
* +---------+      snd SYN,ACK  /       \   snd SYN          +---------+
* |         |<-----------------           ------------------>|         |
* |   SYN   |                    rcv SYN                     |   SYN   |
* |   RCVD  |<-----------------------------------------------|   SENT  |
* |         |                    snd ACK                     |         |
* |         |------------------           -------------------|         |
* +---------+   rcv ACK of SYN  \       /  rcv SYN,ACK       +---------+
*   |           --------------   |     |   -----------
*   |                  x         |     |     snd ACK
*   |                            V     V
*   |  CLOSE                   +---------+
*   | -------                  |  ESTAB  |
*   | snd FIN                  +---------+
*   |                   CLOSE    |     |    rcv FIN
*   V                  -------   |     |    -------
* +---------+          snd FIN  /       \   snd ACK          +---------+
* |  FIN    |<-----------------           ------------------>|  CLOSE  |
* | WAIT-1  |------------------                              |   WAIT  |
* +---------+          rcv FIN  \                            +---------+
*   | rcv ACK of FIN   -------   |                            CLOSE  |
*   | --------------   snd ACK   |                           ------- |
*   V        x                   V                           snd FIN V
* +---------+                  +---------+                   +---------+
* |FINWAIT-2|                  | CLOSING |                   | LAST-ACK|
* +---------+                  +---------+                   +---------+
*   |                rcv ACK of FIN |                 rcv ACK of FIN |
*   |  rcv FIN       -------------- |    Timeout=2MSL -------------- |
*   |  -------              x       V    ------------        x       V
*    \ snd ACK                 +---------+delete TCB         +---------+
*     ------------------------>|TIME WAIT|------------------>| CLOSED  |
*                              +---------+                   +---------+
*/


/* Transmission Control Block */
struct tcp_tcb
{
    enum tcp_state state;
    tcp_socket_callback callback;
    uint16_t port_local;
    uint16_t port_remote;
    ip_address ip_remote;
    uint16_t mss;
//     uint32_t ts_val;
//     uint32_t ts_ecr;
//     uint8_t ws_shift_cnt;
    uint32_t rcv_seq;
    uint32_t snd_seq;
};

static struct tcp_tcb tcp_tcbs[TCP_MAX_SOCKETS] EXMEM;

#define FOREACH_TCB(tcb) for(tcb = &tcp_tcbs[0] ; tcb < &tcp_tcbs[TCP_MAX_SOCKETS] ; tcb++)

uint8_t tcp_send_packet(struct tcp_tcb * tcb,uint8_t flags,uint8_t send_empty);
uint8_t tcp_state_machine(struct tcp_tcb * tcb,const ip_address * ip_remote,const struct tcp_header * tcp,uint16_t length);
uint8_t tcp_send_rst(const ip_address * ip_remote,const struct tcp_header * tcp,uint16_t length);
uint8_t tcp_get_options(struct tcp_tcb * tcb,const struct tcp_header * tcp,uint16_t length);
uint16_t tcp_get_checksum(const ip_address * ip_remote,const struct tcp_header * tcp,uint16_t length);
tcp_socket_t tcp_get_socket_num(struct tcp_tcb * tcb);
uint8_t tcp_socket_valid(tcp_socket_t socket);
uint8_t tcp_free_port(uint16_t port);


uint8_t tcp_state_machine(struct tcp_tcb * tcb,const ip_address * ip_remote,const struct tcp_header * tcp,uint16_t length)
{
  if(!tcb || !ip_remote || !tcp || length < sizeof(struct tcp_header))
    return 0;
  DEBUG_PRINT_COLOR(B_IGREEN,"TCP: state machine\n");
  tcp_get_options(tcb,tcp,length);
  switch(tcb->state)
  {
    case tcp_state_listen:
      if(tcp->flags & TCP_FLAG_RST)
	return 0;
      if(tcp->flags & TCP_FLAG_ACK)
	return tcp_send_rst(ip_remote,tcp,length);
      
    default:
      break;
  }
  return 1;
}

uint8_t tcp_send_packet(struct tcp_tcb * tcb,uint8_t flags,uint8_t send_empty)
{
  if(!tcb)
    return 0;
  struct tcp_header * tcp = (struct tcp_header*)ip_get_buffer();
  memset(tcp,0,sizeof(struct tcp_header));
  tcp->port_destination = hton16(tcb->port_remote);
  tcp->port_source = hton16(tcb->port_local);
  tcp->urgent = HTON16(0x0000);
  
}

uint8_t tcp_get_options(struct tcp_tcb * tcb,const struct tcp_header * tcp,uint16_t length)
{
    if(!tcb || !tcp || length < sizeof(struct tcp_header))
      return 0;
    uint16_t offset = (tcp->offset>>4)<<2;
    uint8_t * options = (uint8_t*)tcp + sizeof(struct tcp_header);
    uint8_t * options_end = (uint8_t*)tcp + offset;
    DEBUG_PRINT_COLOR(B_IGREEN,"TCP Get Options:\n");
    for(;options < options_end;options++)
    {
	if(*options == TCP_OPT_EOL)
	{
	    /* end of options list*/
	    DEBUG_PRINT_COLOR(IGREEN,"EOL:\n");
	    break;
	}
	else if (*options == TCP_OPT_NOP)
	{
	    /* no operation */
	    DEBUG_PRINT_COLOR(IGREEN,"NOP:\n");
	}
	else if(*options == TCP_OPT_MSS && *(options+1) == TCP_OPT_LENGTH_MSS)
	{
	  /* maximum segment size */
	  options+=2;
	  tcb->mss = ntoh16(*((uint16_t*)options));
	  options++;
	  DEBUG_PRINT_COLOR(IGREEN,"MSS: %d\n",tcb->mss);
	}
// 	else if(*options == TCP_OPT_WS && *(options +1) == TCP_OPT_LENGTH_WS)
// 	{
// 	    options+=2;
// 	    tcb->ws_shift_cnt = *options;
// 	    DEBUG_PRINT_COLOR(IGREEN,"WS: %d\n",tcb->ws_shift_cnt);
// 	}
// 	else if(*options == TCP_OPT_TS && *(options+1) == TCP_OPT_LENGTH_TS)
// 	{
// 	    options+=2;
// 	    tcb->ts_val = ntoh32(*((uint32_t*)options));
// 	    options+=4;
// 	    tcb->ts_ecr = ntoh32(*((uint32_t*)options));
// 	    DEBUG_PRINT_COLOR(IGREEN,"TS: val = %d ecr = %d\n",tcb->ts_val,tcb->ts_val);
// 	}
// 	else if(*options == TCP_OPT_SACK && *(options+1) == TCP_OPT_LENGTH_SACK)
// 	{
// 	    DEBUG_PRINT_COLOR(IGREEN,"SACK\n");
// 	    options+=2;
// 	}
	else
	{
	    options += *(options+1);
	}
    }
    return 1;
}

uint8_t tcp_init(void)
{
    memset(tcp_tcbs,0,sizeof(tcp_tcbs));
    return 1;
}

uint8_t tcp_free_port(uint16_t port)
{
    struct tcp_tcb * tcb;
    FOREACH_TCB(tcb)
    {
	if(tcb->state != tcp_state_unused && tcb->port_local == port)
	  return 0;
    }	
    return 1;
}

uint8_t tcp_socket_free(tcp_socket_t socket)
{
    if(!tcp_socket_valid(socket))
      return 0;
    struct tcp_tcb * tcb = &tcp_tcbs[socket];
    /* there should be buffer_free */
    memset(tcb,0,sizeof(struct tcp_tcb));    
}

tcp_socket_t tcp_socket_alloc(tcp_socket_callback callback)
{
    struct tcp_tcb * tcb;
    tcp_socket_t socket_num = -1;
    FOREACH_TCB(tcb)
    {
	if(tcb->state != tcp_state_unused)
	  continue;
	socket_num = tcp_get_socket_num(tcb);
	if(socket_num < 0)
	  continue;
	tcb->state = tcp_state_closed;
	tcb->callback = callback;
	break;
    }
    return socket_num;
}


tcp_socket_t tcp_get_socket_num(struct tcp_tcb * tcb)
{
    tcp_socket_t socket_num = (tcp_socket_t)(((uint16_t)tcb - (uint16_t)tcp_tcbs)/sizeof(struct tcp_tcb));
    if(tcp_socket_valid(socket_num))
      return socket_num;
    return -1;
}

uint8_t tcp_socket_valid(tcp_socket_t socket)
{
    return (socket >=0 && socket < TCP_MAX_SOCKETS);
}

uint8_t tcp_listen(tcp_socket_t socket,uint16_t port)
{
    if(!tcp_socket_valid(socket))
      return 0;
    if(!tcp_free_port(port))
      return 0;
    struct tcp_tcb * tcb = &tcp_tcbs[socket];
    if(tcb->state != tcp_state_closed)
      return 0;
    tcb->port_local = port;
    tcb->state = tcp_state_listen;
    return 1;
}

uint8_t tcp_connect(tcp_socket_t socket,ip_address * ip,uint16_t port)
{
  
}

uint8_t tcp_disconnect(tcp_socket_t socket)
{
  
}

uint8_t tcp_handle_packet(const ip_address * ip_remote,const struct tcp_header * tcp,uint16_t length)
{
    DEBUG_PRINT_COLOR(B_IGREEN,"TCP handle packet\n");
    if(length < sizeof(struct tcp_header))
      return 0;
    //DEBUG_PRINT_COLOR(IGREEN,"TCP: length ok\n");
    if(ntoh16(tcp->checksum) != tcp_get_checksum(ip_remote,tcp,length))
      return 0;
    //DEBUG_PRINT_COLOR(IGREEN,"TCP: checksum ok\n");
    //DEBUG_PRINT_COLOR(IGREEN,"Received TCP packet, remote port 0x%x\n",ntoh16(tcp->port_source));
    struct tcp_tcb * tcb;
    struct tcp_tcb * tcb_selected = 0;
    FOREACH_TCB(tcb)
    {
	if(tcb->state == tcp_state_unused)
	  continue;
	if(tcb->state == tcp_state_closed)
	  continue;
	if(tcb->state == tcp_state_listen)
	{
	    tcb_selected = tcb;
	    continue;
	}
	if(tcb->port_local != ntoh16(tcp->port_destination))
	  continue;
	if(tcb->port_remote != ntoh16(tcp->port_source))
	  continue;
	if(memcmp(&tcb->ip_remote,ip_remote,sizeof(ip_address)))
	  continue;
	
	tcb_selected = tcb;
	break;
    }
    if(tcb_selected != 0)
      return tcp_state_machine(tcb_selected,ip_remote,tcp,length); 
    tcp_send_rst(ip_remote,tcp,length);
    return 0;
}



uint8_t tcp_send_rst(const ip_address * ip_remote,const struct tcp_header * tcp_rcv,uint16_t length)
{
  /* Behaviour due to RFC-793 [Page 36] point 1.*/
  if(!tcp_rcv)
    return 0;
  if(tcp_rcv->flags & TCP_FLAG_RST)
    return 0;
  
  struct tcp_header * tcp_rst = (struct tcp_header*)ip_get_buffer();
  memset(tcp_rst,0,sizeof(struct tcp_header));
  tcp_rst->port_destination = tcp_rcv->port_source;
  tcp_rst->port_source = tcp_rcv->port_destination;
  /* the number of 32-bit owrds shifted by 4 positions due to reserved bits*/
  tcp_rst->offset = (sizeof(struct tcp_header)/4)<<4;
  
  if(tcp_rcv->flags & TCP_FLAG_ACK)
  {
    /*If the incoming segment has an ACK field, the reset takes its
      sequence number from the ACK field of the segment..*/
    tcp_rst->seq = tcp_rcv->ack;
    tcp_rst->flags = TCP_FLAG_RST;
  }
  else
  {
    /*..otherwise 
    the reset has sequence number zero and the ACK field is set to the sum
    of the sequence number and segment length of the incoming segment*/
						/* (tcp_rcv->offset>>4)*4 */
    uint32_t ack = ntoh32(tcp_rcv->seq) + length - (tcp_rcv->offset>>2);
//     if(tcp_rcv->flags & TCP_FLAG_SYN)
//       ack++;
    tcp_rst->flags = TCP_FLAG_RST | TCP_FLAG_ACK;
    tcp_rst->ack = hton32(ack);
  }
  DEBUG_PRINT_COLOR(IGREEN,"Sending RST\n");
  tcp_rst->checksum = hton16(tcp_get_checksum(ip_remote,tcp_rst,sizeof(struct tcp_header)));
  return ip_send_packet(ip_remote,IP_PROTOCOL_TCP,sizeof(struct tcp_header));
}
uint16_t tcp_get_checksum(const ip_address * ip_remote,const struct tcp_header * tcp,uint16_t length)
{
    /* tcp pseudo header :
             +--------+--------+--------+--------+
             |           Source Address          |
             +--------+--------+--------+--------+
             |         Destination Address       |
             +--------+--------+--------+--------+
             |  zero  |  PTCL  |    TCP Length   |
             +--------+--------+--------+--------+
    */
    /* PTCL + TCP length */
    uint16_t checksum = IP_PROTOCOL_TCP + length;
    /* source/destination ip address */
    checksum = net_get_checksum(checksum,(const uint8_t*)ip_remote,sizeof(ip_address),4);
    /* our ip address */
    checksum = net_get_checksum(checksum,(const uint8_t*)ip_get_addr(),sizeof(ip_address),4);
    
    /* TCP header + data */
    return ~net_get_checksum(checksum,(const uint8_t*)tcp,length,16);
}