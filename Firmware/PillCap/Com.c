/*! \file****************************************************************
 	com.c
		\brief Parses and responds to commands from the serial port

\section FH File Revision History
\par 	0.10		12/28/13
		Initial Revision
		
*/

#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "Com.h"
#include "SoftUART.h"
#include "PillCap.h"
#include "PowerDown.h"
#include "PCF85063.h"
#include "DoseClock.h"

#define ACK()	{Soft_UART_Tx_Const_Message(PSTR("A"));} //!< send acknowlege to com port
#define NACKU()	{Soft_UART_Tx_Const_Message(PSTR("NU"));}//!< send not acknowlege unrecognized to com port
#define NACKR()	{Soft_UART_Tx_Const_Message(PSTR("NR"));}//!< send not acknowlege range error to com port
#define NACKB()	{Soft_UART_Tx_Const_Message(PSTR("NB"));}//!< send not acknowlege busy to com port
#define TX_TAB(){Soft_UART_Tx_Const_Message(PSTR("	"));}//!< send TAB character to com port

#define UCHAR_2_A_SIZE	3		//!< Global definition of buffer size for unsigned character to ACSII conversion
#define CHAR_2_A_SIZE	4       //!< Global definition of buffer size for character to ACSII conversion
#define UINT_2_A_SIZE	6       //!< Global definition of buffer size for unsigned integer to ACSII conversion
#define INT_2_A_SIZE	7       //!< Global definition of buffer size for integer to ACSII conversion
#define ULONG_2_A_SIZE	8		//!< Global definition of buffer size for unsigned long to ACSII conversion
#define LONG_2_A_SIZE	9		//!< Global definition of buffer size for long to ACSII conversion

#define EEPROM_SIZE		512

EEMEM uint8_t EEFacility[EE_STRING_LENGTH] = {0};
EEMEM uint8_t EEDoctor[EE_STRING_LENGTH] = {0};
EEMEM uint8_t EETreatment[EE_STRING_LENGTH] = {0};
EEMEM uint8_t EEPatient[EE_STRING_LENGTH] = {0};
EEMEM uint8_t EEClient[EE_STRING_LENGTH] = {0};
EEMEM uint8_t EEBatStartDate[PCF85063_TIME_LENGTH] = {0,0,0,1,6,1,0};
EEMEM uint16_t EEDosePeriodHours = 8;

/*************************************************************//**
\brief    Com error handling subroutine

\return	none
***************************************************************/
void Com_Parse_Error(void)
{
	NACKU();
}

/*************************************************************//**
\brief    Com special character handling subroutine

\return	none
***************************************************************/
void Com_Rx_Control_Char(char RxdChar)
{
	NACKU();
}

/*************************************************************//**
\brief Send model, version and serial number to com port

\return	none
***************************************************************/
void Com_Tx_Title(void)
{
	uint8_t Status;
	Soft_UART_Tx_Const_Message(PSTR("T:PillCap:"));
	Soft_UART_Tx_Byte(MAJ_VERSION+0x30);
	Soft_UART_Tx_Byte('.');
	Soft_UART_Tx_Byte(MIN_VERSION+0x30);
	Soft_UART_Tx_Byte(TER_VERSION+0x30);
	Soft_UART_Tx_Byte(':');
	if(!GetTotalDoses() || GetTotalDoses()==0xFF)
		Status = 4;	// un-programmed
	else if(GetCurrentDose() == GetTotalDoses())
		Status = 0;	// done
	else if(GetFirstDoseTimeAbs() > GetCurrentTimeAbs())
		Status = 2;	// programmed, not running
	else
		Status = 1;	// running
	Soft_UART_Tx_Byte(Status+0x30);
	TX_NEWLINE;
}

