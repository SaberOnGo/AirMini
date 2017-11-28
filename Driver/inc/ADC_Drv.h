
#ifndef __ADC_DRV_H__
#define  __ADC_DRV_H__


#include "GlobalDef.h"



void ADCDrv_Start(void);
void ADCDrv_StartBatteryMeasure(void  (*end_exe_func)(uint16_t arg));


#endif

