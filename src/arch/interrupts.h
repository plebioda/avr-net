/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H
/**
* \addtogroup arch
* @{
*/
#define interrupt_timer0_init() \
  TCNT0 = 0; \
  TCCR0 |= (1<<WGM01)|(0<<WGM00)|(0<<CS02)|(1<<CS01)|(1<<CS00); \
  OCR0 = 31; \
  TIMSK |= (1<<OCIE0)|(0<<TOIE0)

#define interrupt_exint_init() \
  EICRB |=  (0<<ISC71)|(0<<ISC70) | \
	    (0<<ISC61)|(1<<ISC60) | \
	    (0<<ISC51)|(0<<ISC50) | \
	    (0<<ISC41)|(0<<ISC40); \
  EICRA |=  (0<<ISC31)|(0<<ISC30) | \
	    (0<<ISC21)|(0<<ISC20) | \
	    (0<<ISC11)|(0<<ISC10) | \
	    (0<<ISC01)|(0<<ISC00); \
  EIMSK |=  (1<<INT7)|\
	    (0<<INT6)|\
	    (0<<INT5)|\
	    (0<<INT4)|\
	    (0<<INT3)|\
	    (0<<INT2)|\
	    (0<<INT1)|\
	    (0<<INT0)

/**
* @}
*/
#endif //_INTERRUPTS_H