/*************************************************************//**
\brief Set trigger to given value, if no value given then send current value to com port
\param CommandString containing new trigger value
\return	none
***************************************************************/
static void Com_Tx_Status(void)
{
	char ToABuffer[ULONG_2_A_SIZE];

	Soft_UART_Tx_Const_Message(PSTR("P:"));
	utoa(DoseReady(), ToABuffer, 10);
	Soft_UART_Tx_Message(ToABuffer);

	Soft_UART_Tx_Byte(':');
	utoa(GetCurrentDose(), ToABuffer, 10);
	Soft_UART_Tx_Message(ToABuffer);
	
	Soft_UART_Tx_Byte(':');
	ultoa(GetFirstDoseTimeAbs(), ToABuffer, 10);
	Soft_UART_Tx_Message(ToABuffer);
	
	Soft_UART_Tx_Byte(':');
	ultoa(GetDosePeriodSecs(), ToABuffer, 10);
	Soft_UART_Tx_Message(ToABuffer);                  
	
	Soft_UART_Tx_Byte(':');
	ultoa(GetCurrentTimeAbs(), ToABuffer, 10);
	Soft_UART_Tx_Message(ToABuffer);
	
	Soft_UART_Tx_Byte(':');
	ultoa(GetDoseReadyTime(), ToABuffer, 10);
	Soft_UART_Tx_Message(ToABuffer);
	
	Soft_UART_Tx_Byte(':');
	ultoa(GetDoseExpireTime(), ToABuffer, 10);
	Soft_UART_Tx_Message(ToABuffer);
	TX_NEWLINE;
}

static void Erase_EEPROM(void)
{
	uint16_t i, BatteryDateAddress;

	BatteryDateAddress = ((uint16_t)&EEBatStartDate) & 0x01FF;
	if(BatteryDateAddress)
	{
		for(i=0; i<BatteryDateAddress; i++)
			eeprom_write_byte((uint8_t *)i, 0x00);
	}
	for(i=BatteryDateAddress+PCF85063_TIME_LENGTH; i<EEPROM_SIZE; i++)	
		eeprom_write_byte((uint8_t *)i, 0x00);

	ACK();TX_NEWLINE;
	// software reset
	wdt_enable(WDTO_15MS);
	for(;;)
		;
}
/*
static void Com_RAM_Dump(void)
{
	uint8_t* i = 0;
	uint16_t j;
	uint8_t TempU8;
	char ToABuffer[UCHAR_2_A_SIZE];
	
	Soft_UART_Tx_Const_Message(PSTR("RAM DUMP"));
	TX_NEWLINE;
	for(j=0;j<=RAMEND;j++)
	{
		if(!(j % 16))
		{
			utoa(j, ToABuffer, 16);
			if(j < 0x10)
				Soft_UART_Tx_Byte('0');
			Soft_UART_Tx_Message(ToABuffer);
			Soft_UART_Tx_Byte(':');
		}
		TempU8 = *i++;
		utoa(TempU8, ToABuffer, 16);
		if(TempU8 < 0x10)
			Soft_UART_Tx_Byte('0');
		Soft_UART_Tx_Message(ToABuffer);
		Soft_UART_Tx_Byte(' ');
		if(!((j+1) % 16))
			TX_NEWLINE;
	}
}

static void Com_EE_Dump(void)
{
	uint16_t i = 0;
	uint8_t TempU8;
	char ToABuffer[UCHAR_2_A_SIZE];
	
	Soft_UART_Tx_Const_Message(PSTR("EEPROM DUMP"));
	TX_NEWLINE;

	for(i=0;i<=E2END;i++)
	{
		if(!(i % 16))
		{
			utoa(i, ToABuffer, 16);
			if(i < 0x10)
				Soft_UART_Tx_Byte('0');
			Soft_UART_Tx_Message(ToABuffer);
			Soft_UART_Tx_Byte(':');
		}
		TempU8 = eeprom_read_byte((uint8_t*)i);
		utoa(TempU8, ToABuffer, 16);
		if(TempU8 < 0x10)
			Soft_UART_Tx_Byte('0');	
		Soft_UART_Tx_Message(ToABuffer);
		Soft_UART_Tx_Byte(' ');
		if(!((i+1) % 16))
			TX_NEWLINE;
	}
}*/

static void Com_Output_Data(void)
{
	uint16_t i = 0;
	uint8_t j;
	uint8_t TempByte;
	char ToABuffer[UCHAR_2_A_SIZE];
	
	Soft_UART_Tx_Const_Message(PSTR("D:"));
	utoa(GetCurrentDose(), ToABuffer, 10);
	Soft_UART_Tx_Message(ToABuffer);
	TX_NEWLINE;

	while(i < GetCurrentDose()*4)
	{
		for(j=0;j<4;j++)
		{
			if(i < EE_DATA_STORAGE_LENGTH)
				TempByte = eeprom_read_byte(EEDataStorage+i+j);
			else
				TempByte = RAMDataStorage[i-EE_DATA_STORAGE_LENGTH+j];
			utoa(TempByte, ToABuffer, 16);
			Soft_UART_Tx_Message(ToABuffer);
			Soft_UART_Tx_Byte(':');
		}
		i += 4;
		TX_NEWLINE;
	}
}

