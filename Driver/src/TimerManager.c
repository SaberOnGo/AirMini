
#include "TimerManager.h"
#include "stm32f10x.h"
#include "stm32f10x_it.h"

static volatile uint32_t sSysTick = 0;
volatile uint8_t flag10ms = 0;
	
void SysTick_Init(void)
{
    if(SysTick_Config(SystemCoreClock / 100))  // 10 ms ÷–∂œ“ª¥Œ
    {
        while(1);
    }
}

void SysTick_Increment(void)
{
   sSysTick++;
   flag10ms = 1;
}

uint32_t OS_GetSysTick(void)
{
   return sSysTick;
}
