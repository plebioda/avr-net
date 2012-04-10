/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "dhcp.h"

#include "../net/ip.h"
#include "../net/udp.h"
#include "../sys/timer.h"

#include <string.h>

#define DEBUG_MODE
#include "../debug.h"

/*
   DHCP Header 
    
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     op (1)    |   htype (1)   |   hlen (1)    |   hops (1)    |
   +---------------+---------------+---------------+---------------+
   |                            xid (4)                            |
   +-------------------------------+-------------------------------+
   |           secs (2)            |           flags (2)           |
   +-------------------------------+-------------------------------+
   |                          ciaddr  (4)                          |
   +---------------------------------------------------------------+
   |                          yiaddr  (4)                          |
   +---------------------------------------------------------------+
   |                          siaddr  (4)                          |
   +---------------------------------------------------------------+
   |                          giaddr  (4)                          |
   +---------------------------------------------------------------+
   |                          chaddr  (16)                         |
   +---------------------------------------------------------------+
   |                          sname   (64)                         |
   +---------------------------------------------------------------+
   |                          file    (128)                        |
   +---------------------------------------------------------------+
   |                          options (variable)                   |
   +---------------------------------------------------------------+
   
      FIELD      OCTETS       DESCRIPTION
   -----      ------       -----------

   op            1  Message op code / message type.
                    1 = BOOTREQUEST, 2 = BOOTREPLY
   htype         1  Hardware address type, see ARP section in "Assigned
                    Numbers" RFC; e.g., '1' = 10mb ethernet.
   hlen          1  Hardware address length (e.g.  '6' for 10mb
                    ethernet).
   hops          1  Client sets to zero, optionally used by relay agents
                    when booting via a relay agent.
   xid           4  Transaction ID, a random number chosen by the
                    client, used by the client and server to associate
                    messages and responses between a client and a
                    server.
   secs          2  Filled in by client, seconds elapsed since client
                    began address acquisition or renewal process.
   flags         2  Flags (see figure 2).
   ciaddr        4  Client IP address; only filled in if client is in
                    BOUND, RENEW or REBINDING state and can respond
                    to ARP requests.
   yiaddr        4  'your' (client) IP address.
   siaddr        4  IP address of next server to use in bootstrap;
                    returned in DHCPOFFER, DHCPACK by server.
   giaddr        4  Relay agent IP address, used in booting via a
                    relay agent.
   chaddr       16  Client hardware address.
   sname        64  Optional server host name, null terminated string.
   file        128  Boot file name, null terminated string; "generic"
                    name or null in DHCPDISCOVER, fully qualified
                    directory-path name in DHCPOFFER.
   options     var  Optional parameters field.  See the options
                    documents for a list of defined options.
*/

