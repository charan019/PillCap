/*! \file****************************************************************
 	SoftUART.C
		\brief Interrupt mode driver for UARTs.

		Code adapted from Atmel AVR application Note AVR306.

\section FH File Revision History
\par 	0.2		7/5/12
		Initial Revision, Demo 7/5/12

*/

#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "SoftUART.h"
#include "Com.h"
#include "PillCap.h"
#include "PowerDown.h"

#define NOP()			asm volatile("nop");

#define COMM_PORT_0		0x00

// UART Buffer Defines
#ifndef RX0_BUFFER_SIZE
	#define RX0_BUFFER_SIZE 32 /* 1,2,4,8,16,32,64,128 or 256 bytes */ // default value
#endif
#define RX0_BUFFER_MASK (RX0_BUFFER_SIZE - 1)

#ifndef RX0_PARSE_BUFFER_SIZE
	#define RX0_PARSE_BUFFER_SIZE RX0_BUFFER_SIZE
#endif

#if (RX0_BUFFER_SIZE & RX0_BUFFER_MASK)
	#error RX0 buffer size is not a power of 2
#endif

static int8_t Rx0Buf[RX0_BUFFER_SIZE];
static volatile uint8_t Rx0Head = 0;
static volatile uint8_t Rx0Tail = 0;

/*************************************************************//**
\brief Get and store received character, schedule parse task
\return	none
***************************************************************/
ISR(PCINT0_vect)
{
	static uint8_t Data = 0;
	static uint8_t TempU8;
	static uint8_t i;

	// if not Rx pin and pull up activated then Rx UART byte
	if(!(PINA & UART_SOFT_RX_PIN) && (PORTA & UART_SOFT_RX_PIN))
	{
		for(i=0; i<68; i++)			// 2400:11, 1200:29, 600:68 @125K
			NOP();

//		i |= 0x02; // time place holder
		if(UART_SOFT_RX_PORT & UART_SOFT_RX_PIN)
			Data |= 0x80;
		else
		{
			Data &= 0x7F;
			NOP();NOP();NOP();
		}
		Data >>= 1;

		i &= ~0x02;
		for(i=0; i<45; i++)			// 2400:9, 1200:19, 600:45 @125K
			NOP();
		// read the received bit and shift
		for(TempU8=1; TempU8<7; TempU8++)
		{
			i |= 0x02;	// time place holder
			if(UART_SOFT_RX_PORT & UART_SOFT_RX_PIN)
				Data |= 0x80;
			else
			{
				Data &= 0x7F;
				NOP();NOP();
			}
			for(i=0; i<45; i++)		// 2400:?, 1200:19, 600:45 @125K
				NOP();

			i &= ~0x02;
			Data >>= 1;
		}
		NOP();NOP();
		// get last bit, no shift
		i &= ~0x02;	// time place holder
		if(UART_SOFT_RX_PORT & UART_SOFT_RX_PIN)
			Data |= 0x80;
		else
			Data &= 0x7F;

		if(ComTimer) // only accept characters if Com enabled, 1 min after button push or full command Rxd
		{
			// calculate buffer index
			TempU8 = (Rx0Head + 1) & RX0_BUFFER_MASK;
			Rx0Head = TempU8; 						// store new index
			Rx0Buf[TempU8] = Data; 					// store received data in buffer
			Tasks |= DO_COM_RX;						// schedule parse task
		}
	}
	else if(!(PINA & 0x02))
		Tasks |= DO_RTC_TASK;

	GIFR = 0x10;							// clear int flag
}

/*************************************************************//**
\brief get next byte out of buffer
\return	data byte from buffer
***************************************************************/
static int8_t Com0_Rx_Byte(void)
{
	uint8_t tmptail;

	while (Rx0Head == Rx0Tail)
		;
	tmptail = (Rx0Tail + 1) & RX0_BUFFER_MASK;		// calculate buffer index
	Rx0Tail = tmptail; 								// store new index
	return Rx0Buf[tmptail]; 						// return data
}

