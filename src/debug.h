/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DEBUG_H_INCLUDED
#define _DEBUG_H_INCLUDED

#include "debug_conf.h"
#include "arch/uart.h"

#include <string.h>



#define __file__	PSTR(__FILE__)

extern fcheat_file debug_file_fcheat;// = FCHEAT_STATIC_FDEVOPENWR(rs_putc,rs_getc);
extern FILE * df;// = (FILE*)&fileRS;
/* Normal colors*/
#define BLACK 		"\033[0;30m"
#define RED 		"\033[0;31m"
#define GREEN 		"\033[0;32m"
#define YELLOW 		"\033[0;33m"
#define BLUE 		"\033[0;34m"
#define MAGENTA		"\033[0;35m"
#define CYAN 		"\033[0;36m"
#define WHITE		"\033[0;37m"

/* Bold */
#define B_BLACK 	"\033[1;30m"
#define B_RED 		"\033[1;31m"
#define B_GREEN 	"\033[1;32m"
#define B_YELLOW 	"\033[1;33m"
#define B_MAGENTA	"\033[1;35m"
#define B_CYAN 		"\033[1;36m"
#define B_BLUE 		"\033[1;34m"
#define B_WHITE		"\033[1;37m"

/* Underline */
#define U_BLACK 	"\033[4;30m"
#define U_RED 		"\033[4;31m"
#define U_GREEN 	"\033[4;32m"
#define U_YELLOW 	"\033[4;33m"
#define U_MAGENTA	"\033[4;35m"
#define U_CYAN 		"\033[4;36m"
#define U_BLUE 		"\033[4;34m"
#define U_WHITE		"\033[4;37m"

/* Intensive colors*/
#define IBLACK 		"\033[0;90m"
#define IRED 		"\033[0;91m"
#define IGREEN 		"\033[0;92m"
#define IYELLOW 	"\033[0;93m"
#define IMAGENTA	"\033[0;95m"
#define ICYAN 		"\033[0;96m"
#define IBLUE 		"\033[0;94m"
#define IWHITE		"\033[0;97m"

/* Bold and intensive colors */
#define B_IBLACK 	"\033[1;90m"
#define B_IRED 		"\033[1;91m"
#define B_IGREEN 	"\033[1;92m"
#define B_IYELLOW 	"\033[1;93m"
#define B_IBLUE 	"\033[1;94m"
#define B_IMAGENTA	"\033[1;95m"
#define B_ICYAN 	"\033[1;96m"
#define B_IWHITE	"\033[1;97m"

/* Background colors */
#define BCKGND_BLACK 	"\033[40m"
#define BCKGND_RED 	"\033[41m"
#define BCKGND_GREEN 	"\033[42m"
#define BCKGND_YELLOW 	"\033[43m"
#define BCKGND_BLUE 	"\033[44m"
#define BCKGND_MAGENTA	"\033[45m"
#define BCKGND_CYAN 	"\033[46m"
#define BCKGND_WHITE	"\033[47m"

#define ENDCOLOR 	"\033[0m"

#define DEBUG_FH		df


#define DEBUG_INIT() uart_init()

#ifdef DEBUG_MODE
	#define DEBUG(x)		(x)
	#define DEBUG_PRINT(fmt,args...)	fprintf_P((DEBUG_FH),PSTR(fmt),## args)
	#define DEBUG_PRINT_COLOR(c,fmt,args...)	fprintf_P((DEBUG_FH),PSTR(c fmt ENDCOLOR),## args)
	#define DEBUG_SET_COLOR(c)	{fprintf_P((DEBUG_FH),PSTR((c)));}
	#define DEBUG_CLR_COLOR()	{fprintf_P((DEBUG_FH),PSTR((ENDCOLOR)));} 
#else
	#define DEBUG(x)
	#define DEBUG_PRINT(fmt,args...) 
	#define DEBUG_PRINT_COLOR(c,fmt,args...)
	#define DEBUG_SET_COLOR(c) 
	#define DEBUG_CLR_COLOR() 
#endif

#ifdef DEBUG_MODE	
	#define DBG_CH_INFO	'I'
	#define DBG_CH_WARN	'W'
	#define DBG_CH_ERROR	'E'	
	#define DBG_CLR_INFO	""
	#define DBG_CLR_WARN	B_IYELLOW
	#define DBG_CLR_ERROR	B_IRED
	#define DBG_MSG(ch,color,fmt,args...)		{						\
							fprintf_P((DEBUG_FH),PSTR(color));		\
							fprintf_P((DEBUG_FH),				\
								PSTR("[%c|%-20S|%-20s]"),		\
								(ch),					\
								(__file__),			\
								(__func__));				\
							fprintf_P((DEBUG_FH),				\
								PSTR(fmt ENDCOLOR),## args);	\
							}						
	
	#define DBG_INFO(fmt,args...)	DBG_MSG(DBG_CH_INFO,DBG_CLR_INFO,fmt,## args)	
	#define DBG_WARN(fmt,args...)	DBG_MSG(DBG_CH_WARN,DBG_CLR_WARN,fmt,## args)
	#define DBG_ERROR(fmt,args...)	DBG_MSG(DBG_CH_ERROR,DBG_CLR_ERROR,fmt,## args)
#else
	#define DBG_INFO(fmt,args...)	
	#define DBG_WARN(fmt,args...)	
	#define DBG_ERROR(fmt,args...)	
#endif 

#endif //_DEBUG_H_INCLUDED
