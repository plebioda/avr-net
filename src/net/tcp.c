/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "tcp.h"
#include "arp.h"

#include "../arch/exmem.h"
#include "../sys/timer.h"
#include "../util/fifo.h"

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
    tcp_state_closed,
    tcp_state_not_accepted,
    tcp_state_accepted
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
    uint32_t ack;
    uint32_t seq;
    uint16_t seq_next;
    uint16_t window;
    uint16_t mss;
    timer_t timer;
    uint8_t rtx;
    struct fifo * fifo_rx;
    struct fifo * fifo_tx;
};

static struct tcp_tcb tcp_tcbs[TCP_MAX_SOCKETS] EXMEM;

#define FOREACH_TCB(tcb) for(tcb = &tcp_tcbs[0] ; tcb < &tcp_tcbs[TCP_MAX_SOCKETS] ; tcb++)

static uint8_t tcp_send_packet(struct tcp_tcb * tcb,uint8_t flags,uint8_t send_data);
static uint8_t tcp_state_machine(struct tcp_tcb * tcb,const ip_address * ip_remote,const struct tcp_header * tcp,uint16_t length);
static uint8_t tcp_send_rst(const ip_address * ip_remote,const struct tcp_header * tcp,uint16_t length);
static uint8_t tcp_get_options(struct tcp_tcb * tcb,const struct tcp_header * tcp,uint16_t length);
static uint16_t tcp_get_checksum(const ip_address * ip_remote,const struct tcp_header * tcp,uint16_t length);
static tcp_socket_t tcp_get_socket_num(struct tcp_tcb * tcb);
static uint8_t tcp_socket_valid(tcp_socket_t socket);
static uint8_t tcp_tcb_valid(struct tcp_tcb * tcb);
static uint8_t tcp_free_port(uint16_t port);
static struct tcp_tcb * tcp_tcb_alloc(void);
static void tcp_tcb_free(struct tcp_tcb * tcb);
static void tcp_tcb_reset(struct tcp_tcb * tcb);
static void tcp_timeout(timer_t timer,void * arg);
static uint16_t tcp_port_find_unused(void);
static uint8_t tcp_tcb_alloc_fifo(struct tcp_tcb * tcb);
static void tcp_tcb_free_fifo(struct tcp_tcb * tcb);


