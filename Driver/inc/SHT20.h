
#ifndef __SHT20_H__
#define  __SHT20_H__

#include "GlobalDef.h"


extern void  TempHumi_SetSensorExisted(uint8_t state);

void SHT20_Init(void);



void SHT20_RegConfig(uint8_t precision_mask, uint8_t is_heated);
void SHT20_ClosePower(void);



#endif

