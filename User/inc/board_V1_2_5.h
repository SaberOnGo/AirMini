


#ifndef __BOARD_V1_2_5_H__
#define  __BOARD_V1_2_5_H__
/**************************************************************************************
主板管脚定义文件
主板硬件版本: V1.1.2.5
日期: 2017-11-11
硬件: GD32F103C8T6 LQFP 48,
             LCD1602, 甲醛传感器, SHT20,TVOC传感器, PMS5003(可选)
OS:      None, 软件定时器
功能: 
**************************************************************************************/

#include "GlobalDef.h"
#include "stm32f10x.h"

// PM2.5 休眠使能控制管脚, 高电平正常工作
// PM25_SET  ->  
#define PM25_SET_Pin         
#define PM25_SET_PORT        
#define PM25_SET_Open()      
#define PM25_SET_Close()      
#define PM25_SET_RCC_APBPeriphClockCmdEnable()    


// XR1151 EN ->  PA4, 高电平使能
#define XR1151_EN_Pin             GPIO_Pin_4
#define XR1151_EN_PORT            GPIOA
#define XR1151_EN_Open()          SET_REG_32_BIT(XR1151_EN_PORT->BSRR, XR1151_EN_Pin)  // 输出高, GPIOx->BSRR = GPIO_Pin;
#define XR1151_EN_Close()         SET_REG_32_BIT(XR1151_EN_PORT->BRR,  XR1151_EN_Pin)  // 输出低  GPIOx->BRR = GPIO_Pin;
#define XR1151_EN_RCC_APBPeriphClockCmdEnable()      SET_REG_32_BIT(RCC->APB2ENR, RCC_APB2Periph_GPIOA)  // PCLK2 = HCLK = 48MHz

// CHRG_Indicate -> PA5  充电状态指示管脚, 读该管脚状态, 低电平: 正在充电; 高阻态: 没有在充电 
#define CHRG_Indicate_Pin      GPIO_Pin_5
#define CHRG_Indicate_PORT     GPIOA
#define CHRG_Indicate_Open()   SET_REG_32_BIT(CHRG_Indicate_PORT->BSRR, CHRG_Indicate_Pin)  // 输出高, GPIOx->BSRR = GPIO_Pin;
#define CHRG_Indicate_Close()  SET_REG_32_BIT(CHRG_Indicate_PORT->BRR,  CHRG_Indicate_Pin)  // 输出低  GPIOx->BRR = GPIO_Pin;
#define CHRG_Indicate_RCC_APBPeriphClockCmdEnable()       SET_REG_32_BIT(RCC->APB2ENR, RCC_APB2Periph_GPIOA)  // PCLK2 = HCLK = 48MHz
#define CHRG_Indicate_Read()   STM32_GPIO_ReadInputDataBit(CHRG_Indicate_PORT, CHRG_Indicate_Pin)   // GPIO_ReadInputDataBit(LCD_DATA_PORT, GPIO_Pin_4)


// LCD_Power_Ctrl -> PB10  IO 输出高电平导通
#define LCD_Power_Ctrl_Pin      GPIO_Pin_10
#define LCD_Power_Ctrl_PORT     GPIOB
#define LCD_Power_Ctrl_H()      SET_REG_32_BIT(LCD_Power_Ctrl_PORT->BSRR, LCD_Power_Ctrl_Pin)  // 输出高, GPIOx->BSRR = GPIO_Pin;
#define LCD_Power_Ctrl_L()      SET_REG_32_BIT(LCD_Power_Ctrl_PORT->BRR,  LCD_Power_Ctrl_Pin)  // 输出低  GPIOx->BRR = GPIO_Pin;
#define LCD_Power_Ctrl_RCC_APBPeriphClockCmdEnable()       SET_REG_32_BIT(RCC->APB2ENR, RCC_APB2Periph_GPIOB)   // PCLK2 = HCLK = 48MHz


// VIN_DETECT -> P7  USB电源插入读取管脚, 高电平: USB 5V 电源插入; 低电平: USB 电源没有插入
#define VIN_DETECT_Pin         GPIO_Pin_7
#define VIN_DETECT_PORT        GPIOA
#define VIN_DETECT_PinSource   GPIO_PinSource7
#define VIN_DETECT_PortSource  GPIO_PortSourceGPIOA
#define EXTI_Line_VinDetect    EXTI_Line7
#define EXTI_VinDetect_IRQn    EXTI9_5_IRQn

