
#include <string.h>
#include "PCF85063.h"
#include "SoftMicrowire.h"

#define READ_NOT_WRITE	0x80

void PCF85063_Setup(void)
{
	PCF85063_CS_PORT |= PCF85063_CS_PIN;
	Soft_Microwire_Write_Byte(PCF85063_ADDR_CONTROL_1);	// Address
	Soft_Microwire_Write_Byte(0x01);				// Control_1: 12.5 pf Caps, 24HR, 
	Soft_Microwire_Write_Byte(0x0F);				// Control_2: No Alarms or Min INT, No ClkOut
	PCF85063_CS_PORT &= ~PCF85063_CS_PIN;

	PCF85063_CS_PORT |= PCF85063_CS_PIN;
	Soft_Microwire_Write_Byte(PCF85063_ADDR_TIMER_VALUE);// Address
	Soft_Microwire_Write_Byte(PCF85063_INTERVAL_SECS);// Timer Interrupt in seconds
	Soft_Microwire_Write_Byte(0x17);				// Timer Mode: Second counter, enabled, INT enabled, Generates pulse
	PCF85063_CS_PORT &= ~PCF85063_CS_PIN;
}

void PCF85063_Write(PCF85063_register_t Address, uint8_t Length, uint8_t* DataArray)
{
	uint8_t i;

	PCF85063_CS_PORT |= PCF85063_CS_PIN;
	Soft_Microwire_Write_Byte(Address);	// Address
	for(i=0; i<Length; i++)
		Soft_Microwire_Write_Byte(DataArray[i]);				
	PCF85063_CS_PORT &= ~PCF85063_CS_PIN;
}

void PCF85063_Read(PCF85063_register_t Address, uint8_t Length, uint8_t* DataArray)
{
	uint8_t i;

	PCF85063_CS_PORT |= PCF85063_CS_PIN;
	Soft_Microwire_Write_Byte(READ_NOT_WRITE|Address);	// Address
	for(i=0; i<Length; i++)
		DataArray[i] = Soft_Microwire_Read_Byte();
	PCF85063_CS_PORT &= ~PCF85063_CS_PIN;
	DataArray[0] &= 0x7F;	// clear OS bit
}

void PCF85063_Write_Byte(PCF85063_register_t Address, uint8_t DataByte)
{
	PCF85063_CS_PORT |= PCF85063_CS_PIN;
	Soft_Microwire_Write_Byte(Address);	// Address
	Soft_Microwire_Write_Byte(DataByte);// Data
	PCF85063_CS_PORT &= ~PCF85063_CS_PIN;
}

uint8_t PCF85063_Read_Byte(PCF85063_register_t Address)
{
	uint8_t RetVal;
	
	PCF85063_CS_PORT |= PCF85063_CS_PIN;
	Soft_Microwire_Write_Byte(READ_NOT_WRITE|Address);	// Address	
	RetVal = Soft_Microwire_Read_Byte();
	PCF85063_CS_PORT &= ~PCF85063_CS_PIN;
	
	return RetVal;
}
