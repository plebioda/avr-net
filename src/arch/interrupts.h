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
* @addtogroup arch
* @{
*/
inline void interrupt_timer0_init(void)
{
	TCNT0 = 0;
	TCCR0 |= (0<<FOC0)|
		 (0<<WGM00)|
		 (0<<COM01)|
		 (0<<COM00)|
		 (1<<WGM01)|
		 (1<<CS02)|
		 (0<<CS01)|
		 (1<<CS00);
	OCR0 = 124;
	TIMSK |= (1<<OCIE0)|
		 (0<<TOIE0);
}

inline void interrupt_timer1_init(void)
{
	TCCR1A = (0<<COM1A1)|
		 (0<<COM1A0)|
		 (0<<COM1B1)|
		 (0<<COM1B0)|
		 (0<<COM1C1)|
		 (0<<COM1C0)|
		 (0<<WGM11)|
		 (0<<WGM10);
	TCCR1B = (0<<ICNC1)|
		 (0<<ICES1)|
		 (0<<WGM13)|
		 (0<<WGM12)|
		 (0<<CS12)|
		 (0<<CS11)|
		 (0<<CS10);
	TCCR1C = (0<<FOC1A)|
		 (0<<FOC1B)|
		 (0<<FOC1C);
	TIMSK |= (0<<TOIE1);
}

inline void interrupt_exint_init(void)
{
	EICRB |=	(0<<ISC71)|(0<<ISC70) |
			(0<<ISC61)|(1<<ISC60) |
			(0<<ISC51)|(0<<ISC50) |
			(0<<ISC41)|(0<<ISC40);
	EICRA |=	(0<<ISC31)|(0<<ISC30) |
			(0<<ISC21)|(0<<ISC20) |
			(0<<ISC11)|(0<<ISC10) |
			(0<<ISC01)|(0<<ISC00);
	EIMSK |=	(1<<INT7)|
			(1<<INT6)|
			(0<<INT5)|
			(0<<INT4)|
			(0<<INT3)|
			(0<<INT2)|
			(0<<INT1)|
			(0<<INT0);
}

/**
* @}
*/
#endif //_INTERRUPTS_H