#define VIN_DETECT_Open()   SET_REG_32_BIT(VIN_DETECT_PORT->BSRR, VIN_DETECT_Pin)  // 输出高, GPIOx->BSRR = GPIO_Pin;
#define VIN_DETECT_Close()  SET_REG_32_BIT(VIN_DETECT_PORT->BRR,  VIN_DETECT_Pin)  // 输出低  GPIOx->BRR = GPIO_Pin;
#define VIN_DETECT_RCC_APBPeriphClockCmdEnable()       SET_REG_32_BIT(RCC->APB2ENR, RCC_APB2Periph_GPIOA)   // PCLK2 = HCLK = 48MHz
#define VIN_DETECT_Read()    STM32_GPIO_ReadInputDataBit(VIN_DETECT_PORT, VIN_DETECT_Pin)   // GPIO_ReadInputDataBit(LCD_DATA_PORT, GPIO_Pin_4)

// 按键管脚定义 -------------------------------------begin------------------------------*/
#define KEY1_PORT      GPIOB
#define KEY1_GPIO_Pin  GPIO_Pin_0

#define KEY2_PORT      GPIOB
#define KEY2_GPIO_Pin  GPIO_Pin_1

#define KEY3_PORT      
#define KEY3_GPIO_Pin  

#if 0
#define KEY1_INPUT   ((READ_REG_32_BIT(KEY1_PORT->IDR, KEY1_GPIO_Pin)) << 1)
#define KEY2_INPUT   ((READ_REG_32_BIT(KEY2_PORT->IDR, KEY2_GPIO_Pin)) << 2)
#else
#define KEY1_INPUT   ((STM32_GPIO_ReadInputDataBit(KEY1_PORT, KEY1_GPIO_Pin)) << 1)
#define KEY2_INPUT   ((STM32_GPIO_ReadInputDataBit(KEY2_PORT, KEY2_GPIO_Pin)) << 2)
#endif

//#define KEY3_INPUT   ((READ_REG_32_BIT(KEY3_PORT->IDR, KEY3_GPIO_Pin)) << 3)

#define KEY_INPUT  ((uint8_t)((KEY1_INPUT) |(KEY2_INPUT) ))
// 按键管脚定义 -------------------------------------end------------------------------*/

// RT9193 电源使能控制管脚, 上升沿使能
// PWR_SW  ->  PA8
#define PWR_SW_Pin          GPIO_Pin_8
#define PWR_SW_PORT         GPIOA
#define PWR_SW_Open()       SET_REG_32_BIT(PWR_SW_PORT->BSRR, PWR_SW_Pin)  // 输出高, GPIOx->BSRR = GPIO_Pin;
#define PWR_SW_Close()      SET_REG_32_BIT(PWR_SW_PORT->BRR,  PWR_SW_Pin)  // 输出低  GPIOx->BRR = GPIO_Pin;
#define PWR_SW_RCC_APBPeriphClockCmdEnable()       SET_REG_32_BIT(RCC->APB2ENR, RCC_APB2Periph_GPIOA)    // PCLK2 = HCLK = 48MHz


// TP4056 电池充电管脚使能控制管脚, 高电平: 正常工作; 低电平: 禁止充电
// BAT_CE  ->  PA9
#define BAT_CE_Pin          GPIO_Pin_9
#define BAT_CE_PORT         GPIOA
#define BAT_CE_Open()       SET_REG_32_BIT(BAT_CE_PORT->BSRR, BAT_CE_Pin)  // 输出高, GPIOx->BSRR = GPIO_Pin;
#define BAT_CE_Close()      SET_REG_32_BIT(BAT_CE_PORT->BRR,  BAT_CE_Pin)  // 输出低  GPIOx->BRR = GPIO_Pin;
#define BAT_CE_H()          SET_REG_32_BIT(BAT_CE_PORT->BSRR, BAT_CE_Pin)  // 输出高, GPIOx->BSRR = GPIO_Pin;
#define BAT_CE_L()          SET_REG_32_BIT(BAT_CE_PORT->BRR,  BAT_CE_Pin)  // 输出低  GPIOx->BRR = GPIO_Pin;
#define BAT_CE_RCC_APBPeriphClockCmdEnable()       SET_REG_32_BIT(RCC->APB2ENR, RCC_APB2Periph_GPIOA)    // PCLK2 = HCLK = 48MHz
#define BAT_CE_Read()       STM32_GPIO_ReadInputDataBit(BAT_CE_PORT, BAT_CE_Pin) 