void Com_Get_Parameter(char *RxdString)
{
	char StringBuffer[EE_STRING_LENGTH];
	
	Soft_UART_Tx_Const_Message(PSTR("G:"));
	Soft_UART_Tx_Byte(*RxdString);
	Soft_UART_Tx_Byte(':');
	
	switch(RxdString[0])
	{
		case '0': eeprom_read_block(StringBuffer, &EEFacility,	EE_STRING_LENGTH);	break;
		case '1': eeprom_read_block(StringBuffer, &EEDoctor,	EE_STRING_LENGTH);	break;
		case '2': eeprom_read_block(StringBuffer, &EETreatment,	EE_STRING_LENGTH);	break;
		case '3': eeprom_read_block(StringBuffer, &EEClient,	EE_STRING_LENGTH);	break;
		case '4': eeprom_read_block(StringBuffer, &EEPatient,	EE_STRING_LENGTH);	break;
		case '5': itoa(eeprom_read_word(&EEDosePeriodHours), StringBuffer, 10); break;
		case '6': itoa(GetTotalDoses(), StringBuffer, 10); break;
		default	: StringBuffer[0] = 0; break;
	}
	StringBuffer[EE_STRING_LENGTH-1] = 0; // NULL terminate just in case
	Soft_UART_Tx_Message(StringBuffer); 
	TX_NEWLINE;
}

void Com_Set_Parameter(char *RxdString)
{
	uint8_t TempU8;
	uint16_t TempU16;

	TempU8 = strlen(RxdString+1);
	if(TempU8 > EE_STRING_LENGTH)
		TempU8 = EE_STRING_LENGTH;

	switch(RxdString[0])
	{
		case '0': eeprom_write_block((RxdString+1), &EEFacility, TempU8); break;
		case '1': eeprom_write_block((RxdString+1), &EEDoctor, TempU8); break;
		case '2': eeprom_write_block((RxdString+1), &EETreatment, TempU8); break;
		case '3': eeprom_write_block((RxdString+1), &EEClient, TempU8); break;
		case '4': eeprom_write_block((RxdString+1), &EEPatient, TempU8); break;
		case '5': 
				TempU16 = (uint16_t)atoi(RxdString+1); 
				eeprom_write_word(&EEDosePeriodHours, TempU16);	// Stored in EEPROM in hours
				SetDosePeriodSecs(3600ul * TempU16);			// Stored in RAM in seconds
		break;
		case '6': SetTotalDoses((uint8_t)atoi(RxdString+1)); break;
		default: NACKU(); TX_NEWLINE; break;
	}
}

uint8_t BCD2Hex(const uint8_t BCDHexValue)
{
	return (BCDHexValue/16) * 10 + (BCDHexValue & 0x0F);
}