/* RFC 1533 */
#define DHCP_OPTION_PAD				0x00
#define DHCP_OPTION_END				0xff
#define DHCP_OPTION_SUBNET_MASK			1
#define DHCP_OPTION_TIME_OFFSET			2
#define DHCP_OPTION_ROUTER			3
#define DHCP_OPTION_SERVER_TIME			4
#define DHCP_OPTION_SERVER_NAME			5
#define DHCP_OPTION_SERVER_DOMAIN_NAME		6
#define DHCP_OPTION_SERVER_LOG			7
#define DHCP_OPTION_SERVER_COOKIE		8
#define DHCP_OPTION_SERVER_LPR			9
#define DHCP_OPTION_SERVER_IMPRESS		10
#define DHCP_OPTION_SERVER_RES_LOC		11
#define DHCP_OPTION_HOST_NAME			12
#define DHCP_OPTION_BOOT_FILE_SIZE		13
#define DHCP_OPTION_MERIT_DUMP_FILE		14
#define DHCP_OPTION_DOMAIN_NAME			15
#define DHCP_OPTION_SWAP_SERVER			16
#define DHCP_OPTION_ROOT_PATH			17
#define DHCP_OPTION_EXT_PATH			18
#define DHCP_OPTION_IP_FORWARDING		19
#define DHCP_OPTION_NON_LOCAL_SRC_ROUT		20
#define DHCP_OPTION_POLICY_FILTER		21
#define DHCP_OPTION_MAX_DGRAM_SIZE		22
#define DHCP_OPTION_DEFAULT_IP_TTL		23
#define DHCP_OPTION_PATH_MTU_AGING_TIMEOUT	24
#define DHCP_OPTION_PATH_MTU_PLATEAU_TABLE	25
#define DHCP_OPTION_IFACE_MTU_OPTION		26
#define DHCP_OPTION_ALL_SUBNETS_ARE_LOCAL	27
#define DHCP_OPTION_BROADCAST_ADDRESS		28
#define DHCP_OPTION_PERFORM_MASK_DISCOVERY	29
#define DHCP_OPTION_MASK_SUPPLIER		30
#define DHCP_OPTION_PERFORM_ROUTER_DISCOVERY	31
#define DHCP_OPTION_ROUTER_SOLICITATION_ADDR	32
#define DHCP_OPTION_STATIC_ROUTE		33 
#define DHCP_OPTION_TRAILER_ENCAPSULATION 	34
#define DHCP_OPTION_ARP_CACHE_TIMEOUT		35
#define DHCP_OPTION_ETH_ENCAPSULATION		36
#define DHCP_OPTION_TCP_DEFAULT_TTL		37
#define DHCP_OPTION_TCP_KEEPALIVE_INTERVAL	38
#define DHCP_OPTION_TCP_KEEPALIVE_GARBAGE	39
#define DHCP_OPTION_NET_INFO_SERVICE_DOMAIN	40
#define DHCP_OPTION_NET_INFO_SERVERS		41
#define DHCP_OPTION_NET_TIME_PROTOCOL_SERVERS	42
#define DHCP_OPTION_VENDOR_SPECIFIC_INFO	43
#define DHCP_OPTION_NBNS			44
#define DHCP_OPTION_NBDD			45
#define DHCP_OPTION_NB_TCPIP_NODE_TYPE		46
#define DHCP_OPTION_NB_TCPIP_SCOPE		47
#define DHCP_OPTION_XWINDOW_SYS_FONT_SERVER	48
#define DHCP_OPTION_XWINDOW_SYS_DISPLAY_MANAGER	49
#define DHCP_OPTION_REQUESTED_IP_ADDR		50
#define DHCP_OPTION_IP_ADDRESS_LEASE_TIME	51
#define DHCP_OPTION_OVERLOAD			52
#define DHCP_OPTION_MESSAGE_TYPE		53
#define DHCP_OPTION_SERVER_ID			54
#define DHCP_OPTION_PARAM_REQUEST_LIST		55
#define DHCP_OPTION_MESSAGE 			56
#define DHCP_OPTION_MAX_MESSAGE_SIZE		57
#define DHCP_OPTION_RENEWAL_TIME_VALUE		58
#define DHCP_OPTION_REBINDING_TIME_VALUE	59
#define DHCP_OPTION_CLASS_ID			60
#define DHCP_OPTION_CLIENT_ID			61


#define DHCP_OPTION_LEN_MESSAGE_TYPE		1
#define DHCP_OPTION_LEN_SUBNET_MASK		4
#define DHCP_OPTION_LEN_SERVER_ID		4
#define DHCP_OPTION_LEN_IP_ADDRESS_LEASE_TIME	4
#define DHCP_OPTION_LEN_REQUESTED_IP_ADDR	4
/*
  DHCP Message Type
  
      Code   Len  Type
   +-----+-----+-----+
   |  53 |  1  | 1-7 |
   +-----+-----+-----+
   
           Value   Message Type
           -----   ------------
             1     DHCPDISCOVER
             2     DHCPOFFER
             3     DHCPREQUEST
             4     DHCPDECLINE
             5     DHCPACK
             6     DHCPNAK
             7     DHCPRELEASE
*/