/*************************************************************//**
\brief Puts byte in 2nd parameter the to be transmitted buffer of the com port based on the 1st parameter

This is more interesting if there are multiple com ports

\param Data One byte of data to be transmitted
\return	none
***************************************************************/
void Soft_UART_Tx_Byte(uint8_t DataByte)
{
	static uint8_t i,j;

	// start transmission
	// send start bit
	LED_PORT |= LED_YELLOW | LED_GREEN;	// LEDs off
	UART_SOFT_TX_PORT &= ~UART_SOFT_TX_PIN;
	for(i=0; i<51; i++)			// 2400:12, 1200:25, 600:51 @125K
		NOP();

	for(i=0; i<8; i++)
	{
		if(DataByte & 0x01)
			UART_SOFT_TX_PORT |= UART_SOFT_TX_PIN;
		else
		{
			UART_SOFT_TX_PORT &= ~UART_SOFT_TX_PIN;
			NOP();NOP();NOP();
		}
		for(j=0; j<48; j++)		// 2400:9, 1200:21, 600:48 @125K
			NOP();

		DataByte >>= 1;
	}
	UART_SOFT_TX_PORT |= UART_SOFT_TX_PIN;
	for(j=0; j<48; j++)			// 2400:9, 1200:21, 600:48 @125K
		NOP();
}

/*************************************************************//**
\brief Put 0 terminated message from RAM into the TX buffer

\param string Message Pointer to message String to be transmitted
\return	none
***************************************************************/
void Soft_UART_Tx_Message(const char *string)
{
	while(*string)
		Soft_UART_Tx_Byte(*string++);
}

/*************************************************************//**
\brief Put 0 terminated message from FLASH into the TX buffer

\param string Message Pointer to message String in flash to be transmitted
\return	none
***************************************************************/
void Soft_UART_Tx_Const_Message(const char *string)
{
	while(pgm_read_byte(&(*string)))
		Soft_UART_Tx_Byte(pgm_read_byte(&(*string++)));
}

/*************************************************************//**
\brief Check Rx buffer for incoming messages on ports, parses and responds
\return none
***************************************************************/
void Soft_UART_Rx_Task(void)
{
	char RxdByte;
	volatile uint8_t *RxHeadptr, *RxTailptr;
	int8_t (*fptrComRxByte0)(void) = NULL;
	uint16_t RxBufferSize;
	static char *ComRxdStringPtr;
	static uint16_t *CharCountPtr;
	static uint16_t CharCount0 = 0;
	static char Com0RxdString[RX0_PARSE_BUFFER_SIZE];

	Tasks &= ~DO_COM_RX;

	RxHeadptr = &Rx0Head;
	RxTailptr = &Rx0Tail;
	ComRxdStringPtr = &Com0RxdString[0];
	CharCountPtr = &CharCount0;
	RxBufferSize = RX0_PARSE_BUFFER_SIZE;

	fptrComRxByte0 = Com0_Rx_Byte;

	while(*RxHeadptr != *RxTailptr)							// check to see if any bytes are in the Rx buffer
	{
		RxdByte = fptrComRxByte0();

		if(*CharCountPtr >= RxBufferSize)					// if over range
		{
			(*CharCountPtr) = 0;
			(*ComRxdStringPtr) = 0;							// reset string
			Com_Parse_Error();								// send error message
		}
		else if(RxdByte == ESCAPE)							// if <ESC>
		{
			(*CharCountPtr) = 0;
			(*ComRxdStringPtr) = 0;							// reset string
			Com_Parse_Error();								// send error message
		}
		else if(RxdByte == BACKSPACE)						// if backspace
		{
			(*CharCountPtr)--;								// go back one character
		}
		else if(isalnum(RxdByte) || ispunct(RxdByte) || (' ' == RxdByte))// check for chars, numbers or punctuation
		{
			*(ComRxdStringPtr+*CharCountPtr) = RxdByte;
			(*CharCountPtr)++;
		}
		else if(RxdByte == '\r')							// if CR, command completed
		{
			*(ComRxdStringPtr+*CharCountPtr) = 0x00;		// NULL terminate string (replace '\r' with 0x00)
			Com_Parse_String(ComRxdStringPtr);				// else parse
			(*ComRxdStringPtr) = 0;							// reset string
			(*CharCountPtr) = 0;							// reset string pointer
		}
//		else ignore character
	}//while
}
