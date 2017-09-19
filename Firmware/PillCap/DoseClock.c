#include <stdint.h>
#include "PillCap.h"
#include "PCF85063.h"
#include "PowerDown.h"

uint8_t TreatmentStarted = FALSE;
static uint32_t DosePeriodSecs;
EEMEM uint8_t EEDoseCounter = 0;	// number of dose time periods passed
EEMEM uint8_t EEDoseCount = 0;		// number of total dose time periods
uint32_t DoseExpireTimeAbs, DoseReadyTimeAbs;

uint8_t TIME_ZERO[PCF85063_TIME_LENGTH] = {0,0,0,1,0,1,14};
uint8_t CurrentTime[PCF85063_TIME_LENGTH];
uint32_t CurrentTimeAbs;
uint8_t FirstDoseTime[PCF85063_TIME_LENGTH];
uint32_t FirstDoseTimeAbs = 0;

// Calculate seconds since Jan 1 2014, 00:00:00
uint32_t CalcAbsoluteTime(uint8_t* TimeArray)
{
	uint8_t i;
	uint8_t TimeDifferenceArray[PCF85063_TIME_LENGTH];
	uint32_t RetVal;
	
	for(i=0; i<PCF85063_TIME_LENGTH; i++)
		TimeDifferenceArray[i] = (TimeArray[i]/16) * 10 + (TimeArray[i] & 0x0F) - TIME_ZERO[i];

	RetVal = (uint32_t)TimeDifferenceArray[6];		// years
	RetVal = RetVal * 12ul + (uint32_t)TimeDifferenceArray[5];	// months per year + months
	RetVal = RetVal * 30ul + (uint32_t)TimeDifferenceArray[3];	// days per month + days
	RetVal = RetVal * 24ul + (uint32_t)TimeDifferenceArray[2];	// hours per day + hours
	RetVal = RetVal * 60ul + (uint32_t)TimeDifferenceArray[1];	// minutes per hour + minutes
	RetVal = RetVal * 60ul + (uint32_t)TimeDifferenceArray[0];	// seconds per minute + second

	return RetVal;
}

void SetDosePeriodSecs(uint32_t DosePeriodSet)
{
	DosePeriodSecs = DosePeriodSet;
}

uint32_t GetDosePeriodSecs(void)
{
	return DosePeriodSecs;
}

void CalcNextDoseTimes(void)
{
	DoseReadyTimeAbs = FirstDoseTimeAbs + eeprom_read_byte(&EEDoseCounter)*DosePeriodSecs;
	DoseExpireTimeAbs = DoseReadyTimeAbs + (DosePeriodSecs*85)/256;	
}

// mark as taken, update counter and calculate next dose times
void SetDoseTaken(void)
{
	uint8_t TempU8;
	
	TempU8 = eeprom_read_byte(&EEDoseCounter);
	if(TempU8 < eeprom_read_byte(&EEDoseCount))
		eeprom_write_byte(&EEDoseCounter, TempU8+1);
	CalcNextDoseTimes();
}

uint32_t GetDoseExpireTime(void)
{
	return DoseExpireTimeAbs;
}

uint32_t GetDoseReadyTime(void)
{
	return DoseReadyTimeAbs;
}

// Check to see if current absolute time is between dose available time and expiration time
// and dose not taken
uint8_t DoseReady(void)
{
	if(CurrentTimeAbs > DoseReadyTimeAbs)
		return TRUE;
	else
		return FALSE;
}

uint8_t DoseMissed(void)
{
	if(CurrentTimeAbs > DoseExpireTimeAbs)
		return TRUE;
	else
		return FALSE;
}

uint8_t GetTotalDoses(void)
{
	return eeprom_read_byte(&EEDoseCount);
}

uint8_t GetCurrentDose(void)
{
	return eeprom_read_byte(&EEDoseCounter);
}

uint8_t GetDosesRemaining(void)
{
	return eeprom_read_byte(&EEDoseCount) - eeprom_read_byte(&EEDoseCounter);
}

void SetTotalDoses(uint8_t SetDoses)
{
	eeprom_write_byte(&EEDoseCount, SetDoses);
	eeprom_write_byte(&EEDoseCounter, 0);
}

void SetFirstDoseTime(uint8_t* DoseTime)
{
	uint8_t i;
	for(i=0; i<PCF85063_TIME_LENGTH; i++)
		FirstDoseTime[i] = DoseTime[i];
	FirstDoseTimeAbs = CalcAbsoluteTime(DoseTime);
}

void GetFirstDoseTime(uint8_t* TimeArray)
{
	uint8_t i;
	
	for(i=0; i<PCF85063_TIME_LENGTH; i++)
		TimeArray[i] = FirstDoseTime[i];
}

uint32_t GetFirstDoseTimeAbs(void)
{
	return FirstDoseTimeAbs;
}

// Get current time in seconds
void UpdateCurrentTime(void)
{
	PCF85063_Read(PCF85063_ADDR_SECONDS, PCF85063_TIME_LENGTH, CurrentTime); // read all time
	CurrentTimeAbs = CalcAbsoluteTime(CurrentTime);
}

void GetCurrentTime(uint8_t* TimeArray)
{
	uint8_t i;
	
	for(i=0; i<PCF85063_TIME_LENGTH; i++)
		TimeArray[i] = CurrentTime[i];
}

uint32_t GetCurrentTimeAbs(void)
{
	return CurrentTimeAbs;
}

void StartTreatment(void)
{
	IgnoreButton = TRUE;
	TreatmentStarted = TRUE;
	CalcNextDoseTimes();	
}