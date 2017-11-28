
#ifndef __BATTERY_LEVEL_H__
#define  __BATTERY_LEVEL_H__

#include "GlobalDef.h"

extern uint8_t bat_lev_percent;

void BatLev_Init(void);
uint8_t BatteryIsCharging(void);
extern E_BOOL is_5v_power_close;
void BatLev_ClosePower(uint8_t pwr_close);
void BatLev_OpenPower(void);


#endif

