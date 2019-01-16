
#ifndef __UART_DRV__
#define __UART_DRV__

#include <stdint.h>
#include "stm32f10x.h"
//#include <intrinsics.h>
#include "GlobalDef.h"
//#include "Util_Drv.h"
#include <string.h>


#pragma pack(1)
// 传感器接收管理
typedef struct
{
       uint8_t   * buf;                                  // 接收缓冲区
       uint8_t      size;                                 // 接收缓冲区长度
	volatile uint16_t  rx_cnt;          // 接收计数
	volatile uint16_t rx_len;           // 接收的数据长度
	volatile uint8_t   last_val;        // 上一次接收的值
	volatile uint8_t   is_rx;            // 包接收标志: 是否已接收到
}T_UART_Rx;	
#pragma pack()



#define UART_Q_RX_BUF_SIZE   64
#define UART_Q_TX_BUF_SIZE  512

#define PM25_RX_BUF_SIZE   64
#define PM25_TX_BUF_SIZE   32

#define HCHO_RX_BUF_SIZE   64
#define HCHO_TX_BUF_SIZE   32



#define PM25_UART              USART1
#define PM25_UART_IRQHandler   USART1_IRQHandler

#define QUEUE_UART             USART2   // 调试输出选择的硬件串口
#define Queue_UART_IRQHandler    USART2_IRQHandler

#define HCHO_UART   USART3
#define HCHO_UART_IRQHandler   USART3_IRQHandler


#define  WIFI_USART_IRQHandler      // USART2_IRQHandler
#define  WIFI_USART                                   //USART2
#define  WIFI_USART_Init                        // USART2_Init


// 使能接收PM25数据
#define EnableRxPM25Sensor()   USART_ITConfig(PM25_UART, USART_IT_RXNE, ENABLE)

// 暂停接收PM25数据
#define DisableRxPM25Sensor()  USART_ITConfig(PM25_UART, USART_IT_RXNE, DISABLE)

extern UartStatus Uart_SendByte(uint8_t data);




void USART1_Init(uint32_t freq, uint32_t baudrate);
void USART2_Init(uint32_t freq, uint32_t baudrate);

void USART3_Init(uint32_t freq, uint32_t baudrate);


void os_print(char * s);

#endif

