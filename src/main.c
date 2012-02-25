/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG_MODE
#include "debug.h"

#include "arch/exmem.h"
#include "arch/spi.h"
#include "arch/uart.h"


#include "net/hal.h"
#include "net/ethernet.h"
#include "net/ip.h"


int main(void)
{
  DEBUG_INIT();
  spi_init(0);
  ethernet_address my_mac = {'<','P','A','K','O','>'};
  ip_address my_ip = {192,168,1,205};
  ethernet_init(&my_mac);
  ip_init(&my_ip);
  hal_init(my_mac);
  uint8_t rev = enc28j60_get_revision();
  DEBUG_PRINT_COLOR(B_IRED,"enc28j60 rev = %d\n",rev);  
  DEBUG_PRINT_COLOR(B_IRED,"Initialized...\n");  
  
  for(;;)
  {
	ethernet_handle_packet();
  }
  return 0;
}
