
#ifndef __TIME_MANAGER_H__
#define  __TIME_MANAGER_H__

#include "stm32f10x.h"
#include <stdint.h>

#define SysTick_Open()  SysTick->CTRL |=  (SysTick_CTRL_ENABLE_Msk)
#define SysTick_Close() SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk)

extern volatile uint8_t flag10ms;

void SysTick_Increment(void);
void SysTick_Init(void);
uint32_t OS_GetSysTick(void);

#endif

