
#ifndef __REG_LIB_H__
#define  __REG_LIB_H__
/********************** STM32F10X 寄存器库 ******************************************/

#include <stm32f10x.h>
#include "sys.h"									//用于位段操作，用法见头文件

#define NVIC_GROUP 2


/********************************************SPI通信实验***********************************************/
/*功能函数使用说明：
	1，STM32_SPIxInit用于初始化SPI口，SPIx为SPI口。
	2，STM32_SPIxSetSpeed设置SPI波特率分频，SPIx为SPI口，波特率=Fpclk/分频数。
	3，STM32_SPIxReadWriteByte用于通过SPI总线读写一个字节，SPIx为SPI口，TxData为要写到SPI总线上的数据，
		函数返回从SPI总线上读取的数据。
*/
// SPI总线波特率设置 
typedef enum 
{
	SPIBaudFreqDiv_2=0,SPIBaudFreqDiv_4,SPIBaudFreqDiv_8,SPIBaudFreqDiv_16,
	SPIBaudFreqDiv_32,SPIBaudFreqDiv_64,SPIBaudFreqDiv_128,SPIBaudFreqDiv_256
}SPIBaudFreqDiv_TypeDef;


#define STM32_RCC_APB2PeriphClockCmd(RCC_APB2Periph, NewState) \
	((NewState) ? (RCC->APB2ENR |= RCC_APB2Periph) : (RCC->APB2ENR &= ~RCC_APB2Periph))

#define STM32_RCC_APB1PeriphClockCmd(RCC_APB1Periph, NewState) \
	((NewState) ? (RCC->APB1ENR |= RCC_APB1Periph) : (RCC->APB1ENR &= ~RCC_APB1Periph))

// 外设复位
#define STM32_RCC_APB2PeriphResetCmd(RCC_APB2Periph, NewState) \
	((NewState) ? (RCC->APB2RSTR |= RCC_APB2Periph) : (RCC->APB2RSTR &= ~RCC_APB2Periph))

#define STM32_GPIOx_DeInit(RCC_APB2Periph) {\
	STM32_RCC_APB2PeriphResetCmd(RCC_APB2Periph, ENABLE);\
	STM32_RCC_APB2PeriphResetCmd(RCC_APB2Periph, DISABLE);\
	}

#define USELIB_CFGR_ADCPRE_Reset_Mask    ((uint32_t)0xFFFF3FFF)

#define STM32_RCC_ADCCLKConfig(RCC_PCLK2) \
  (RCC->CFGR = (RCC->CFGR & USELIB_CFGR_ADCPRE_Reset_Mask) | RCC_PCLK2)

#define REG_CFGR_USBPRE_BB   ((uint32_t)0x424200D8)

#define STM32_RCC_USBCLKConfig(RCC_USBCLKSource) \
	(*(__IO uint32_t *) REG_CFGR_USBPRE_BB = RCC_USBCLKSource)

// GPIO 管脚设置为模拟输入方式
#define STM32_GPIO_Mode_AIN(GPIOx, GPIO_Pin) \
	((GPIO_Pin < 8) ? (GPIOx->CRL &=~ (0x0F << (GPIO_Pin * 4))) : (GPIOx->CRH &=~ (0x0F <<((GPIO_Pin - 8) * 4))))

#define STM32_GPIO_ReadInputDataBit(GPIOx, GPIO_Pin) ( (GPIOx->IDR & GPIO_Pin) ? 1 : 0)

// 软件触发ADC
#define STM32_ADC_SoftwareStartConvCmd(ADCx, NewState) \
	(NewState ? (ADCx->CR2 |= (ADC_CR2_EXTTRIG | ADC_CR2_SWSTART)) : \
	(ADCx->CR2 &=~ (ADC_CR2_EXTTRIG | ADC_CR2_SWSTART)) )

#define STM32_ADC_DeInit_ADC1() {\
	RCC->APB2RSTR |= RCC_APB2Periph_ADC1;\
	RCC->APB2RSTR &= ~(RCC_APB2Periph_ADC1);\
	}

//使能指定的ADC1
#define STM32_ADC_Cmd(ADCx, NewState) \
	(NewState ? (ADCx->CR2 |= ADC_CR2_ADON) : \
	(ADCx->CR2 &= ~(ADC_CR2_ADON)))
	
//使能复位校准  
#define STM32_ADC_ResetCalibration(ADCx)   (ADCx->CR2 |= ADC_CR2_RSTCAL)

//获取复位校准状态
#define STM32_ADC_GetResetCalibrationStatus(ADCx) (ADCx->CR2 & ADC_CR2_RSTCAL)

//开启AD校准
#define STM32_ADC_StartCalibration(ADCx)  (ADCx->CR2 |= ADC_CR2_CAL)

