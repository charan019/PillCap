/*! \file****************************************************************
 	PCF85063.h
		\brief Global variable and constant declarations.

\section FH File Revision History
\par 	0.10		12/28/13
		Initial Revision
*/
#define PORTA _SFR_IO8(0x1B)
#define PORTA0 0
#define PORTA1 1
#define PORTA2 2
#define PORTA3 3
#define PORTA4 4
#define PORTA5 5
#define PORTA6 6
#define PORTA7 7

#ifndef _PCF85063_H_
#define _PCF85063_H_

#include <stdint.h>
#include <avr/io.h>

/* default pins */
#define PCF85063_CS_PORT		PORTA
#define PCF85063_CS_PIN			0x04

#define PCF85063_INTERVAL_SECS	10

#define PCF85063_TIME_LENGTH	7
enum
{
	PCF85063_SECONDS,
	PCF85063_MINUTES,
	PCF85063_HOURS,
	PCF85063_DAYS,
	PCF85063_WEEKDAYS,
	PCF85063_MONTHS,
	PCF85063_YEARS,
};

typedef enum {
	PCF85063_ADDR_CONTROL_1 = 0x20,		// includes 0x20 sub-address
	PCF85063_ADDR_CONTROL_2,
	PCF85063_ADDR_OFFSET,
	PCF85063_ADDR_RAM_BYTE,
	PCF85063_ADDR_SECONDS,
	PCF85063_ADDR_MINUTES,
	PCF85063_ADDR_HOURS,
	PCF85063_ADDR_DAYS,
	PCF85063_ADDR_WEEKDAYS,
	PCF85063_ADDR_MONTHS,
	PCF85063_ADDR_YEARS,
	PCF85063_ADDR_SECOND_ALARM,
	PCF85063_ADDR_MINUTE_ALARM,
	PCF85063_ADDR_HOUR_ALARM,
	PCF85063_ADDR_DAY_ALARM,
	PCF85063_ADDR_WEEKDAY_ALARM,
	PCF85063_ADDR_TIMER_VALUE,
	PCF85063_ADDR_TIMER_MODE
} PCF85063_register_t;

#define PCF85063_TF_BIT			0x80

void PCF85063_Setup(void);
void PCF85063_Write(PCF85063_register_t, uint8_t, uint8_t*);
void PCF85063_Read(PCF85063_register_t, uint8_t, uint8_t*);
void PCF85063_Write_Byte(PCF85063_register_t, uint8_t);
uint8_t PCF85063_Read_Byte(PCF85063_register_t);

#endif // _PCF85063_H_
