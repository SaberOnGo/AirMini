
#include "ADC_Drv.h"
#include "board_version.h"


#include <stdio.h>
#include <stdarg.h>
#include "delay.h"


#if   DEBUG_ADC_EN
#define ADC_DEBUG(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#else
#define ADC_DEBUG(...)
#endif


// 计算电压值
// 比实际值大1000倍, 即将后3位小数转为整数
#define  GetVoltValue(adc_val) \
	((uint16_t)((double)adc_val * 3300 / 4096))  

// 计算电池电压
// 参数: uint16_t volt: ADC测量的电压, 单位: mV
// 返回值: uint16_t 电池电压: 单位: mV
#define  GetBatVolt(volt)   ((uint16_t)(( 635.0 / 470.0) * (volt)))




// 常规的ADC 模式初始化
void ADCDrv_NormalModeInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	STM32_RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1	, ENABLE );	  //使能ADC1通道时钟
	STM32_RCC_ADCCLKConfig(RCC_PCLK2_Div4);   //设置ADC分频因子4, 48 M /4 = 12,ADC最大时间不能超过14M

	//PA0 作为模拟通道输入引脚                         
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;		//模拟输入引脚

    STM32_GPIOInit(GPIOA, &GPIO_InitStructure);	
	STM32_ADC_DeInit_ADC1();
	
	STM32_ADC_Init(ADC1, 
		             ADC_Mode_Independent,        //ADC工作模式:ADC1和ADC2工作在独立模式
		             DISABLE,   	                //模数转换工作在单通道模式
		             DISABLE,	                    //模数转换工作在单次转换模式
		             ADC_ExternalTrigConv_None, 	//转换由软件而不是外部触发启动
		             ADC_DataAlign_Right,        //ADC数据右对齐
		             1);                            //顺序进行规则转换的ADC通道的数目

    /* ADC1 regular channel 0 configuration */ 
    // STM32_ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);

	STM32_ADC_Cmd(ADC1, ENABLE);	//使能指定的ADC1
	delay_us(30);
	
	STM32_ADC_ResetCalibration(ADC1);	//使能复位校准  
	 
	while(STM32_ADC_GetResetCalibrationStatus(ADC1));	//等待复位校准结束
	
	STM32_ADC_StartCalibration(ADC1);	 //开启AD校准
 
	while(STM32_ADC_GetCalibrationStatus(ADC1));	 //等待校准结束

	//STM32_NVICInit(ADC1_2_IRQn,NVIC_GROUP,1,1);	//设置ADC1中断优先级
}



//获得ADC值
//ch:通道值 0~3
uint16_t ADCDrv_GetVal(void)   
{
  	//设置指定ADC的规则组通道，一个序列，采样时间
	STM32_ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5 );	//ADC1,ADC通道,采样时间为239.5周期	  			    

    STM32_ADC_SoftwareStartConvCmd(ADC1, ENABLE);         // 使能指定的ADC1的软件转换启动功能	
    while(! (READ_REG_32_BIT(ADC1->SR, ADC_FLAG_EOC)));  // 等待转换结束

	return (uint16_t)ADC1->DR;	//返回最近一次ADC1规则组的转换结果
}

#include "os_timer.h"
#include "ExtiDrv.h"

static os_timer_t tADCTimer;
static os_timer_t tBatChargeCloseTimer;  
#define MAX_TIMES   40    // 40 次平均
static uint32_t total_sum = 0;
static uint16_t get_adc_count = 0;


// 每次获取电池电压的启动操作由此定时器来启动
#if DEBUG_ADC_EN
static uint8_t is_first_sequ = 0;
#endif

static void BatChargeCloseTimer_CallBack(void * arg)
{
   #if DEBUG_ADC_EN
   if(VIN_DETECT_Read())
    {
       //ADC_DEBUG("USB Power is pluged\r\n");
    }
   is_first_sequ = E_TRUE;
   #endif

   BAT_CE_Set(SW_CLOSE);  // 先关闭电池充电, 再检测电池电压, 这样测量的电池电压才准确
   STM32_ADC_SoftwareStartConvCmd(ADC1, ENABLE);   // 软件启动 AD 转换
   os_timer_arm(&tADCTimer, 5, 0);  
}