// 获取校准状态
#define STM32_ADC_GetCalibrationStatus(ADCx)  (ADCx->CR2 & ADC_CR2_CAL)

/**
  * @brief  Checks whether the specified SPI/I2S flag is set or not.
  * @param  SPIx: where x can be
  *   - 1, 2 or 3 in SPI mode 
  *   - 2 or 3 in I2S mode
  * @param  SPI_I2S_FLAG: specifies the SPI/I2S flag to check. 
  *   This parameter can be one of the following values:
  *     @arg SPI_I2S_FLAG_TXE: Transmit buffer empty flag.
  *     @arg SPI_I2S_FLAG_RXNE: Receive buffer not empty flag.
  *     @arg SPI_I2S_FLAG_BSY: Busy flag.
  *     @arg SPI_I2S_FLAG_OVR: Overrun flag.
  *     @arg SPI_FLAG_MODF: Mode Fault flag.
  *     @arg SPI_FLAG_CRCERR: CRC Error flag.
  *     @arg I2S_FLAG_UDR: Underrun Error flag.
  *     @arg I2S_FLAG_CHSIDE: Channel Side flag.
  * @retval The new state of SPI_I2S_FLAG (SET or RESET).
  */
#define STM32_SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG)  (SPIx->SR & (SPI_I2S_FLAG))

#define STM32_SPI_Cmd(SPIx, NewState) \
	(NewState ? (SPIx->CR1 |= SPI_CR1_SPE) : (SPIx->CR1 &= ~(SPI_CR1_SPE)) )

#define STM32_SPI_CalculateCRC(SPIx, NewState) \
	(NewState ? (SPIx->CR1 |= SPI_CR1_CRCEN) : (SPIx->CR1 &= ~(SPI_CR1_CRCEN)) )
	

#define SPI1_I2S_DEINIT()   { RCC->APB1RSTR |= RCC_APB2Periph_SPI1; RCC->APB2RSTR &= ~RCC_APB2Periph_SPI1; }
#define SPI2_I2S_DEINIT()   { RCC->APB1RSTR |= RCC_APB1Periph_SPI2; RCC->APB1RSTR &= ~RCC_APB1Periph_SPI2; }
#define SPI3_I2C_DEINIT()   { RCC->APB1RSTR |= RCC_APB1Periph_SPI3; RCC->APB1RSTR &= ~RCC_APB1Periph_SPI3; }

	
#define SPI_CR1_CLEAR_Mask       ((uint16_t)0x3040)

void STM32_SPI_I2S_DeInit(SPI_TypeDef * SPIx);

// 功能同 SPI_Init()
#define STM32_SPI_Init(SPIx, SPI_Direction,\
	SPI_Mode, SPI_DataSize, SPI_CPOL, SPI_CPHA,\
	SPI_NSS, SPI_BaudRatePrescaler, SPI_FirstBit, SPI_CRCPolynomial) {\
	SPIx->CR1 = (uint16_t)((uint32_t)SPI_Direction | SPI_Mode | SPI_DataSize |\
	SPI_CPOL |  SPI_CPHA | SPI_NSS | SPI_BaudRatePrescaler | SPI_FirstBit) +\
	(SPIx->CR1 & SPI_CR1_CLEAR_Mask);\
	SPIx->I2SCFGR &=~ (SPI_I2SCFGR_I2SMOD);	\
	SPIx->CRCPR = SPI_CRCPolynomial;\
	}

void STM32_NVICPriorityGroupConfig(u8 NVIC_Group);	
void STM32_NVICInit(u8 NVIC_Channel,u8 NVIC_Group,u8 NVIC_PreemptionPriority,u8 NVIC_SubPriority);

void STM32_GPIOInit(GPIO_TypeDef* GPIOx,GPIO_InitTypeDef* GPIOInit_pst);
void STM32_SPIxInit(SPI_TypeDef *SPIx);

void STM32_ADC_RegularChannelConfig(ADC_TypeDef* ADCx, uint8_t ADC_Channel, uint8_t Rank, uint8_t ADC_SampleTime);
void STM32_ADC_Init(ADC_TypeDef* ADCx, 
                          uint32_t ADC_Mode, 
                          uint8_t  ScanConvMode, 
                          uint8_t  ContinuousConvMode, 
                          uint32_t ExternalTrigConv,
                          uint32_t DataAlign,
                          uint8_t  NbrOfChannel);

#define  IO_H(GPIOx, Pin)          (GPIOx->BSRR = Pin)
#define  IO_L(GPIOx, Pin)          (GPIOx->BRR    = Pin)
#define  IO_READ_IN(GPIOx, Pin)    ( (GPIOx->IDR & Pin) ? 1 : 0)
#define IO_READ_OUT(GPIOx, Pin)   ( (GPIOx->ODR & Pin) ? 1 : 0)



#endif


