
#include "ExtiDrv.h"
#include "os_global.h"
#include "os_timer.h"
#include "board_version.h"
#include "BatteryLevel.h"
#include "PowerCtrl.h"
#include "FatFs_Demo.h"
#include "Application.h"

#if EXTI_DEBUG_EN 
#define EXT_DEBUG(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#else
#define EXT_DEBUG(...)
#endif

uint8_t battery_is_charging = 0; // 电池是否正在充电, 高4位表示: USB是否插入; 低 4位表示: 电池是否在充电

static os_timer_t tExtiCheckTimer;   // 外部中断延时检测定时器
static void ExtiCheckTimer_CallBack(void * arg)
{
    if(VIN_DETECT_Read())		// 当前为高电平, 说明是上升沿
	{
		EXT_DEBUG("usb pluged\n");
		battery_is_charging |= USB_PLUGED_MASK;
		if(is_5v_power_close)
		{
		        JumpToBootloader();
				is_5v_power_close = E_FALSE;
		}
		else 
			FILE_SearchUpdateBinFile(); // 搜索bin文件, 符合条件则升级
	}
	else  // 当前是低电平, 说明是下降沿
	{
	   EXT_DEBUG("usb unpluged\n");
	   battery_is_charging = 0;
	}
}


#define STM32_EXTI_ClearITPendingBit(EXTI_Line) \
	(EXTI->PR = EXTI_Line)

void ExtiDrv_Init(void)
{
    EXTI_InitTypeDef   EXTI_InitStructure;
    GPIO_InitTypeDef   GPIO_InitStructure;


	 os_timer_setfn(&tExtiCheckTimer, ExtiCheckTimer_CallBack, NULL);
	 
	 // 使能 IO 时钟
	 VIN_DETECT_RCC_APBPeriphClockCmdEnable();
	 
	 GPIO_InitStructure.GPIO_Pin  = VIN_DETECT_Pin;
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;

	 STM32_GPIOInit(VIN_DETECT_PORT, &GPIO_InitStructure);
	 
	 /* Enable AFIO clock */
	// RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
   
	 /* Connect EXTIn 外部中断线到 IO 管脚  */
	 GPIO_EXTILineConfig(VIN_DETECT_PortSource, VIN_DETECT_PinSource);
     STM32_EXTI_ClearITPendingBit(EXTI_Line_VinDetect);  // 清除中断标志位

	 
	 /* Configure EXTI0 line */
	 EXTI_InitStructure.EXTI_Line = EXTI_Line_VinDetect;
	 EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	 EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;  
	 EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	 EXTI_Init(&EXTI_InitStructure);
   
	 /* Enable and set EXTI0 Interrupt to the lowest priority */
	 STM32_NVICInit(EXTI_VinDetect_IRQn,3, 6, 1);	 // 第3组优先级, 3位抢占优先级, 1位响应优先级
}

//外部中断7服务程序 
void EXTI9_5_IRQHandler(void)
{
    CLEAR_REG_32_BIT(EXTI->IMR, EXTI_Line_VinDetect);  // 禁止外部中断
    
    if(READ_REG_32_BIT(EXTI->PR, EXTI_Line_VinDetect))  // PA7 管脚的中断
    {

		if(VIN_DETECT_Read())		// 当前为高电平, 说明是上升沿
		{
		    EXT_DEBUG("VD Rise\r\n");
		}
		else  // 当前是低电平, 说明是下降沿
		{
		    EXT_DEBUG("VD Fall\r\n");
		}
		os_timer_arm(&tExtiCheckTimer, 1, 0);  // 延时 10 ms 检测

		#if 0
		EXTI_ClearITPendingBit(EXTI_Line_VinDetect); //清除LINE0上的中断标志位  
		#else
        EXTI->PR = EXTI_Line_VinDetect; // 往挂起位写 1 清中断标志
		#endif
    }
	SET_REG_32_BIT(EXTI->IMR, EXTI_Line_VinDetect);  // 使能外部中断
}

