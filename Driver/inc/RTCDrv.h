
#ifndef __RTC_DRV_H__
#define  __RTC_DRV_H__

#include "GlobalDef.h"

//时间结构体
typedef struct 
{
	vu8 hour;
	vu8 min;
	vu8 sec;
	
	//公历日月年周
	vu16 year;
	vu8  month;
	vu8  day;
	vu8  week;		 
}T_Calendar_Obj;		

extern T_Calendar_Obj calendar;

void    RTCDrv_Init(void);
uint8_t RTCDrv_GetTime(T_Calendar_Obj * cal);
uint8_t RTCDrv_SetAlarm(uint16_t syear, uint8_t smon, uint8_t sday, uint8_t hour, uint8_t min, uint8_t sec);
uint8_t RTCDrv_SetTime(uint16_t syear,uint8_t smon, uint8_t sday, uint8_t hour, uint8_t min, uint8_t sec);

extern void TIME_SetSensorExisted(uint8_t state);

#endif