/*----------------------I2C 硬件管脚定义 begin ----------------*/
// IIC_SDA -> PB7  
#define IIC_SDA_Pin       GPIO_Pin_7
#define IIC_SDA_PORT      GPIOB
#define IIC_SDA_H()       SET_REG_32_BIT(IIC_SDA_PORT->BSRR, IIC_SDA_Pin)  // 输出高, GPIOx->BSRR = GPIO_Pin;
#define IIC_SDA_L()       SET_REG_32_BIT(IIC_SDA_PORT->BRR,  IIC_SDA_Pin)  // 输出低  GPIOx->BRR = GPIO_Pin;
#define IIC_SDA_READ()    ( (IIC_SDA_PORT->IDR &IIC_SDA_Pin) ? 1 : 0)

// IIC_SCL -> PB6
#define IIC_SCL_Pin       GPIO_Pin_6
#define IIC_SCL_PORT      GPIOB
#define IIC_SCL_H()       SET_REG_32_BIT(IIC_SCL_PORT->BSRR, IIC_SCL_Pin)  // 输出高, GPIOx->BSRR = GPIO_Pin;
#define IIC_SCL_L()       SET_REG_32_BIT(IIC_SCL_PORT->BRR,  IIC_SCL_Pin)  // 输出低  GPIOx->BRR = GPIO_Pin;
#define IIC_APBPeriphClockCmdEnable()       SET_REG_32_BIT(RCC->APB2ENR, RCC_APB2Periph_GPIOB)    // PCLK2 = HCLK = 48MHz

#define IIC_BUS_PORT      GPIOB
/*----------------------I2C 硬件管脚定义 end ----------------*/

// LED 蓝灯控制管脚
// LED_Blue  ->  PA6
#define LED_Blue_Pin          GPIO_Pin_6
#define LED_Blue_PORT         GPIOA
#define LED_Blue_Open()       SET_REG_32_BIT(LED_Blue_PORT->BSRR, LED_Blue_Pin)  // 输出高, GPIOx->BSRR = GPIO_Pin;
#define LED_Blue_Close()      SET_REG_32_BIT(LED_Blue_PORT->BRR,  LED_Blue_Pin)  // 输出低  GPIOx->BRR = GPIO_Pin;
#define LED_Blue_RCC_APBPeriphClockCmdEnable()       SET_REG_32_BIT(RCC->APB2ENR, RCC_APB2Periph_GPIOA)   // PCLK2 = HCLK = 48MHz

// LED 绿灯控制管脚
// LED_Green  ->  PA2
#define LED_Green_Pin          GPIO_Pin_2
#define LED_Green_PORT         GPIOA
#define LED_Green_Open()       SET_REG_32_BIT(LED_Green_PORT->BSRR, LED_Green_Pin)  // 输出高, GPIOx->BSRR = GPIO_Pin;
#define LED_Green_Close()      SET_REG_32_BIT(LED_Green_PORT->BRR,  LED_Green_Pin)  // 输出低  GPIOx->BRR = GPIO_Pin;
#define LED_Green_RCC_APBPeriphClockCmdEnable()       SET_REG_32_BIT(RCC->APB2ENR, RCC_APB2Periph_GPIOA)   // PCLK2 = HCLK = 48MHz

// LED 红灯控制管脚
// LED_Red  ->  PA1
#define LED_Red_Pin          GPIO_Pin_1
#define LED_Red_PORT         GPIOA
#define LED_Red_Open()       SET_REG_32_BIT(LED_Red_PORT->BSRR, LED_Red_Pin)  // 输出高, GPIOx->BSRR = GPIO_Pin;
#define LED_Red_Close()      SET_REG_32_BIT(LED_Red_PORT->BRR,  LED_Red_Pin)  // 输出低  GPIOx->BRR = GPIO_Pin;
#define LED_Red_RCC_APBPeriphClockCmdEnable()       SET_REG_32_BIT(RCC->APB2ENR, RCC_APB2Periph_GPIOA)   // PCLK2 = HCLK = 48MHz


/*--------------------------ADC 管脚定义 begin ---------------------*/
// BAT_ADC -> PA0  ->  ADC1 Channel 0
#define BAT_ADC_Pin             GPIO_Pin_0
#define BAT_ADC_PORT            GPIOA
#define BAT_ADC_x               ADC1
#define BAT_ADC_Channel         ADC_Channel_0

/*--------------------------ADC 管脚定义 end  ---------------------*/


/* ----------------------LCD 1602 管脚定义 begin  -----------------*/
#define LCD_RS_PIN   GPIO_Pin_13   // PC13
#define LCD_RS_PORT  GPIOC
#define LCD_RS_H()   (LCD_RS_PORT->BSRR =  LCD_RS_PIN)   // GPIO_SetBits(LCD_RS_PORT, LCD_RS_PIN);
#define LCD_RS_L()   (LCD_RS_PORT->BRR  =  LCD_RS_PIN)