uint8_t tcp_state_machine(struct tcp_tcb * tcb,const ip_address * ip_remote,const struct tcp_header * tcp,uint16_t length)
{
  if(!tcb || !ip_remote || !tcp || length < sizeof(struct tcp_header))
    return 0;
  tcp_get_options(tcb,tcp,length);
  tcp_socket_t socket = tcp_get_socket_num(tcb);
  if(socket < 0)
    return 0;
  switch(tcb->state)
  {
    case tcp_state_closed:
      if(tcp->flags & TCP_FLAG_RST)
	return 0;
      tcp_send_rst(ip_remote,tcp,length);
      return 1;
    case tcp_state_listen:
      if(tcp->flags & TCP_FLAG_RST)
	return 0;
      if(tcp->flags & TCP_FLAG_ACK)
	return tcp_send_rst(ip_remote,tcp,length);
      /* there is only one valid situation when SYN is set */
      if(!(tcp->flags & TCP_FLAG_SYN))
	return tcp_send_rst(ip_remote,tcp,length);
      /* get TCB for new connection */
      struct tcp_tcb * new_tcb = tcp_tcb_alloc();
      tcp_tcb_alloc_fifo(new_tcb);
      if(!new_tcb || !new_tcb->fifo_rx || !new_tcb->fifo_tx)
      {
	  /* if there is no free tcb then maybe remote host will send 
	   another SYN and then we will have any free slot, so we do not send RST*/
	  tcp_tcb_free(new_tcb);
	  return 0;
      }
      /* reset tcb for new connection */
      tcp_tcb_reset(new_tcb);
      /* set local port */
      new_tcb->port_local = tcb->port_local;
      /* set callback */
      new_tcb->callback = tcb->callback;
      /* set remote ip address */
      memcpy(new_tcb->ip_remote,ip_remote,sizeof(ip_address));
      /* set remote port number */
      new_tcb->port_remote = ntoh16(tcp->port_source);
      /* set state to not acepted */
      new_tcb->state = tcp_state_not_accepted;
      /* set mss */
      new_tcb->mss = tcb->mss;
      socket = tcp_get_socket_num(new_tcb);
      /* Send information to user about incoming new connection */
      new_tcb->callback(socket,tcp_event_connection_incoming);
      /* Chec if user accepted the connection */
      if(new_tcb->state != tcp_state_accepted)
      {
	    /* If not accepted then clean-up and refuse connection by sending RST */
	    tcp_tcb_free(new_tcb);
	    return tcp_send_rst(ip_remote,tcp,length);
      }
      /* User accepted the connection so we have to establish a connection */
      /* set ack */
      new_tcb->ack = ntoh32(tcp->seq) + 1;
      /* send SYN, ACK segment */
      if(!tcp_send_packet(new_tcb,TCP_FLAG_SYN|TCP_FLAG_ACK,0))
      {
	  /* if any error occured during sending packet 
	  change state to closed, send error event to user 
	  and free tcb of this connection */
	  new_tcb->state = tcp_state_closed;
	  new_tcb->callback(socket,tcp_event_error);
	  tcp_tcb_free(new_tcb);
	  return 0;
      }
      /* if packet sent then set the timer and wait for reply */
      timer_set(new_tcb->timer,TCP_TIMEOUT_GENERIC);
      /* change state to SYN_RECEIVED */
      new_tcb->state = tcp_state_syn_received;
      /* set number off allowed retransmissions */
      new_tcb->rtx = TCP_RTX_SYN_ACK;
      return 1;
    case tcp_state_syn_sent:
      /* stop the timer because we received packet in state SYN-SENT */
      timer_stop(tcb->timer);
      /* check for ACK flag */
      if(tcp->flags & TCP_FLAG_ACK)
      {
	/* if ACK set, check if this is answer for our SYN packet */
	if(ntoh32(tcp->ack) != tcb->seq + 1 )
	{
	  /* if ack is invalid and RST flag is not set, send RST packet */
	  if(!(tcp->flags&TCP_FLAG_RST))
	    return tcp_send_rst(ip_remote,tcp,length);
	}
	if(tcp->flags & TCP_FLAG_RST)
	{
	  /*if ACK is set and RST is set there was a problem with aour SYN packet
	  so send event to user */
	  tcb->state = tcp_state_closed;
	  tcb->callback(socket,tcp_event_error);
	  return 0;
	}
	/* increase sequnce number */
	tcb->seq++;
      }
      /* if no valid no ACK or valid ACK chec for RST*/
      if(tcp->flags & TCP_FLAG_RST)
	return 0;
      /* This step should be reached only if the ACK is ok, or there is
        no ACK, and it the segment did not contain a RST */
      /*check for SYN flag */
      if(tcp->flags & TCP_FLAG_SYN)
      {
	/* set ack number */
	tcb->ack = ntoh32(tcp->seq) + 1;
	/* if ACK is set it means this is answer for our SYN packet */
	if(tcp->flags & TCP_FLAG_ACK)
	{
	  /* send ACK of SYN-ACK */
	  if(!tcp_send_packet(tcb,TCP_FLAG_ACK,0))
	  {
	    tcb->state = tcp_state_closed;
	    tcb->callback(socket,tcp_event_error);
	    return 0;
	  }
	  /* change state to established */
	  tcb->state = tcp_state_established;
	  /* send event to user */
	  tcb->callback(socket,tcp_event_connection_established);
	  /* set timer to IDLE timeout */
	  timer_set(tcb->timer,TCP_TIMEOUT_IDLE);
	  return 1;
	}
	/* ACK is not set so it means that there was simultaneous connecton request
	send SYN-ACK*/
	if(!tcp_send_packet(tcb,TCP_FLAG_SYN|TCP_FLAG_ACK,0))
	{
	  tcb->state = tcp_state_closed;
	  tcb->callback(socket,tcp_event_error);
	  return 0;	  
	}
	/* change state to SYN-RECEIVED*/
	tcb->state = tcp_state_syn_received;
	return 1;
      }
      return 0;
    default:
      break;
  }
  if(ntoh32(tcp->seq) != tcb->ack)
  {
      return tcp_send_packet(tcb,TCP_FLAG_ACK,0);
  }
  switch(tcb->state)
  {
    case tcp_state_syn_received:
    case tcp_state_established:
    case tcp_state_fin_wait_1:
    case tcp_state_fin_wait_2:
    case tcp_state_close_wait:
    case tcp_state_closing:
    case tcp_state_last_ack:
    case tcp_state_time_wait:
      if(tcp->flags & (TCP_FLAG_RST|TCP_FLAG_SYN))
      {
	tcb->state = tcp_state_closed;
	tcb->callback(socket,tcp_event_reset);
	timer_stop(tcb->timer);
	return 0;
      }
      break;
    default:
      break;
  }
  /* all following situations requires ACK flag on */
  if(!(tcp->flags & TCP_FLAG_ACK))
    return 0;
  uint32_t rcv_ack = ntoh32(tcp->ack);
  switch(tcb->state)
  {
    case tcp_state_syn_received:
    {

      if(rcv_ack != tcb->seq+1)
	return 0;
      tcb->seq++;
      tcb->state = tcp_state_established;
      tcb->callback(socket,tcp_event_connection_established);
      timer_set(tcb->timer,TCP_TIMEOUT_IDLE);
      break;
    }
    case tcp_state_established:
    case tcp_state_fin_wait_1:
    case tcp_state_fin_wait_2:
    case tcp_state_close_wait:
    case tcp_state_closing:
    {
      /* nothing acked */
      if(rcv_ack == tcb->seq)
	break;
      uint32_t seq_next = tcb->seq + (uint32_t)tcb->seq_next;
      if((rcv_ack > tcb->seq) && (rcv_ack <= seq_next))
      {
	uint16_t acked_bytes = (uint16_t)(rcv_ack - tcb->seq);
	
      }
      
    }
    default: 
      break; 
  }
  switch(tcb->state)
  {
    case tcp_state_established:
    case tcp_state_fin_wait_1:
    case tcp_state_fin_wait_2:
    {
      uint8_t data_offset = (tcp->offset>>4)<<2;
      uint16_t data_length = length - data_offset;
      if(data_length > 0)
      {
	uint16_t buffered_data = fifo_enqueue(tcb->fifo_rx,(const uint8_t*)tcp + data_offset,data_length);
	tcb->ack += buffered_data;
	tcp_send_packet(tcb,TCP_FLAG_ACK,0);
	DEBUG_PRINT_COLOR(IGREEN,"rcv %d bytes\n",buffered_data);
	tcb->callback(socket,tcp_event_data_received);
      }
      break;
    }
    default:
      break;
  }
  
  return 0;
}

