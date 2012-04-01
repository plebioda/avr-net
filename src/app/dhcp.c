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
  uint32_t magic_cookie;	//99, 130, 83 and 99
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
  
} dhcp_client;


static void dhcp_udp_callback(udp_socket_t socket,uint8_t * data,uint16_t len);
static void dhcp_timer_callback(timer_t timer,void * arg);
static void dhcp_free(void);

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
  timer_set(dhcp_client.timer,100);
  return 0;
}

uint8_t dhcp_stop(void)
{
  return 0;
}

static void dhcp_udp_callback(udp_socket_t socket,uint8_t * data,uint16_t len)
{
  
}

static void dhcp_timer_callback(timer_t timer,void * arg)
{
  if(timer != dhcp_client.timer)
  {
    dhcp_client.callback(dhcp_event_error);
    dhcp_free();
  }
  DEBUG_PRINT_COLOR(B_IGREEN,"dhcp_timer_callback\n");
  switch(dhcp_client.state)
  {
   
    default:
//       DEBUG_PRINT_
      break;
  }
}

void dhcp_free(void)
{
  udp_socket_free(dhcp_client.socket);
  timer_free(dhcp_client.timer);
}