#include <stdint.h>
#include <avr/io.h>
#include "SoftMicrowire.h"

void Soft_Microwire_Write_Byte(const uint8_t WriteByte)
{
	uint8_t i;
	uint8_t MaskBit = 0x80;

	SOFT_MIROWIRE_DATA_DDR |= SOFT_MIROWIRE_DATA_PIN;	// set to output

	/* transmit MSb first, sample at clock rising edge */
	for(i=0; i<8; i++)
	{	
		SOFT_MIROWIRE_CLK_PORT &= ~SOFT_MIROWIRE_CLK_PIN;
		if(WriteByte & MaskBit)
			SOFT_MIROWIRE_DATA_WRITE_PORT |= SOFT_MIROWIRE_DATA_PIN;
		else
			SOFT_MIROWIRE_DATA_WRITE_PORT &= ~SOFT_MIROWIRE_DATA_PIN;
		SOFT_MIROWIRE_CLK_PORT |= SOFT_MIROWIRE_CLK_PIN;
		MaskBit >>= 1;
	}
	SOFT_MIROWIRE_CLK_PORT &= ~SOFT_MIROWIRE_CLK_PIN;
}

uint8_t Soft_Microwire_Read_Byte(void)
{
	uint8_t i;
	uint8_t ReturnByte = 0;
	uint8_t MaskBit = 0x80;

	SOFT_MIROWIRE_DATA_DDR &= ~SOFT_MIROWIRE_DATA_PIN;	// set to input

	/* receive MSb first, sample at clock rising edge */
	for(i=0; i<8; i++)
	{
		SOFT_MIROWIRE_CLK_PORT &= ~SOFT_MIROWIRE_CLK_PIN;
		SOFT_MIROWIRE_CLK_PORT |= SOFT_MIROWIRE_CLK_PIN;

		if(SOFT_MIROWIRE_DATA_READ_PORT & SOFT_MIROWIRE_DATA_PIN)
			ReturnByte |= MaskBit;
		MaskBit >>= 1;
	}

	SOFT_MIROWIRE_CLK_PORT &= ~SOFT_MIROWIRE_CLK_PIN;
	return ReturnByte;
}