/* Client broadcast to locate available servers. */
#define DHCPDISCOVER 	1
/* Server to client in response to DHCPDISCOVER with
  offer of configuration parameters. */
#define DHCPOFFER	2
/* Client message to servers either (a) requesting
offered parameters from one server and implicitly
declining offers from all others, (b) confirming
correctness of previously allocated address after,
e.g., system reboot, or (c) extending the lease on a
particular network address.*/
#define DHCPREQUEST	3
/* Client to server indicating network address is already in use. */
#define DHCPDECLINE	4
/* Server to client with configuration parameters,
including committed network address. */
#define DHCPACK 	5
/* Server to client indicating client's notion of network
address is incorrect (e.g., client has moved to new
subnet) or client's lease as expired */
#define DHCPNAK		6
/* Client to server relinquishing network address and
cancelling remaining lease. */
#define DHCPRELEASE	7
/* Client to server, asking only for local configuration
parameters; client already has externally configured
network address.*/
#define DHCPINFORM	8


/*
  DHCP Client Transition Diagram
  
   --------                               -------
|        | +-------------------------->|       |<-------------------+
| INIT-  | |     +-------------------->| INIT  |                    |
| REBOOT |DHCPNAK/         +---------->|       |<---+               |
|        |Restart|         |            -------     |               |
 --------  |  DHCPNAK/     |               |                        |
    |      Discard offer   |      -/Send DHCPDISCOVER               |
-/Send DHCPREQUEST         |               |                        |
    |      |     |      DHCPACK            v        |               |
 -----------     |   (not accept.)/   -----------   |               |
|           |    |  Send DHCPDECLINE |           |                  |
| REBOOTING |    |         |         | SELECTING |<----+            |
|           |    |        /          |           |     |DHCPOFFER/  |
 -----------     |       /            -----------   |  |Collect     |
    |            |      /                  |   |       |  replies   |
DHCPACK/         |     /  +----------------+   +-------+            |
Record lease, set|    |   v   Select offer/                         |
timers T1, T2   ------------  send DHCPREQUEST      |               |
    |   +----->|            |             DHCPNAK, Lease expired/   |
    |   |      | REQUESTING |                  Halt network         |
    DHCPOFFER/ |            |                       |               |
    Discard     ------------                        |               |
    |   |        |        |                   -----------           |
    |   +--------+     DHCPACK/              |           |          |
    |              Record lease, set    -----| REBINDING |          |
    |                timers T1, T2     /     |           |          |
    |                     |        DHCPACK/   -----------           |
    |                     v     Record lease, set   ^               |
    +----------------> -------      /timers T1,T2   |               |
               +----->|       |<---+                |               |
               |      | BOUND |<---+                |               |
  DHCPOFFER, DHCPACK, |       |    |            T2 expires/   DHCPNAK/
   DHCPNAK/Discard     -------     |             Broadcast  Halt network
               |       | |         |            DHCPREQUEST         |
               +-------+ |        DHCPACK/          |               |
                    T1 expires/   Record lease, set |               |
                 Send DHCPREQUEST timers T1, T2     |               |
                 to leasing server |                |               |
                         |   ----------             |               |
                         |  |          |------------+               |
                         +->| RENEWING |                            |
                            |          |----------------------------+
                             ----------
*/

#define DHCP_OP_BOOTREQUEST	1
#define DHCP_OP_BOOTREPLY	2
#define DHCP_HTYPE_ETH		0x01
#define DHCP_HLEN_ETH		6
#define DHCP_FLAG_BROADCAST	0x8000
#define DHCP_COOKIE		0x63825363
#define DHCP_XID		0x50414b4f