/*************************************************************//**
\brief Parse and respond to commands from the com port
First character containing a number or letter (all letters made CAP) denotes the command
checks to see if command has been enabled
\param ParseString string containing command to be parsed
\return	none
***************************************************************/
void Com_Parse_String(char* ParseString)
{
	char ToABuffer[UINT_2_A_SIZE];
	char StringBuffer[RX0_BUFFER_SIZE];
	uint8_t i, TempU8;
	uint32_t TempU32;

	ComTimer = COM_TIMER_RELOAD_60_SECS; // received complete command, reload timer
	
	switch(toupper(ParseString[0]))
	{
//		case '0': break;
//		case '1': break;
//		case '2': break;
//		case '3': break;
//		case '4': break;
//		case '5': break;
//		case '6': break;
//		case '7': break;
//		case '8': break;
//		case '9': break;
		case 'A': 
			if(ParseString[1] == 'R') 
			{
				TempU32 = GetDosePeriodSecs();
				SetDosePeriodSecs(TempU32/3600);
				CalcNextDoseTimes();
				ACK();
			} 
			else
				NACKB();
		break;
		case 'B': 
			for(i=0; i<PCF85063_TIME_LENGTH; i++)
				StringBuffer[i] = (ParseString[3*i + 2]-0x30)*16 + ParseString[3*i + 3]-0x30;
			PCF85063_Write(PCF85063_ADDR_SECONDS, PCF85063_TIME_LENGTH, (uint8_t*)StringBuffer); // write time
			ACK();
			TX_NEWLINE;
		break;
		case 'C': 
			PCF85063_Read(PCF85063_ADDR_SECONDS, PCF85063_TIME_LENGTH, (uint8_t*)StringBuffer); // read time
			Soft_UART_Tx_Byte('C');
			for(i=0; i<PCF85063_TIME_LENGTH; i++)
			{
				TempU8 = BCD2Hex(StringBuffer[i]);
				Soft_UART_Tx_Byte(':');
				itoa(TempU8, ToABuffer, 10);
				if(TempU8 < 10)
					Soft_UART_Tx_Byte('0');
				Soft_UART_Tx_Message(ToABuffer);
			}
			TX_NEWLINE;
		break;
		case 'D': Com_Output_Data(); break;
		case 'E': if(ParseString[1] == 'R') Erase_EEPROM(); else {NACKB();} break;
		case 'F': // first dose time
			for(i=0; i<PCF85063_TIME_LENGTH; i++)
				StringBuffer[i] = (ParseString[3*i + 2]-0x30)*16 + ParseString[3*i + 3]-0x30;
			SetFirstDoseTime((uint8_t*)StringBuffer); // write time
			ACK();
			TX_NEWLINE;
		break;
		case 'G': Com_Get_Parameter(ParseString+1); break;
		case 'H': 
			GetFirstDoseTime((uint8_t*)StringBuffer); // read time
			Soft_UART_Tx_Byte('H');
			for(i=0; i<PCF85063_TIME_LENGTH; i++)
			{
				TempU8 = BCD2Hex(StringBuffer[i]);
				Soft_UART_Tx_Byte(':');
				itoa(TempU8, ToABuffer, 10);
				if(TempU8 < 10)
					Soft_UART_Tx_Byte('0');
				Soft_UART_Tx_Message(ToABuffer);
			}
			TX_NEWLINE;
		break;
		case 'I': StartTreatment(); break;
//		case 'J': break;
//		case 'K': break;
//		case 'L': break;
		case 'M':  
			eeprom_read_block(StringBuffer, &EEBatStartDate, PCF85063_TIME_LENGTH);	
			Soft_UART_Tx_Byte('M');
			for(i=0; i<PCF85063_TIME_LENGTH; i++)
			{
				TempU8 = BCD2Hex(StringBuffer[i]);
				Soft_UART_Tx_Byte(':');
				itoa(TempU8, ToABuffer, 10);
				if(TempU8 < 10)
					Soft_UART_Tx_Byte('0');
				Soft_UART_Tx_Message(ToABuffer);
			}
			TX_NEWLINE;
		break;
		case 'N': 
			PCF85063_Read(PCF85063_ADDR_SECONDS, PCF85063_TIME_LENGTH, (uint8_t*)StringBuffer); // read time
			StringBuffer[0] &= 0x7F;	// clear OS bit
			eeprom_write_block(StringBuffer, &EEBatStartDate, PCF85063_TIME_LENGTH);
			ACK();TX_NEWLINE;
		break;
//		case 'O': ACK(); break;
		case 'P': Com_Tx_Status(); break;
//		case 'Q': break;
/*		case 'R': 
			PCF85063_Read(PCF85063_CONTROL_1, 18, (uint8_t*)StringBuffer); // read time
			Soft_UART_Tx_Byte('R');
			for(i=0; i<18; i++)
			{
				Soft_UART_Tx_Byte(':');
				itoa(StringBuffer[i], ToABuffer, 16);
				if(StringBuffer[i] < 0x10)
					Soft_UART_Tx_Byte('0');
				Soft_UART_Tx_Message(ToABuffer);
			}
		break;	*/	
		case 'S': Com_Set_Parameter(ParseString+1); break;
		case 'T': Com_Tx_Title(); break;
//		case 'U': Com_RAM_Dump(); break;
//		case 'V': Com_EE_Dump(); break;
/*		case 'W': // fill data
			SetTotalDoses((EE_DATA_STORAGE_LENGTH+RAM_DATA_STORAGE_LENGTH)/4);
			for(TempU32 = 0; TempU32<(EE_DATA_STORAGE_LENGTH+RAM_DATA_STORAGE_LENGTH)/4; TempU32++)
				Dose_Record((uint16_t)TempU32, 3, 2, 1);
			ACK(); TX_NEWLINE;
		break;*/
//		case 'X': Tasks |= DO_RTC_TASK; break;
//		case 'Y': Power_Down(); break;
//		case 'Z': Soft_UART_Tx_Const_Message(PSTR("Z:BaseStation:0.00\r\n")); 
		break;
		default: NACKU(); TX_NEWLINE; break;
	}
}
