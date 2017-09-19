#ifndef _POWER_H_
#define _POWER_H_

#include <avr/eeprom.h>
#include "DoseClock.h"

extern uint16_t DataStorageCounter;
#define COM_TIMER_RELOAD_60_SECS	60u
extern uint8_t ComTimer;
extern uint8_t IgnoreButton;

// Time Stamp Size 4, number of time points to store 97, total space 388
#define EE_DOSE_STORAGE_COUNT	99u
#define DOSE_TIMESTAMP_SIZE		4u
#define EE_DATA_STORAGE_LENGTH	(EE_DOSE_STORAGE_COUNT * DOSE_TIMESTAMP_SIZE)
extern EEMEM uint8_t EEDataStorage[EE_DATA_STORAGE_LENGTH];
#define RAM_DOSE_STORAGE_COUNT	21u
#define RAM_DATA_STORAGE_LENGTH	(RAM_DOSE_STORAGE_COUNT * DOSE_TIMESTAMP_SIZE)
extern uint8_t RAMDataStorage[RAM_DATA_STORAGE_LENGTH];

void Dose_Record(uint8_t Min, uint8_t Hr, uint8_t Day, uint8_t Mon);
void RTC_Task(void);
void Button_Task(void);
void Power_Down(void);

#endif