void tcp_timeout(timer_t timer,void * arg)
{
    if(!arg)
      return;
    struct tcp_tcb * tcb = (struct tcp_tcb*)arg;
    tcp_socket_t socket = tcp_get_socket_num(tcb);
    if(socket < 0)
      return;
    if(timer != tcb->timer)
      return;
    int32_t tset = -1;
    if(--tcb->rtx <= 0)
    {
      /* if we exceeded maximum number of retransmissions 
      change state to closed and send event to user*/
      tcb->state = tcp_state_closed;
      tcb->callback(socket,tcp_event_timeout);
      return;
    }
    switch(tcb->state)
    {
      /* TCB is in this state when user called connect */
      case tcp_state_closed:
	/* retransmit SYN packet */
	if(!tcp_send_packet(tcb,TCP_FLAG_SYN,0))
	{
	  /* if tcp_send_packet returned zero there is 
	    a possibility that it was becouse there was not remote host 
	    MAC address, so we remain in closed state and set timer to ARP
	    MAC request timeout*/
	  tcb->state = tcp_state_closed;
	  tset = TCP_TIMEOUT_ARP_MAC;
	}
	else
	{
	  /* SYN packet sent so set timer and change state to SYN-SENT*/
	  tcb->state = tcp_state_syn_sent;
	  tset = TCP_TIMEOUT_GENERIC;
	}
	break;
      case tcp_state_syn_sent:
	if(!tcp_send_packet(tcb,TCP_FLAG_SYN,0))
	{
	  /* error while sending packet */
	  tcb->state = tcp_state_closed;
	  tcb->callback(socket,tcp_event_error);
	  break;
	}
	/* set timer and wait for SYN-ACK packet */
	tset = TCP_TIMEOUT_GENERIC;
	break;
      case tcp_state_syn_received:
	if(!tcp_send_packet(tcb,TCP_FLAG_SYN|TCP_FLAG_ACK,0))
	{
	  tcb->state = tcp_state_closed;
	  tcb->callback(socket,tcp_event_error);	  
	  break;
	}
	tset = TCP_TIMEOUT_GENERIC;
	break;
      case tcp_state_established:
	/* IDLE tiemout */
	DEBUG_PRINT_COLOR(IGREEN,"established\n");
	break;
      default:
	break;
    }
    timer_set(tcb->timer,tset);
}

