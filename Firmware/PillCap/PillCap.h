/*! \file****************************************************************
 	PillCap.h
		\brief Global variable and constant declarations.

\section FH File Revision History
\par 	0.10		12/28/13
		Initial Revision
*/

#ifndef _PILLCAP_H_
#define _PILLCAP_H_

#include <stdint.h>

#define F_CPU				125000UL//!< Crystal frequency, for setting up baud rates and timers

#define MAJ_VERSION			0		//!< Major Version number
#define MIN_VERSION			3		//!< Minor Version number
#define TER_VERSION			0		//!< Tertiary Version number

#define TRUE				1		//!< Global definition of TRUE
#define FALSE				0		//!< Global definition of FALSE

////////////////////////////////////////////////////////////////
// Hardware Defines

#define LED_PORT			PORTA
//#define ANALOG_POWER		0x80
#define LED_RED				0x40	//!< SPI_MOSI
#define IR_SENSOR			0x20	//!< SPI_MISO
#define LED_YELLOW			0x10    //!< SPI_CLK
#define LED_GREEN			0x08	//!<
//#define RTC_CS			0x04	//!<
//#define RTC_INT			0x02	//!< 
//#define VREF				0x01

// Port B
//#define RESET				0x08    //!< Reset pin
//#define BUTTON			0x04    //!< 
#define MICROWIRE_CLK		0x02	//!<
#define MICROWIRE_DATA		0x01	//!< 

////////////////////////////////////////////////////////////////
// global variables
////////////////////////////////////////////////////////////////
// OS Schedule
//#define DO_UNUSED_TASK	0x20    //!< Schedule 
#define DO_POWERDOWN_TASK	0x10	//!< Schedule power down
#define DO_BUTTON_TASK		0x08    //!< Schedule button response
#define DO_RTC_TASK			0x04	//!< Schedule task to deal with Timer 1 stuff: trigger and input capture
#define DO_COM_RX			0x02	//!< Schedule Com 0 Receive task, also enables port code in ComSubs.c

extern volatile uint8_t		Tasks;	//!< OS Schedule tracking variable

#endif //_PILLCAP_H_
