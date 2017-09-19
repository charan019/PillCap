/*! \file****************************************************************
 	PillCap.c
		\brief Pill cap program.  Very fancy timer.
 Target : ATTiny84A
 Crystal: 125 KHz

Program Memory Usage:	5392 bytes	65.8 % Full
Data Memory Usage:		206 bytes	40.2 % Full
EEPROM Memory Usage:	512 bytes	100.0 % Full

Fuses: Defaults are correct
Extended:	FE
High:		DF
Low:		62

 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include "PillCap.h"
#include "SoftUART.h"
#include "PCF85063.h"
#include "PowerDown.h"

volatile uint8_t Tasks;

int main(void)
{
	clock_prescale_set(clock_div_64);	// SLOW DOWN, 8MHz/64 = 125 kHz
	cli();			// disable all interrupts
	DDRA   = 0x5C;
	PORTA  = 0x7A;
	DDRB   = 0x03;
	PORTB  = 0x04;
	MCUCR  = 0x00;	// Low level INT0, Button
	DIDR0  = 0x80;	// disable digital input on analog input pin
	PRR    = 0x0F;	// power down T0, T1, ADC, and USI
	sei();			// re-enable interrupts

	PCF85063_Setup();
	Power_Down();

    while(1)
    {
        if(Tasks & DO_COM_RX)				Soft_UART_Rx_Task();
		else if(Tasks & DO_BUTTON_TASK)		Button_Task();
        else if(Tasks & DO_RTC_TASK)		RTC_Task();
		else if(Tasks & DO_POWERDOWN_TASK)	Power_Down();
    }
}

// in case of software reset turn off wdt during initialization
void get_mcusr(void) __attribute__((naked)) __attribute__((section(".init3")));
void get_mcusr(void)
{
	MCUSR = 0;
	wdt_disable();
}