#define LCD_RW_PIN   GPIO_Pin_9   // PB9
#define LCD_RW_PORT  GPIOB
#define LCD_RW_H()   (LCD_RW_PORT->BSRR = LCD_RW_PIN)
#define LCD_RW_L()   (LCD_RW_PORT->BRR  = LCD_RW_PIN)


#define LCD_EN_PIN   GPIO_Pin_8   // PB8
#define LCD_EN_PORT  GPIOB
#define LCD_EN_H()   (LCD_EN_PORT->BSRR = LCD_EN_PIN)
#define LCD_EN_L()   (LCD_EN_PORT->BRR  = LCD_EN_PIN)

// LCD_D4  -> PB5
#define  LCD_D4_Pin    GPIO_Pin_5
#define  LCD_D4_PORT   GPIOB

// LCD_D5  -> PB4
#define  LCD_D5_Pin    GPIO_Pin_4
#define  LCD_D5_PORT   GPIOB

// LCD_D6  -> PB3
#define  LCD_D6_Pin    GPIO_Pin_3
#define  LCD_D6_PORT   GPIOB

// LCD_D7  -> PA15
#define  LCD_D7_Pin    GPIO_Pin_15
#define  LCD_D7_PORT   GPIOA


#define GPIO_WRITE_BIT(GPIOx, GPIO_Pin, bitVal) \
	((bitVal) ? (GPIOx->BSRR = GPIO_Pin) : (GPIOx->BRR = GPIO_Pin))


#define LCD_D7_WRITE(bitVal)  GPIO_WRITE_BIT(LCD_D7_PORT, LCD_D7_Pin, bitVal)  // GPIO_WriteBit(LCD_D7_PORT, LCD_D7_Pin, ((bitVal) ? Bit_SET : Bit_RESET) )
#define LCD_D6_WRITE(bitVal)  GPIO_WRITE_BIT(LCD_D6_PORT, LCD_D6_Pin, bitVal)  // GPIO_WriteBit(LCD_D6_PORT, LCD_D6_Pin, ((bitVal) ? Bit_SET : Bit_RESET) )
#define LCD_D5_WRITE(bitVal)  GPIO_WRITE_BIT(LCD_D5_PORT, LCD_D5_Pin, bitVal)  // GPIO_WriteBit(LCD_D5_PORT, LCD_D5_Pin, ((bitVal) ? Bit_SET : Bit_RESET) )
#define LCD_D4_WRITE(bitVal)  GPIO_WRITE_BIT(LCD_D4_PORT, LCD_D4_Pin, bitVal)  // GPIO_WriteBit(LCD_D4_PORT, LCD_D4_Pin, ((bitVal) ? Bit_SET : Bit_RESET) )

#define LCD_D7_READ()  READ_REG_32_BIT(LCD_D7_PORT, LCD_D7_Pin)
#define LCD_D6_READ()  READ_REG_32_BIT(LCD_D6_PORT, LCD_D6_Pin)
#define LCD_D5_READ()  READ_REG_32_BIT(LCD_D5_PORT, LCD_D5_Pin)
#define LCD_D4_READ()  READ_REG_32_BIT(LCD_D4_PORT, LCD_D4_Pin)



#define LCD_DATA_4BIT_H()   {\
	LCD_D7_WRITE(Bit_SET);\
	LCD_D6_WRITE(Bit_SET);\
	LCD_D5_WRITE(Bit_SET);\
	LCD_D4_WRITE(Bit_SET);\
	}  // 数据位 4bit 都置为 1
	
#define LCD_DATA_4BIT_L()   {\
	LCD_D7_WRITE(Bit_RESET);\
	LCD_D6_WRITE(Bit_RESET);\
	LCD_D5_WRITE(Bit_RESET);\
	LCD_D4_WRITE(Bit_RESET);\
	}

// 发送val的高4位值
#define LCD_DATA_4BIT_OUT(val)   {\
	LCD_D7_WRITE(val & 0x80);\
	LCD_D6_WRITE(val & 0x40);\
	LCD_D5_WRITE(val & 0x20);\
	LCD_D4_WRITE(val & 0x10);\
	}

#define RCC_APB2Periph_LCD_GPIO  ( RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOA)
/* ----------------------LCD 1602 管脚定义 end  -----------------*/

void LCD_Ctrl_Set(E_SW_STATE sta);
void Board_GpioInit(void);
void BAT_CE_Set(E_SW_STATE sta);



#endif

