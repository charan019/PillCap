
#ifndef _SOFT_UART_H_
#define _SOFT_UART_H_

#include <avr/io.h>

#define UART_SOFT_TX_PORT	 	PORTA
#define UART_SOFT_RX_PORT	 	PINA
#define UART_SOFT_TX_PIN		0x40
#define UART_SOFT_RX_PIN		IR_SENSOR

#define COM0_BAUDRATE			600u	//!< Baud rate
#define RX0_BUFFER_SIZE 		32		//!< 1,2,4,8,16,32,64,128,256,512... bytes
#define RX0_PARSE_BUFFER_SIZE RX0_BUFFER_SIZE//!< Parse buffer = Transmit buffer

#define COM_ERR_SHORT			0x01    //!< Number passed to error handler for message too short error
#define COM_ERR_LONG			0x02    //!< Number passed to error handler for message too long error
#define COM_ERR_CRC				0x03    //!< Number passed to error handler for CRC error
#define COM_ERR_ESC				0x04	//!< Number passed to error handler for escape error

#define ESCAPE 					0x1B    //!< ACSII for escape button
#define BACKSPACE				0x08    //!< ACSII for backspace button

#define TX_NEWLINE	{Soft_UART_Tx_Const_Message(PSTR("\r\n"));} //!< Macro to send carrige return and line feed

void Soft_UART_Tx_Byte(const uint8_t);		//!< Load one byte into tx buffer
void Soft_UART_Tx_Message(const char *);	//!< Load string to tx buffer
void Soft_UART_Tx_Const_Message(const char *);//!< Load constant string to tx buffer
void Soft_UART_Rx_Task(void);				//!< handle incoming bytes

#endif //_SOFT_UART_H_