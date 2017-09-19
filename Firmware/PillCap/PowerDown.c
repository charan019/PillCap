#include <avr/io.h>
#include <avr/interrupt.h>#include <avr/pgmspace.h>#include <avr/eeprom.h>#include <avr/sleep.h>#include <avr/power.h>
#include "PillCap.h"
#include "PCF85063.h"
#include "PowerDown.h"
#include "SoftUART.h"
#include "DoseClock.h"

EEMEM uint8_t EEDataStorage[EE_DATA_STORAGE_LENGTH];
uint8_t RAMDataStorage[RAM_DATA_STORAGE_LENGTH];
static volatile uint8_t LEDTimer = 0;
uint8_t IgnoreButton;
uint8_t ComTimer = COM_TIMER_RELOAD_60_SECS;

void Timer0_Init(void);

// Either dose taken or time expired
// record and setup to wait for next time period
void Dose_Record(uint8_t Min, uint8_t Hr, uint8_t Day, uint8_t Mon)
{
	uint16_t DataDestination;

	DataDestination = GetCurrentDose()*4;
	if(DataDestination < EE_DATA_STORAGE_LENGTH)	// store minutes, hours, months, years
	{
		eeprom_write_byte((uint8_t *)(&EEDataStorage)+DataDestination, Min);
		eeprom_write_byte((uint8_t *)(&EEDataStorage)+DataDestination+1, Hr);
		eeprom_write_byte((uint8_t *)(&EEDataStorage)+DataDestination+2, Day);
		eeprom_write_byte((uint8_t *)(&EEDataStorage)+DataDestination+3, Mon);
	}
	else
	{
		RAMDataStorage[DataDestination-EE_DATA_STORAGE_LENGTH] = Min;
		RAMDataStorage[DataDestination-EE_DATA_STORAGE_LENGTH+1] = Hr;
		RAMDataStorage[DataDestination-EE_DATA_STORAGE_LENGTH+2] = Day;
		RAMDataStorage[DataDestination-EE_DATA_STORAGE_LENGTH+3] = Mon;
	}
	SetDoseTaken();		// update counter and calculate new time periods
	IgnoreButton = TRUE;
}

//	Set LEDs as appropriate
//	Setup LED timer
//	Read RTC Status and time
//	Enable/Disable Dose Button response
void RTC_Task(void)
{
	uint8_t TimeArray[PCF85063_TIME_LENGTH];

	Tasks &= ~DO_RTC_TASK;
	if(ComTimer)
		ComTimer--;

	// Setup LEDs and LED timer, will be one period behind, but that's OK
	// if dose has been set
	//		if no doses left
	//			RED
	//		else if between dose ready and dose ready + 1/3 dose time period
	//			if 1 dose left
	//				YELLOW
	//			else
	//				GREEN
	if(GetTotalDoses() && TreatmentStarted)
	{
		if(!GetDosesRemaining())
			LED_PORT &= ~LED_RED;
		else if(!IgnoreButton)
		{
			if(GetDosesRemaining() == 1)
				LED_PORT &= ~LED_YELLOW;
			else
				LED_PORT &= ~LED_GREEN;
		}
		Timer0_Init();				// setup time for LED on

		UpdateCurrentTime();		// read from RTC

		// ignore button if outside of dose compliance times
		// or if dose taken this time period
		if(DoseReady())
			IgnoreButton = FALSE;
		if(DoseMissed())
		{
			GetCurrentTime(TimeArray);
			Dose_Record(0,0,0,0);// record as missed dose
		}
	}
	else
		Tasks |= DO_POWERDOWN_TASK;	// if no doses no need to track, just power down
}

// Button Task
// reset com timer
// Save push time if not too soon
void Button_Task(void)
{
	uint8_t TimeArray[PCF85063_TIME_LENGTH];

	Tasks &= ~DO_BUTTON_TASK;
	Tasks |= DO_POWERDOWN_TASK;

	ComTimer = COM_TIMER_RELOAD_60_SECS;

	if(!IgnoreButton)
	{
		GetCurrentTime(TimeArray);
		Dose_Record(TimeArray[1], TimeArray[2], TimeArray[3], TimeArray[5]);
	}
}

// Things that can wake controller up
// Button, Command, RTC
// Deal with Button and RTC here
void Power_Down(void)
{
	Tasks &= ~DO_POWERDOWN_TASK;

	// disable UART Rx pin if timer expired
	if(ComTimer)
		PORTA |= UART_SOFT_RX_PIN;
	else
		PORTA &= ~UART_SOFT_RX_PIN;

	// wait for button not pushed and RTC timer trigger over
	while(!(PINA & 0x02))
		;
	while(!(PINB & 0x04))
		;

	GIFR = 0x10;	// clear int flag
	PCMSK0 = 0x22;	// Enable PCINT5 for PA5, UART Rx and PCINT1 for PA1, RTC
	GIMSK  = 0x50;	// enable PCIE0 interrupts 0-7 and INT0

	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_mode();
}

// External Interrupt Request 0, Button
// Disable Interrupt
// Schedule Button Task
ISR(EXT_INT0_vect)
{
	GIMSK &= ~0x40;				// disable INTs
	Tasks |= DO_BUTTON_TASK;
}

//TIMER0 initialize - prescale:8
// WGM: Normal, desired value: 20 mSec, actual value: 20.0 mSec
void Timer0_Init(void)
{
	PRR &= ~0x04;
	TIMSK0 = 0x01;		// enable OVF interrupt
	TIFR0  = 0x00;
	TCCR0A = 0x00;
	TCCR0B = 0x03;		// div by 64 pre-scalar
	TCNT0  = 0xd9;		// reload value
}

//  Timer/Counter0 Overflow
ISR(TIM0_OVF_vect)
{
	TCCR0B = 0x00;	// timer off
	TIMSK0 = 0x00;	// disable interrupt
	PRR |= 0x04;	// power down timer
	LED_PORT |= LED_RED | LED_YELLOW | LED_GREEN;	// All LEDs off
	Tasks |= DO_POWERDOWN_TASK;
}