struct dhcp_header
{
  uint8_t op;
  uint8_t htype;
  uint8_t hlen;
  uint8_t hops;
  uint32_t xid;
  uint16_t secs;
  uint16_t flags;
  ip_address ciaddr;
  ip_address yiaddr;
  ip_address siaddr;
  ip_address giaddr;
  uint8_t charrd[16];
  char sname[64];
  char file[128];
  uint32_t cookie;	//99, 130, 83 and 99
};

enum dhcp_state
{
  dhcp_state_init=0,
  dhcp_state_selecting,
  dhcp_state_requesting,
  dhcp_state_bound,
  dhcp_state_rebinding,
  dhcp_state_renewing,
  dhcp_state_rebooting,
  dhcp_state_init_reboot
};


static struct 
{
  enum dhcp_state state;
  dhcp_callback callback;
  udp_socket_t socket;
  timer_t timer;
  ip_address addr;
  ip_address server;
  ip_address gateway;
  ip_address netmask;
  uint8_t rtx;
  uint32_t time_rebind;
  
} dhcp_client;


static void dhcp_udp_callback(udp_socket_t socket,uint8_t * data,uint16_t len);
static void dhcp_timer_callback(timer_t timer,void * arg);
static void dhcp_free(void);
static uint8_t dhcp_send(void);

static uint8_t * dhcp_add_msg_type(uint8_t * ptr,uint8_t msgtype);
static uint8_t * dhcp_add_end(uint8_t * ptr);
static uint8_t * dhcp_add_params(uint8_t * ptr);
static uint8_t * dhcp_add_request_ip(uint8_t * ptr);
static uint8_t * dhcp_add_server_id(uint8_t * ptr);
static uint8_t * dhcp_add_host_name(uint8_t * ptr);
static uint8_t dhcp_parse_options(const struct dhcp_header * dhcp_header);

uint8_t dhcp_start(dhcp_callback callback)
{
  if(dhcp_client.state != dhcp_state_init || !callback)
    return DHCP_ERR_STATE;
  memset(&dhcp_client,0,sizeof(dhcp_client));
  /* allocate udp socket */
  dhcp_client.socket = udp_socket_alloc(UDP_PORT_ANY,dhcp_udp_callback);
  if(dhcp_client.socket < 0)
    return DHCP_ERR_SOCKET_ALLOC;
  /* allocate timer */
  dhcp_client.timer = timer_alloc(dhcp_timer_callback);
  if(dhcp_client.timer<0)
  {
    udp_socket_free(dhcp_client.socket);
    return DHCP_ERR_TIMER_ALLOC;
  }
  
  /* reset ip configuration */
  ip_init((const ip_address*)&dhcp_client.addr,
	  (const ip_address*)&dhcp_client.netmask,
	  (const ip_address*)&dhcp_client.gateway);	  
  dhcp_client.state = dhcp_state_init;
  dhcp_client.callback = callback;
  /* start on the next timeout */
  timer_set(dhcp_client.timer,1);
  return 0;
}