struct tcp_tcb * tcp_tcb_alloc(void)
{
    struct tcp_tcb * tcb;
    FOREACH_TCB(tcb)
    {
	if(tcb->state == tcp_state_unused)
	{
	    tcb->state = tcp_state_closed;
	    tcb->timer = timer_alloc(tcp_timeout);
	    timer_set_arg(tcb->timer,(void*)tcb);
	    return tcb;
	}
    }
    return 0;
}
void tcp_tcb_free(struct tcp_tcb * tcb)
{
    timer_free(tcb->timer);
    tcp_tcb_free_fifo(tcb);
    memset(tcb,0,sizeof(struct tcp_tcb));
}
void tcp_tcb_reset(struct tcp_tcb * tcb)
{
  if(!tcp_tcb_valid(tcb))
    return;
  /* save callback */
  tcp_socket_callback callback = tcb->callback;
  /* save timer */
  timer_t timer = tcb->timer;
  /* save local port */
  uint16_t port = tcb->port_local;
  /* save rx and tx fifo */
  struct fifo * fifo_tx = tcb->fifo_tx;
  struct fifo * fifo_rx = tcb->fifo_rx;
  memset(tcb,0,sizeof(struct tcp_tcb));
  tcb->callback = callback;
  tcb->timer = timer;
  tcb->state = tcp_state_closed;
  tcb->port_local = port;
  tcb->fifo_rx = fifo_rx;
  tcb->fifo_tx = fifo_tx;
}

