
#ifndef __LED_DRV_H__
#define  __LED_DRV_H__

#include "stm32f10x.h"
#include "GlobalDef.h"


typedef enum
{
    LED_GREEN = 0,
	LED_YELLOW,
	LED_RED,
}E_LED_SELECT;

typedef enum
{
    LED_CLOSE = 0,
    LED_OPEN = 1,
}E_LED_STATE;

#define LED_GREEN_PORT         GPIOC
#define LED_GREEN_GPIO_Pin     GPIO_Pin_15
#define LED_GREEN_Set(val)    GPIO_WriteBit(LED_GREEN_PORT, LED_GREEN_GPIO_Pin,\
	(val == LED_OPEN)? Bit_SET: Bit_RESET)


#define LED_YELLOW_PORT        GPIOD
#define LED_YELLOW_GPIO_Pin    GPIO_Pin_0
#define LED_YELLOW_Set(val)       GPIO_WriteBit(LED_YELLOW_PORT, LED_YELLOW_GPIO_Pin,\
	(val == LED_OPEN)? Bit_SET: Bit_RESET)


#define LED_RED_PORT           GPIOD
#define LED_RED_GPIO_Pin       GPIO_Pin_1
#define LED_RED_Set(val)       GPIO_WriteBit(LED_RED_PORT, LED_RED_GPIO_Pin,\
	(val == LED_OPEN)? Bit_SET: Bit_RESET)

// 对应 PM25 的LED等级, 7级
typedef enum
{
   AQI_GOOD = 0,           // 优,      绿灯
   AQI_Moderate = 1,       // 中等,    绿黄灯
   AQI_LightUnhealthy = 2, // 轻度污染, 黄灯
   AQI_MidUnhealthy = 3,   // 中度污染, 黄灯
   AQI_VeryUnhealthy = 4,  // 重度污染, 红灯
   AQI_JustInHell    = 5,  // 刚进入地狱,   黄 红灯慢闪
   AQI_HeavyInHell   = 6,  // 深度地狱模式, 3灯狂闪
   AQI_LEVEL_END,
}E_AQI_LEVEL;

void LED_AllSet(E_LED_STATE state);



void LED_Init(void);
void LED_Config(E_LED_SELECT led, E_LED_STATE state);

void LED_AqiIndicate(E_AQI_LEVEL level);
void LED_StopAqiIndicate(void);
void LED_Flash(uint16_t times);
void LEDTest(void);

#define LED_AllClose() LED_AllSet(LED_CLOSE)
#define LED_AllOpen()  LED_AllSet(LED_OPEN)


#endif

