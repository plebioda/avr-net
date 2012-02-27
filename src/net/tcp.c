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
*/
#define TCP_OPT_EOL		0
#define TCP_OPT_NOP		1
#define TCP_OPT_MSS		2
#define TCP_OPT_MSS_LENGTH 	4

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
    tcp_socket_callback callback;
    uint16_t port_local;
    uint16_t port_remote;
    ip_address ip_remote;
};

static struct tcp_tcb tcp_tcbs[TCP_MAX_SOCKETS] EXMEM;


uint16_t tcp_get_checksum(const ip_address * ip_remote,const struct tcp_header * tcp,uint16_t length);


uint8_t tcp_init(void)
{
    memset(tcp_tcbs,0,sizeof(tcp_tcbs));
    return 1;
}


uint8_t tcp_handle_packet(const ip_address * ip_remote,const struct tcp_header * tcp,uint16_t length)
{
    DEBUG_PRINT_COLOR(B_IBLUE,"TCP handle packet\n");
    if(length < sizeof(struct tcp_header))
      return 0;
    //DEBUG_PRINT_COLOR(IBLUE,"TCP: length ok\n");
    if(ntoh16(tcp->checksum) != tcp_get_checksum(ip_remote,tcp,length))
      return 0;
    //DEBUG_PRINT_COLOR(IBLUE,"TCP: checksum ok\n");
    
    
    return 1;
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