uint8_t tcp_send_packet(struct tcp_tcb * tcb,uint8_t flags,uint8_t send_empty)
{
  if(!tcb)
    return 0;
  struct tcp_header * tcp = (struct tcp_header*)ip_get_buffer();
  memset(tcp,0,sizeof(struct tcp_header));
  /* set destination port */
  tcp->port_destination = hton16(tcb->port_remote);
  /* set source port */
  tcp->port_source = hton16(tcb->port_local);
  /* not using urgent */
  tcp->urgent = HTON16(0x0000);
  /* set acknowledgment number */
  tcp->ack = hton32(tcb->ack);
  /* set sequence number */
  tcp->seq = hton32(tcb->seq);
  /* set flags */
  tcp->flags = flags;
  /* set window to buffer free space length */
  tcp->window = hton16(fifo_space(tcb->fifo_rx));
  DEBUG_PRINT("window = %d\n",fifo_space(tcb->fifo_rx));
  uint16_t packet_header_len = sizeof(struct tcp_header);
  uint16_t packet_data_len = 0;
  uint16_t packet_total_len =0;
  uint16_t max_packet_szie = tcp_get_buffer_size();
  uint8_t * data_ptr = (uint8_t*)tcp + sizeof(struct tcp_header);
  /* if SYN packet send maximum segment size in options field */
  if(tcp->flags & TCP_FLAG_SYN)
  {
      *((uint32_t*)data_ptr) = HTON32(((uint32_t)TCP_OPT_MSS<<24)|((uint32_t)TCP_OPT_LENGTH_MSS<<16)|(uint32_t)TCP_MSS);
      data_ptr += sizeof(uint32_t);
      packet_header_len += sizeof(uint32_t);
      max_packet_szie -= sizeof(uint32_t);
  }
  tcp->offset = (packet_header_len>>2)<<4;
  packet_data_len += fifo_dequeue(tcb->fifo_tx,data_ptr,max_packet_szie);
  packet_total_len = packet_header_len+packet_data_len;
  tcp->checksum = hton16(tcp_get_checksum((const ip_address*)&tcb->ip_remote,tcp,packet_total_len));
  return ip_send_packet((const ip_address*)&tcb->ip_remote,IP_PROTOCOL_TCP,packet_total_len);
}

