
#ifndef __SW_UART_H__
#define __SW_UART_H__

#include "GlobalDef.h"

#define   SW_UART1_TX_EN      1  // 发送使能:1 ; 0: 禁止
#define   SW_UART1_RX_EN      1   // 接收使能(1), 0: 禁止


#define   SW_UART1_RX_MAX_LEN  	64   // 最大接收字节数


 // SW UART  TX: PA.14
#define   RCC_APB2Periph_SW_UART1_TX_PIN    RCC_APB2Periph_GPIOA
#define   SW_UART1_TX_PIN                    GPIO_Pin_14
#define   SW_UART1_TX_PORT               GPIOA
#define   SW_UART1_TX_H()                    IO_H(SW_UART1_TX_PORT, SW_UART1_TX_PIN)
#define   SW_UART1_TX_L()                     IO_L(SW_UART1_TX_PORT, SW_UART1_TX_PIN)


// SW UART RX: PA.13
#define   RCC_APB2Periph_SW_UART1_RX_PIN    RCC_APB2Periph_GPIOA
#define   SW_UART1_RX_PIN                    GPIO_Pin_13   // PA 13
#define   SW_UART1_RX_PORT               GPIOA
#define   GPIO_PortSource_SW_UART1_RX              GPIO_PortSourceGPIOA
#define   GPIO_PinSource_SW_UART1_RX                 GPIO_PinSource13
#define    EXTI_Line_SW_UART1_RX                              EXTI_Line13
#define    EXTI_SW_UART1_RX_IRQn                           EXTI15_10_IRQn



void SWUART1_Init(void);
void SWUART1_Send(uint8_t Byte);

void SW_UART1_IRQHandler(uint8_t data);

#endif

