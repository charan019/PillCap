/*! \file****************************************************************
 	SoftMicrowire.h
		\brief Global variable and constant declarations.

\section FH File Revision History
\par 	0.10		12/28/13
		Initial Revision
*/

#ifndef _SOFT_MICROWIRE_H_
#define _SOFT_MICROWIRE_H_

#include <stdint.h>
#include <avr/io.h>

/* default pins */
#define SOFT_MIROWIRE_CLK_DDR			DDRB
#define SOFT_MIROWIRE_CLK_PORT			PORTB
#define SOFT_MIROWIRE_CLK_PIN			0x02

#define SOFT_MIROWIRE_DATA_DDR			DDRB
#define SOFT_MIROWIRE_DATA_WRITE_PORT	PORTB
#define SOFT_MIROWIRE_DATA_READ_PORT	PINB
#define SOFT_MIROWIRE_DATA_PIN			0x01

void Soft_Microwire_Write_Byte(uint8_t);
uint8_t Soft_Microwire_Read_Byte(void);

#endif // _SOFT_MICROWIRE_H_