static void dhcp_udp_callback(udp_socket_t socket,uint8_t * data,uint16_t len)
{
  DEBUG_PRINT_COLOR(B_IGREEN,"dhcp udp packet len = %d\n",len);
  struct dhcp_header * dhcp_header = (struct dhcp_header*)data;
  
  /* accept only BOOTREPLY */
  if(dhcp_header->op != DHCP_OP_BOOTREPLY)
    return;
  DEBUG_PRINT_COLOR(B_IGREEN,"BOOTREPLY OK\n");
  /* check our transaction id*/
  if(dhcp_header->xid != HTON32(DHCP_XID))
    return;
  DEBUG_PRINT_COLOR(B_IGREEN,"XID OK\n");
  /* check our ethernet address */
  if(memcmp(&dhcp_header->charrd,ethernet_get_mac(),sizeof(ethernet_address)))
    return;
  DEBUG_PRINT_COLOR(B_IGREEN,"ETH ADDR OK\n");
  /* check COOKIE*/
  if(dhcp_header->cookie != HTON32(DHCP_COOKIE))
    return;
  DEBUG_PRINT_COLOR(B_IGREEN,"COOKIE OK\n");
  /* parse options */
  uint8_t msgtype = dhcp_parse_options((const struct dhcp_header *)dhcp_header);
  
  if(msgtype==0)
    return;
  
  switch(dhcp_client.state)
  {
    case dhcp_state_bound:
    case dhcp_state_init:
      break;
    case dhcp_state_selecting:
      /* ignore anything except DHCPOFFER */
      if(msgtype != DHCPOFFER)
	return;
      
      /* change state to REQUESTING */
      dhcp_client.state = dhcp_state_requesting;
      /* set timer to send dhcp response */
      timer_set(dhcp_client.timer,1);
      dhcp_client.rtx=0xff;
      break;
    case dhcp_state_requesting:
    case dhcp_state_rebinding:
      /* ignore offers from other servers */
      if(msgtype == DHCPOFFER)
	return;
      if(msgtype != DHCPACK)
      {
	if(dhcp_client.state == dhcp_state_requesting)
	  dhcp_client.callback(dhcp_event_lease_denied);
	else
	  dhcp_client.callback(dhcp_event_lease_expired);
	dhcp_free();
	return;
      }
      /* set ip configuration */
      ip_init((const ip_address*)&dhcp_client.addr,
	      (const ip_address*)&dhcp_client.netmask,
	      (const ip_address*)&dhcp_client.gateway);
      timer_set(dhcp_client.timer,(dhcp_client.time_rebind*1000));
      dhcp_client.state = dhcp_state_bound;
      dhcp_client.callback(dhcp_event_lease_acquired);
      break;
    default:
      DEBUG_PRINT_COLOR(B_IGREEN,"WRONG STATE!!!!\n");
      break;
  }
  
}