uint8_t tcp_get_options(struct tcp_tcb * tcb,const struct tcp_header * tcp,uint16_t length)
{
    if(!tcb || !tcp || length < sizeof(struct tcp_header))
      return 0;
    uint16_t offset = (tcp->offset>>4)<<2;
    uint8_t * options = (uint8_t*)tcp + sizeof(struct tcp_header);
    uint8_t * options_end = (uint8_t*)tcp + offset;
    for(;options < options_end;options++)
    {
	if(*options == TCP_OPT_EOL)
	{
	    /* end of options list*/
	    break;
	}
	else if (*options == TCP_OPT_NOP)
	{
	    /* no operation */
	}
	else if(*options == TCP_OPT_MSS && *(options+1) == TCP_OPT_LENGTH_MSS)
	{
	  /* maximum segment size */
	  options+=2;
	  tcb->mss = ntoh16(*((uint16_t*)options));
	  options++;
	}
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
    timer_free(tcb->timer);
    memset(tcb,0,sizeof(struct tcp_tcb));   
    return 1;
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
	tcb->timer = timer_alloc(tcp_timeout);
	if(tcb->timer < 0)
	  return -1;
	timer_set_arg(tcb->timer,(void*)tcb);
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
uint8_t tcp_accept(tcp_socket_t socket)
{
    if(!tcp_socket_valid(socket))
      return 0;
    struct tcp_tcb * tcb = &tcp_tcbs[socket];
    if(tcb->state != tcp_state_not_accepted)
      return 0;
    tcb->state = tcp_state_accepted;
    return 1;
}
uint8_t tcp_connect(tcp_socket_t socket,ip_address * ip,uint16_t port)
{
  /* check saocket number */
  if(!tcp_socket_valid(socket))
    return 0;
  struct tcp_tcb * tcb = &tcp_tcbs[socket];
  /* check if state is closed */
  if(tcb->state != tcp_state_closed)
    return 0;
  /* get any unused local port */
  tcb->port_local = tcp_port_find_unused();
  /* if returned zero it means there are no free ports (in practice it's impossible)*/
  if(!tcb->port_local)
    return 0;
  /* reset current TCB context */
  tcp_tcb_reset(tcb);
  /* set remote ip address */
  memcpy(&tcb->ip_remote,ip,sizeof(ip_address));
  /* set remote port number */
  tcb->port_remote = port;
  /* set number of allowed retransmissions */
  tcb->rtx = TCP_RTX_SYN;
  /* set timer to 1 so tcp_timeout will be called in a moment 
    the rest part of processing connect user's call is in tcp_timeout function*/
  timer_set(tcb->timer,1);
  return 1;
}

uint8_t tcp_disconnect(tcp_socket_t socket)
{
  return 1;
}

uint8_t tcp_handle_packet(const ip_address * ip_remote,const struct tcp_header * tcp,uint16_t length)
{
    if(length < sizeof(struct tcp_header))
      return 0;
    if(ntoh16(tcp->checksum) != tcp_get_checksum(ip_remote,tcp,length))
      return 0;
    struct tcp_tcb * tcb;
    struct tcp_tcb * tcb_selected = 0;
    FOREACH_TCB(tcb)
    {
	if(tcb->state == tcp_state_unused)
	  continue;
	if(tcb->port_local != ntoh16(tcp->port_destination))
	  continue;
	if(tcb->state == tcp_state_listen || tcb->state == tcp_state_closed)
	{
	    tcb_selected = tcb;
	    continue;
	}
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
//   DEBUG_PRINT_COLOR(IGREEN,"send rst\n");
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
    uint32_t ack = ntoh32(tcp_rcv->seq) + length - ((tcp_rcv->offset>>4)<<2);
//     if(tcp_rcv->flags & TCP_FLAG_SYN)
//       ack++;
    tcp_rst->flags = TCP_FLAG_RST | TCP_FLAG_ACK;
    tcp_rst->ack = hton32(ack);
  }
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

uint16_t tcp_get_remote_port(tcp_socket_t socket)
{
  if(!tcp_socket_valid(socket))
    return 0;
  return tcp_tcbs[socket].port_remote;
}
const ip_address * tcp_get_remote_ip(tcp_socket_t socket)
{
  if(!tcp_socket_valid(socket))
      return 0;
  return (const ip_address*)&tcp_tcbs[socket].ip_remote;
}
uint16_t tcp_port_find_unused(void)
{
    uint16_t port = 1023;
    do
    {
	port++;
    }while(!tcp_free_port(port) && port!=0);
    return port;
}

uint8_t tcp_tcb_alloc_fifo(struct tcp_tcb * tcb)
{
    if(!tcp_tcb_valid(tcb))
      return 0;
    tcb->fifo_rx = fifo_alloc();
    tcb->fifo_tx = fifo_alloc();
    if(tcb->fifo_rx == 0 || tcb->fifo_tx == 0)
    {
	fifo_free(tcb->fifo_rx);
	fifo_free(tcb->fifo_tx);
	return 0;
    }
    return 1;
}
uint8_t tcp_tcb_valid(struct tcp_tcb * tcb)
{
    return (tcb >= &tcp_tcbs[0] && tcb < &tcp_tcbs[TCP_MAX_SOCKETS]);
}

void tcp_tcb_free_fifo(struct tcp_tcb * tcb)
{
    fifo_free(tcb->fifo_rx);
    fifo_free(tcb->fifo_tx);
    tcb->fifo_rx = 0;
    tcb->fifo_tx = 0;
}
int16_t tcp_read(tcp_socket_t socket,uint8_t * data,uint16_t maxlen)
{
    if(!tcp_socket_valid(socket))
      return -1;
    struct tcp_tcb * tcb = &tcp_tcbs[socket];
    return fifo_dequeue(tcb->fifo_rx,data,maxlen);
}

int16_t tcp_write(tcp_socket_t socket,uint8_t * data,uint16_t len)
{
  if(!tcp_socket_valid(socket))
    return -1;
  struct tcp_tcb * tcb = &tcp_tcbs[socket];
  int16_t ret = fifo_enqueue(tcb->fifo_tx,data,len);
  tcp_send_packet(tcb,TCP_FLAG_ACK,1);
  return ret;
}

int16_t tcp_write_P(tcp_socket_t socket,prog_uint8_t * data,uint16_t len)
{
  if(!tcp_socket_valid(socket))
    return -1;
  struct tcp_tcb * tcb = &tcp_tcbs[socket];
  int16_t ret = fifo_enqueue_P(tcb->fifo_tx,data,len);
  tcp_send_packet(tcb,TCP_FLAG_ACK,1);
  return ret;
}