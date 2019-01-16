
#include "board_V1_2_3.h"
#include "os_timer.h"
#include "delay.h"
#include "RegLib.h"


/*****************************************
功能: 控制LCD 电源输出
参数: CTRL_OPEN: 打开;  CTRL_CLOSE: 关闭此开关

****************************************/
void LCD_Ctrl_Set(E_SW_STATE sta)
{
    GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin   = LCD_Power_Ctrl_Pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;

	if(SW_OPEN == sta)
	{
	    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
		STM32_GPIOInit(LCD_Power_Ctrl_PORT, &GPIO_InitStructure);
		
		LCD_Power_Ctrl_L();  // PMOS管 低电平导通
	}
	else // 设置为浮空输入, 电源上拉为高电平, PMOS管关闭
	{
	   GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	   STM32_GPIOInit(LCD_Power_Ctrl_PORT, &GPIO_InitStructure);
	}
}

void BAT_CE_Set(E_SW_STATE sta)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin   = BAT_CE_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;

	if(SW_OPEN == sta)
	{
	   GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
       STM32_GPIOInit(BAT_CE_PORT, &GPIO_InitStructure);
	}
	else 
	{
	    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
        STM32_GPIOInit(BAT_CE_PORT, &GPIO_InitStructure);
		
		BAT_CE_L();  // 禁止电池充电
	}
}

// 设置数据口为输入或输出
// 板级管脚初始化
void Board_GpioInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 时钟使能
	STM32_RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
	
    // XR1151 5V EN  
	GPIO_InitStructure.GPIO_Pin   = XR1151_EN_Pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    STM32_GPIOInit(XR1151_EN_PORT, &GPIO_InitStructure);
	
	//XR1151_EN_Close();
	XR1151_EN_Open();

	// PWR_SW, 3.3V稳压器开关
	GPIO_InitStructure.GPIO_Pin   = PWR_SW_Pin;
    STM32_GPIOInit(PWR_SW_PORT, &GPIO_InitStructure);
	
	//PWR_SW_Close();
	PWR_SW_Open();

	// LCD_Power, 屏幕电源开关
	LCD_Ctrl_Set(SW_OPEN);

    // 电池充电控制管脚
	BAT_CE_Set(SW_OPEN);  // 默认打开充电
    
    // 电池充电检测管脚
    GPIO_InitStructure.GPIO_Pin   = CHRG_Indicate_Pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
	STM32_GPIOInit(CHRG_Indicate_PORT, &GPIO_InitStructure);
	
	// VIN_DETECT 
	GPIO_InitStructure.GPIO_Pin   = VIN_DETECT_Pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD;
	STM32_GPIOInit(VIN_DETECT_PORT, &GPIO_InitStructure);
}

