
#ifndef __DHT11_H__
#define  __DHT11_H__

#include "GlobalDef.h"

// DHT11 IO 管脚 -->   PB1
#define DHT_IO_Pin         GPIO_Pin_1
#define DHT_IO_PORT        GPIOB
#define DHT_IO_READ()      READ_REG_32_BIT(DHT_IO_PORT->IDR, DHT_IO_Pin)   
#define DHT_IO_H()         SET_REG_32_BIT(DHT_IO_PORT->BSRR, DHT_IO_Pin)  // 输出高, GPIOx->BSRR = GPIO_Pin;
#define DHT_IO_L()         SET_REG_32_BIT(DHT_IO_PORT->BRR,  DHT_IO_Pin)  // 输出低  GPIOx->BRR = GPIO_Pin;
#define DHT_RCC_APBPeriphClockCmdEnable()       RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE)   // PCLK2 = HCLK = 48MHz

// DHT 响应错误码
typedef enum
{
   DHT_OK = 0,
   DHT_INIT_OK,      // 1 初始化成功
   DHT_NO_RESP,     // 总线上 无DHT 响应
   DHT_PD_TIMEOUT,  // DHT 拉低超时
   DHT_PU_TIMEOUT,  // DHT 拉高超时
   DHT_BIT_ERR,     // DHT 数据位错误
   DHT_CHECK_ERR,  // 校验错误
   DHT_NOT_INIT,  // 还未初始化
   DHT_ERR,
}DHT_RESULT;

typedef struct
{
   uint8_t humi_H;
   uint8_t humi_L;
   uint8_t temp_H;
   uint8_t temp_L;
   uint8_t sum;
}T_DHT;

extern T_DHT tDHT;

extern void  TempHumi_SetSensorExisted(uint8_t state);

void DHT_Init(void);
void DHT_Start(void);

#endif

