#ifndef _CLOCK_H_
#define _CLOCK_H_

extern uint8_t TreatmentStarted;
uint8_t GetTotalDoses(void);
uint8_t GetDosesRemaining(void);
uint8_t GetCurrentDose(void);
void SetTotalDoses(uint8_t);

uint32_t GetDoseExpireTime(void);
uint32_t GetDoseReadyTime(void);

void SetDosePeriodSecs(uint32_t);
uint32_t GetDosePeriodSecs(void);

void SetDoseTaken(void);
uint8_t DoseReady(void);
uint8_t DoseMissed(void);

uint32_t CalcAbsoluteTime(uint8_t* TimeArray);
void CalcNextDoseTimes(void);
	
void SetFirstDoseTime(uint8_t*);
void GetFirstDoseTime(uint8_t* TimeArray);
uint32_t GetFirstDoseTimeAbs(void);

void UpdateCurrentTime(void);
void GetCurrentTime(uint8_t*);
uint32_t GetCurrentTimeAbs(void);
void StartTreatment(void);
#endif //_CLOCK_H_