static void dhcp_timer_callback(timer_t timer,void * arg)
{
  if(timer != dhcp_client.timer)
  {
    dhcp_client.callback(dhcp_event_error);
    dhcp_free();
  }
  DEBUG_PRINT_COLOR(B_IGREEN,"dhcp_timer_callback state %d\n",dhcp_client.state);
  switch(dhcp_client.state)
  {
    case dhcp_state_init:
    {
      dhcp_client.state = dhcp_state_selecting;
      dhcp_client.rtx = 0xff;
    }
    case dhcp_state_selecting:
    {
      /* resend DHCPDISCOVER */
      if(++dhcp_client.rtx > DHCP_RTX)
      {
	/* if max number of retransissions exceeds send timeout  event to user*/
	dhcp_client.callback(dhcp_event_timeout);
	dhcp_free();
	return;
      }
      dhcp_send();
      timer_set(dhcp_client.timer,DHCP_TIMEOUT);
      break;
    }
    case dhcp_state_bound:
    {
      dhcp_client.state = dhcp_state_rebinding;
      dhcp_client.rtx = 0xff;
    }
    case dhcp_state_rebinding:
    case dhcp_state_requesting:
    {
      if(++dhcp_client.rtx > DHCP_RTX)
      {
	/* if max number of retransissions exceeds send timeout  event to user*/
	if(dhcp_client.state == dhcp_state_requesting)
	  dhcp_client.callback(dhcp_event_timeout);
	else
	  dhcp_client.callback(dhcp_event_lease_expiring);
	dhcp_free();
      }
      /* send DHCPREQUEST */
      dhcp_send();
      timer_set(dhcp_client.timer,DHCP_TIMEOUT);
      break;
    }
    default:
      DEBUG_PRINT_COLOR(B_IRED,"dhcp time: not handled state %x\n",dhcp_client.state);
      break;
  }
}
uint8_t dhcp_parse_options(const struct dhcp_header * dhcp_header)
{
  if(!dhcp_header)
    return 0;
  
  /* store ip address proposed by dhcp server */
  memcpy(&dhcp_client.addr,&dhcp_header->yiaddr,sizeof(ip_address));
  
  uint8_t msgtype=0;
  uint8_t * options = (uint8_t*)(dhcp_header+1);
  DEBUG_PRINT_COLOR(B_IGREEN,"dhcp parsing options:");
  while(1)
  {
    uint8_t optype = *options;
    uint8_t optlen = *(options+1);
    const uint8_t * optdata = (options+2);
    if(optype == DHCP_OPTION_PAD)
    {
      DEBUG_PRINT_COLOR(IGREEN,"PAD ");
      options++;
      continue;
    }
    else if(optype == DHCP_OPTION_END)
    {
      DEBUG_PRINT_COLOR(IGREEN,"END ");
      break;
    }
    else if(optype == DHCP_OPTION_MESSAGE_TYPE && optlen == DHCP_OPTION_LEN_MESSAGE_TYPE)
    {
      msgtype = *optdata;
      DEBUG_PRINT_COLOR(IGREEN,"MSGTYPE[%d] ",msgtype);
    }
    else if(optype == DHCP_OPTION_ROUTER && optlen/4 > 0)
    {
      memcpy(&dhcp_client.gateway,optdata,sizeof(ip_address));
      DEBUG_PRINT_COLOR(IGREEN,"ROUTER ",msgtype);
    }
    else if(optype == DHCP_OPTION_SUBNET_MASK && optlen == DHCP_OPTION_LEN_SUBNET_MASK)
    {
      memcpy(&dhcp_client.netmask,optdata,sizeof(ip_address));
      DEBUG_PRINT_COLOR(IGREEN,"NETMASK ",msgtype);
    }
    else if(optype == DHCP_OPTION_SERVER_ID && optlen == DHCP_OPTION_LEN_SERVER_ID)
    {
      memcpy(&dhcp_client.server,optdata,sizeof(ip_address));
      DEBUG_PRINT_COLOR(IGREEN,"SERVER_ID ",msgtype);
    }
    else if(optype == DHCP_OPTION_IP_ADDRESS_LEASE_TIME && optlen == DHCP_OPTION_LEN_IP_ADDRESS_LEASE_TIME)
    {
      dhcp_client.time_rebind = ntoh32(*((uint32_t*)optdata));
      DEBUG_PRINT_COLOR(IGREEN,"TIME[%lx] ",dhcp_client.time_rebind);
    }
    options += optlen +2;
  }
  DEBUG_PRINT("\n");
  return msgtype;
}
uint8_t dhcp_send(void)
{
  /* get UDP buffer */
  struct dhcp_header * dhcp_header = (struct dhcp_header*)udp_get_buffer();
  /* clear header */
  memset(dhcp_header,0,sizeof(*dhcp_header));
  /* set operation to BOOTREQUEST */
  dhcp_header->op = DHCP_OP_BOOTREQUEST;
  /* set hardware address type to Ethernet*/
  dhcp_header->htype = DHCP_HTYPE_ETH;
  /* set hardware address length */
  dhcp_header->hlen = DHCP_HLEN_ETH;
  /* set transaction id */
  dhcp_header->xid = HTON32(DHCP_XID);
  /* set flags to broadcast */
  dhcp_header->flags = HTON16(DHCP_FLAG_BROADCAST);
  /* set COOKIE */
  dhcp_header->cookie = HTON32(DHCP_COOKIE);
  /* set our ip address */
  memcpy(&dhcp_header->ciaddr,ip_get_addr(),sizeof(ip_address));
  /* set our ethernet address */
  memcpy(&dhcp_header->charrd,ethernet_get_mac(),sizeof(ethernet_address));
  
  uint8_t msg_type;
  /* get message type according to current state */
  switch(dhcp_client.state)
  {
    case dhcp_state_selecting:
      msg_type = DHCPDISCOVER;
      break;
    case dhcp_state_rebinding:
    case dhcp_state_requesting:
      msg_type = DHCPREQUEST;
      break;
    default:
      return 1;
  };
  
  uint8_t * options = (uint8_t*)(dhcp_header +1);
  /* set option: message type */
  options = dhcp_add_msg_type(options,msg_type);
  /* set option: request params */
  options = dhcp_add_params(options);
  if(dhcp_client.state == dhcp_state_requesting ||
     dhcp_client.state == dhcp_state_rebinding)
  {
    options = dhcp_add_request_ip(options);
    options = dhcp_add_server_id(options);
#if DHCP_USE_HOST_NAME    
    options = dhcp_add_host_name(options);
#endif    
  }
  /* set end of options */
  options = dhcp_add_end(options);
  /* set packet's length */
  uint16_t len = options - (uint8_t*)dhcp_header;
  
  /* bind local to bootpc*/
  if(!udp_bind_local(dhcp_client.socket,UDP_PORT_BOOTPC))
  {
    DEBUG_PRINT_COLOR(B_IRED,"dhcp send: can't bind local\n");
    return 1;
  }
  /* bind remote to bootps */
  const ip_address * ip_remote = 0;
  if(dhcp_client.state == dhcp_state_rebinding)
    ip_remote = (const ip_address *)&dhcp_client.server;
  if(!udp_bind_remote(dhcp_client.socket,UDP_PORT_BOOTPS,ip_remote))
  {
    DEBUG_PRINT_COLOR(B_IRED,"dhcp send: can't bind remote\n");
    return 1;
  }
  /* send udp packet */
  return udp_send(dhcp_client.socket,len);
}

