
#ifndef __UART_QUEUE_H__
#define  __UART_QUEUE_H__

#include <stdint.h>
#include "Uart_Drv.h"

#define  UART_CIRCLE_TX_QUEUE_EN    // 循环队列使能






void Uart_Q_Init(void);
void Queue_UART_IRQHandler(void);



#ifndef UART_CIRCLE_TX_QUEUE_EN
void Uart_SendByte(uint8_t data);
void Uart_SendDatas(uint8_t *pString, uint16_t len);
void Uart_SendString(uint8_t *pString);
#else
UartStatus Uart_SendByte(uint8_t data);
UartStatus Uart_SendDatas(uint8_t *pString, uint16_t len);
UartStatus Uart_SendString(uint8_t *pString);

#endif  //end  UART_CIRCLE_TX_QUEUE_EN

#ifdef UART_DEBUG
void Uart_RxToTx(uint8_t * ReadOutBuff, uint16_t BuffSize);
#endif //end UART_DEBUG

Compare_State Uart_IsRxBuffSizeEqualThan(uint16_t compare_size);
uint8_t Uart_IsTxDataBuffEmpty(void);

uint16_t Uart_CheckRxFlag(void);
void Uart_ClearRxFlag(void);


UartStatus Uart_ReadDequeueByte(uint8_t *pdata);
uint16_t Uart_ReadDequeueBuff(QueueMemType *ReadOutBuff);

#endif