static void ADCTimer_CallBack(void * arg)
{
	uint32_t tick = 2;

	//ADC_DEBUG("c=%d, %ld\n", get_adc_count, os_get_tick());
	if(READ_REG_32_BIT(ADC1->SR, ADC_SR_EOC)) //转换结束  
	{
	    #if DEBUG_ADC_EN
	    uint16_t cur_volt = 0;  //电压

		if(is_first_sequ)
		{
		    is_first_sequ = E_FALSE;
			
			cur_volt = (uint16_t)ADC1->DR;
			cur_volt = 3300 * cur_volt / 4096;
			cur_volt = GetBatVolt(cur_volt);
			ADC_DEBUG("1th=%d.%03d V\n", cur_volt / 1000, cur_volt % 1000);
		}
		#endif
		
	    total_sum += (uint16_t)ADC1->DR;
		get_adc_count++;
		if(get_adc_count >= MAX_TIMES)
		{
		   uint16_t aver_volt, aver_adc, bat_volt;
		   
		   aver_adc = total_sum / MAX_TIMES;

           aver_volt = GetVoltValue(aver_adc); // 平均电压值
		   bat_volt  = GetBatVolt(aver_volt);
		   
		   ADC_DEBUG("adc=%d, v=%d.%03d V, bat=%d.%03d V, %ld\n", aver_adc, aver_volt / 1000, aver_volt % 1000, 
		   	           bat_volt / 1000, bat_volt % 1000, os_get_tick());
		   
		   total_sum = 0;
		   get_adc_count = 0;
		   
		   if(arg)
		   {
		       ((void (*)(uint16_t))(arg))(bat_volt);
		   }
		   BAT_CE_Set(SW_OPEN);  // 打开 TP4056 进行充电
		   os_timer_arm(&tBatChargeCloseTimer, SEC(30), 0);
		   return;
		}
	}
	else
	{
	  ADC_DEBUG("adc not end\n");
	  tick = 50;
	}
    STM32_ADC_SoftwareStartConvCmd(ADC1, ENABLE);   // 软件启动 AD 转换
    os_timer_arm(&tADCTimer, tick, 0);  
}

// ADC 启动初始化
void ADCDrv_Start(void)
{
    ADCDrv_NormalModeInit();
    STM32_ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5 );
	
	/* Start ADC1 Software Conversion */ 
    //STM32_ADC_SoftwareStartConvCmd(ADC1, ENABLE);  // 软件触发ADC

	//os_timer_setfn(&tADCTimer, ADCTimer_CallBack, NULL);
	//os_timer_arm(&tADCTimer, 100, 0);
}	

/*****************************
功能: 启动 ADC 测量电池剩余电量
             测量结束后执行(*end_exe_func)(uint16_t ) 回调函数
参数: end_exe_func: 测量结束后需要执行的操作
********************************/
void ADCDrv_StartBatteryMeasure(void  (*end_exe_func)(uint16_t arg))
{
    
    if(VIN_DETECT_Read())  
    {
       //ADC_DEBUG("USB Power is pluged\r\n");
       battery_is_charging |= USB_PLUGED_MASK;
    }
	#if DEBUG_ADC_EN
	is_first_sequ = E_TRUE;
    #endif
	
    ADCDrv_Start();

    os_timer_setfn(&tBatChargeCloseTimer, BatChargeCloseTimer_CallBack, NULL);
	os_timer_setfn(&tADCTimer, ADCTimer_CallBack, end_exe_func);

	BAT_CE_Set(SW_CLOSE);
	STM32_ADC_SoftwareStartConvCmd(ADC1, ENABLE);  // 软件启动 AD 转换
    os_timer_arm(&tADCTimer, 5, 0);
}


