
#ifndef __USART_DRV_H__
#define  __USART_DRV_H__

#include <stdint.h>

#define USART_WIFI               USART1
#define USART_WIFI_GPIO          GPIOA
#define USART_WIFI_CLK           RCC_APB2Periph_USART1
#define USART_WIFI_GPIO_CLK      RCC_APB2Periph_GPIOA
#define USART_WIFI_RxPin         GPIO_Pin_10
#define USART_WIFI_TxPin         GPIO_Pin_9
#define USART_WIFI_IRQn          USART1_IRQn
#define USART_WIFI_IRQHandler    USART1_IRQHandler
#define RCC_WIFI_APBPeriphClockCmd_Init()   {\
 	RCC_APB2PeriphClockCmd(USART_WIFI_GPIO_CLK, ENABLE);\
	RCC_APB2PeriphClockCmd(USART_WIFI_CLK, ENABLE);\
	}

#define USART_PM25                   USART2
#define USART_PM25_GPIO              GPIOA
#define USART_PM25_CLK               RCC_APB1Periph_USART2
#define USART_PM25_GPIO_CLK          RCC_APB2Periph_GPIOA
#define USART_PM25_RxPin             GPIO_Pin_3
#define USART_PM25_TxPin             GPIO_Pin_2
#define USART_PM25_IRQn              USART2_IRQn
#define USART_PM25_IRQHandler        USART2_IRQHandler
#define RCC_PM25_APBPeriphClockCmd_Init()   {\
 	RCC_APB2PeriphClockCmd(USART_PM25_GPIO_CLK, ENABLE);\
	RCC_APB1PeriphClockCmd(USART_PM25_CLK, ENABLE);\
	}

#define USART_Radio                   USART3
#define USART_Radio_GPIO              GPIOB
#define USART_Radio_CLK               RCC_APB1Periph_USART3
#define USART_Radio_GPIO_CLK          RCC_APB2Periph_GPIOB
#define USART_Radio_RxPin             GPIO_Pin_11
#define USART_Radio_TxPin             GPIO_Pin_10
#define USART_Radio_IRQn              USART3_IRQn
#define USART_Radio_IRQHandler        USART3_IRQHandler
#define RCC_Radio_APBPeriphClockCmd_Init() {\
	RCC_APB2PeriphClockCmd(USART_Radio_GPIO_CLK, ENABLE);\
	RCC_APB1PeriphClockCmd(USART_Radio_CLK, ENABLE);\
	}


void USART_WIFI_Init(uint32_t baudrate);
void USART_PM25_Init(uint32_t baudrate);
void USART_Radio_Init(uint32_t bautrate);


#endif





