/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "debug.h"

fcheat_file debug_file_fcheat = FCHEAT_STATIC_FDEVOPENWR(uart_putc,uart_getc);
FILE * df = (FILE*)&debug_file_fcheat;
