/*! \file****************************************************************
 	com.c
		\brief Parses and responds to commands from the serial port


\section FH File Revision History
\par 	0.10		12/26/13
		Initial Revision

*/

#ifndef _COM_ROUTINES_H_
#define _COM_ROUTINES_H_

#include <stdint.h>

#define EE_STRING_LENGTH	21

void Com_Rx_Control_Char(char);		//!< Respond to control characters, with out carrige return
void Com_Tx_Title(void);			//!< Send model, version and serial number to com port
void Com_Tx_Time_Stamp(void);		//!< Send data point to com port
void Com_Parse_Error(void);			//!< Com error handling subroutine
void Com_Parse_String(char*);		//!< Parse and respond to commands from the com port

#endif