uint8_t * dhcp_add_host_name(uint8_t * ptr)
{
#if DHCP_USE_HOST_NAME  
  const prog_char * host_name = PSTR(DHCP_HOST_NAME);
  uint8_t len = (uint8_t)strlen_P(host_name)+1;
  *(ptr++) = DHCP_OPTION_HOST_NAME;
  *(ptr++) = len;
  memcpy_P(ptr,host_name,len);
  ptr+=len;
#endif  
  return ptr;
}

uint8_t * dhcp_add_msg_type(uint8_t * ptr,uint8_t msgtype)
{
  *(ptr++) = DHCP_OPTION_MESSAGE_TYPE;
  *(ptr++) = DHCP_OPTION_LEN_MESSAGE_TYPE;
  *(ptr++) = msgtype;
  return ptr;
}
uint8_t * dhcp_add_end(uint8_t * ptr)
{
  *(ptr++) = DHCP_OPTION_END;
  return ptr;
}
uint8_t * dhcp_add_params(uint8_t * ptr)
{
  *(ptr++) = DHCP_OPTION_PARAM_REQUEST_LIST;
  *(ptr++) = 2;
  *(ptr++) = DHCP_OPTION_SUBNET_MASK;
  *(ptr++) = DHCP_OPTION_ROUTER;
  return ptr;
}
uint8_t * dhcp_add_request_ip(uint8_t * ptr)
{
  *(ptr++) = DHCP_OPTION_REQUESTED_IP_ADDR;
  *(ptr++) = DHCP_OPTION_LEN_REQUESTED_IP_ADDR;
  memcpy(ptr,&dhcp_client.addr,sizeof(ip_address));
  ptr+=sizeof(ip_address);
  return ptr;
}
uint8_t * dhcp_add_server_id(uint8_t * ptr)
{
  *(ptr++) = DHCP_OPTION_SERVER_ID;
  *(ptr++) = DHCP_OPTION_LEN_SERVER_ID;
  memcpy(ptr,&dhcp_client.server,sizeof(ip_address));
  ptr+=sizeof(ip_address);
  return ptr; 
}
void dhcp_stop(void)
{
  dhcp_free();
}
void dhcp_free(void)
{
  udp_socket_free(dhcp_client.socket);
  timer_free(dhcp_client.timer);
  memset(&dhcp_client,0,sizeof(dhcp_client));
}