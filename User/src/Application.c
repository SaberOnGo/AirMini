
#include "stm32f10x.h"
#include "TimerManager.h"
#include "os_timer.h"
#include "os_global.h"
#include "Uart_drv.h"
#include "uart_queue.h"
#include "LCD1602_Drv.h"
#include "delay.h"
#include "PM25Sensor.h"
#include "Key_Drv.h"
#include "LED_Drv.h"
#include "bsp.h"
#include "SDRR.h"
#include "board_version.h"



#ifndef USE_STD_LIB
#define REG_HSI_CR_HSION_BB  ((uint32_t)0x42420000)
#define REG_CR_PLLON_BB      ((uint32_t)0x42420060)


#define ACR_LATENCY_Mask         ((uint32_t)0x00000038)
#define ACR_PRFTBE_Mask          ((uint32_t)0xFFFFFFEF)

#define CFGR_PPRE1_Reset_Mask     ((uint32_t)0xFFFFF8FF)
#define CFGR_PPRE2_Reset_Mask     ((uint32_t)0xFFFFC7FF)
#define CFGR_HPRE_Reset_Mask      ((uint32_t)0xFFFFFF0F)

#define CFGR_SW_Mask              ((uint32_t)0xFFFFFFFC)
#define CFGR_SWS_Mask             ((uint32_t)0x0000000C)


/* CFGR register bit mask */
#if defined (STM32F10X_LD_VL) || defined (STM32F10X_MD_VL) || defined (STM32F10X_HD_VL) || defined (STM32F10X_CL) 
 #define CFGR_PLL_Mask            ((uint32_t)0xFFC2FFFF)
#else
 #define CFGR_PLL_Mask            ((uint32_t)0xFFC0FFFF)
#endif /* STM32F10X_CL */ 


// 参数: state: ENABLE  or DISABLE
#define STM32_RCC_HSICmd(state) \
	(*(__IO uint32_t *) REG_HSI_CR_HSION_BB = (uint32_t)state)  //打开内部高速时钟

#define STM32_FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer) \
	(FLASH->ACR |= (FLASH->ACR & ACR_PRFTBE_Mask) + FLASH_PrefetchBuffer)

#define  STM32_FLASH_SetLatency(FLASH_Latency) \
	(FLASH->ACR  = (FLASH->ACR & ACR_LATENCY_Mask) | FLASH_Latency)

#define  STM32_RCC_HCLKConfig(RCC_SYSCLK) \
	(RCC->CFGR	= (RCC->CFGR & CFGR_HPRE_Reset_Mask ) | RCC_SYSCLK)

#define  STM32_RCC_PCLK2Config(RCC_HCLK) \
	(RCC->CFGR   = (RCC->CFGR & CFGR_PPRE2_Reset_Mask) | (RCC_HCLK << 3))

#define  STM32_RCC_PCLK1Config(RCC_HCLK) \
	(RCC->CFGR	= (RCC->CFGR & CFGR_PPRE1_Reset_Mask) | (RCC_HCLK))

#define   STM32_RCC_PLLConfig(RCC_PLLSource, RCC_PLLMul) \
	(RCC->CFGR = (RCC->CFGR & CFGR_PLL_Mask) | RCC_PLLSource | RCC_PLLMul) 

// 参数: state: ENABLE  or DISABLE
#define STM32_RCC_PLLCmd(state) \
	(*(__IO uint32_t *) REG_CR_PLLON_BB = (uint32_t)state)


#define STM32_RCC_SYSCLKConfig(RCC_SYSCLKSource) \
	(RCC->CFGR = (RCC->CFGR & CFGR_SW_Mask) | RCC_SYSCLKSource)



#endif

/*******************************************************************************
* Function Name  : RCC_Configuration
* Description    : 配置不同的系统时钟
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SysClockConfig(void)
{

#ifdef USE_STD_LIB

   #if (! HSIENABLE)
		ErrorStatus HSEStartUpStatus;
   #endif

        /* RCC system reset(for debug purpose) */
        RCC_DeInit();

    #if    HSIENABLE                //当使用HSI高速内部时钟作为系统时钟时
        RCC_HSICmd(ENABLE);        //打开内部高速时钟
        
        //等待HSI准备好
        #if USE_STD_LIB
        while(RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET);
		#else
        while(! (READ_REG_32_BIT(RCC->CR, RCC_CR_HSIRDY)));
		#endif

        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);        //开启FLASH预取指功能
        //FLASH时序控制
        //推荐值:SYSCLK = 0~24MHz   Latency=0
        //       SYSCLK = 24~48MHz  Latency=1
        //       SYSCLK = 48~72MHz  Latency=2
        FLASH_SetLatency(FLASH_Latency_2);
        RCC_HCLKConfig(RCC_SYSCLK_Div1);        //设置HCLK(AHB时钟)=SYSCLK
        RCC_PCLK2Config(RCC_HCLK_Div1);                //PCLK2(APB2) = HCLK
        RCC_PCLK1Config(RCC_HCLK_Div2);                //PCLK1(APB1) = HCLK / 2= 24 MHz

        //PLL设置 SYSCLK: HSI / 2 * 12 = 4*12 = 48MHz
        RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_12);
        //启动PLL
        RCC_PLLCmd(ENABLE);//如果PLL被用于系统时钟,不能被DISABLE
        //等待PLL稳定
        while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET){;}

        //设置系统时钟SYSCLK = PLL输出
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

        //等待PLL成功用作于系统时钟的时钟源,并等待稳定
        // 0x00:HSI作为系统时钟
        // 0x04:HSE作为系统时钟
        // 0x08:PLL作为系统时钟
        while(RCC_GetSYSCLKSource() != 0x08);
		
		SystemCoreClock = CPU_CLOCK;
    #else
        /* Enable HSE */
        RCC_HSEConfig(RCC_HSE_ON);

        /* Wait till HSE is ready */
        HSEStartUpStatus = RCC_WaitForHSEStartUp();

        if(HSEStartUpStatus == SUCCESS)
        {
                /* HCLK = SYSCLK */
                RCC_HCLKConfig(RCC_SYSCLK_Div1);
                /* PCLK2 = HCLK */
                RCC_PCLK2Config(RCC_HCLK_Div1);
                /* PCLK1 = HCLK/2 */
                RCC_PCLK1Config(RCC_HCLK_Div2);
                /* Flash 2 wait state */
                FLASH_SetLatency(FLASH_Latency_2);
                /* Enable Prefetch Buffer */
                FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
                /* PLLCLK = 8MHz * 6 = 48 MHz */
                RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_6);
                /* Enable PLL */
                RCC_PLLCmd(ENABLE);
                /* Wait till PLL is ready */
                while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
                {
                }
                /* Select PLL as system clock source */
                RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
                /* Wait till PLL is used as system clock source */
                while(RCC_GetSYSCLKSource() != 0x08);
				SystemCoreClock = CPU_CLOCK;
        }
   #endif
        /* Enable DMA1 clock */
       // RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
        /* Enable ADC1E clock */
        //RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);        
        /* TIM4 clock source enable */
       // RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
        /* Enable GPIOA, GPIOB, GPIOC, GPIOD and AFIO clocks */
       // RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);

#else   // #ifdef USE_STD_LIB

     RCC_DeInit();
     STM32_RCC_HSICmd(ENABLE);  //打开内部高速时钟
     while(! (READ_REG_32_BIT(RCC->CR, RCC_CR_HSIRDY)));
     STM32_FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);    //开启FLASH预取指功能
     STM32_FLASH_SetLatency(FLASH_Latency_2);                          // FLASH时序控制, SYSCLK = 48~72MHz  Latency=2
     STM32_RCC_HCLKConfig(RCC_SYSCLK_Div1);                            // 设置HCLK(AHB时钟)=SYSCLK
     STM32_RCC_PCLK2Config(RCC_HCLK_Div1);                             // PCLK2(APB2) = HCLK
     STM32_RCC_PCLK1Config(RCC_HCLK_Div2);                             // PCLK1(APB1) = HCLK / 2= 24 MHz

     //PLL设置 SYSCLK: HSI / 2 * 12 = 4*12 = 48MHz
     STM32_RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_12);
     STM32_RCC_PLLCmd(ENABLE); // 启动PLL, 如果PLL被用于系统时钟,不能被DISABLE

	 //等待PLL稳定
	 while( ! READ_REG_32_BIT(RCC->CR, RCC_CR_PLLRDY));
	 //设置系统时钟SYSCLK = PLL输出
     STM32_RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
	 
	 //等待PLL成功用作于系统时钟的时钟源,并等待稳定
     // 0x00:HSI作为系统时钟
     // 0x04:HSE作为系统时钟
     // 0x08:PLL作为系统时钟
	 while( ((uint8_t)READ_REG_32_BIT(RCC->CFGR, CFGR_SWS_Mask)) != 0x08);
	 SystemCoreClock = CPU_CLOCK;	 
#endif

}

void PowerCtrl_SetClock(void)
{
     RCC_DeInit();
     STM32_RCC_HSICmd(ENABLE);  //打开内部高速时钟
     while(! (READ_REG_32_BIT(RCC->CR, RCC_CR_HSIRDY)));
     STM32_FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);    //开启FLASH预取指功能
     STM32_FLASH_SetLatency(FLASH_Latency_2);                          // FLASH时序控制, SYSCLK = 48~72MHz  Latency=2
     STM32_RCC_HCLKConfig(RCC_SYSCLK_Div8);                            // 设置HCLK(AHB时钟)=SYSCLK
     STM32_RCC_PCLK2Config(RCC_HCLK_Div8);                             // PCLK2(APB2) = HCLK
     STM32_RCC_PCLK1Config(RCC_HCLK_Div8);                             // PCLK1(APB1) = HCLK / 2= 4 MHz

     //PLL设置 SYSCLK: HSI / 2 * 12 = 4*12 = 48MHz
     //STM32_RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_12);
     //STM32_RCC_PLLCmd(ENABLE); // 启动PLL, 如果PLL被用于系统时钟,不能被DISABLE

	 //等待PLL稳定
	// while( ! READ_REG_32_BIT(RCC->CR, RCC_CR_PLLRDY));
	 //设置系统时钟SYSCLK = PLL输出
     STM32_RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
	 
	 //等待PLL成功用作于系统时钟的时钟源,并等待稳定
     // 0x00:HSI作为系统时钟
     // 0x04:HSE作为系统时钟
     // 0x08:PLL作为系统时钟
	 while( ((uint8_t)READ_REG_32_BIT(RCC->CFGR, CFGR_SWS_Mask)) != 0x00);
	 SystemCoreClock = FREQ_1MHz;	 
}

/*******************************************************************************
* Function Name  : Set_USBClock
* Description    : Configures USB Clock input (48MHz)
* Input          : None.
* Return         : None.
*******************************************************************************/
void Set_USBClock(void)
{
    /* USBCLK = PLLCLK  = 48 MHz */
	STM32_RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_Div1);

    /* Enable USB clock */
	STM32_RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
}

// 外设时钟初始化
static void RCC_PeriphInit(void)
{
    #if USE_STD_LIB
    RCC_APB2PeriphClockCmd(  RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC
		                      | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);  // 禁止JTAG
    //GPIO_PinRemapConfig(GPIO_Remap_PD01, ENABLE);  // 使能PD0 PD1
    #else
	STM32_RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC
		                      | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);
    //SET_REG_32_BIT(AFIO->MAPR, AFIO_MAPR_PD01_REMAP);   // 使能 PD0, PD1
	SET_REG_32_BIT(AFIO->MAPR, AFIO_MAPR_SWJ_CFG_JTAGDISABLE); 
	#endif
}

#include "sfud_demo.h"
#include "UsbApp.h"
#include "FatFs_Demo.h"
#include "ff.h"
#include "DHT11.h"
#include "stm32f1_temp_sensor.h"

#define  SOFT_VERSION    "HV1.125 SV1.47.5"

void AppVersionInfo(uint8_t display_mode)
{
   char time_str[16];
   char ver_str[16];
   
   os_memset(time_str,  0, sizeof(time_str));
   os_memset(ver_str,   0, sizeof(ver_str));
   os_snprintf(time_str, sizeof(time_str),  "%s ", __DATE__);
   os_strncpy(ver_str, SOFT_VERSION, sizeof(ver_str));
   
   LCD1602_WriteString(0, 0, (const uint8_t *)ver_str);
   LCD1602_WriteString(1, 0, (const uint8_t *)time_str);
}

#define NVIC_SET_VECTOR_TABLE(NVIC_VectTab, Offset) (SCB->VTOR = NVIC_VectTab | (Offset & (uint32_t)0x1FFFFF80))

/**
 * NVIC Configuration
 */
void NVIC_Configuration(void)
{
#if  defined(VECT_TAB_RAM)
    // Set the Vector Table base location at 0x20000000
    NVIC_SET_VECTOR_TABLE(NVIC_VectTab_RAM, 0x00);
#else
    #if SYS_USING_BOOTLOADER
    NVIC_SET_VECTOR_TABLE(NVIC_VectTab_FLASH, APP_VECTOR_OFFSET);
    #else
	 /* Set the Vector Table base location at 0x08000000 */
    NVIC_SET_VECTOR_TABLE(NVIC_VectTab_FLASH, 0x0);
	#endif
#endif
}

#include "sfud_demo.h"
#include "UsbApp.h"
#include "FatFs_Demo.h"
#include "ff.h"
#include "SHT20.h"
#include "TVOCSensor.h"
#include "stm32f1_temp_sensor.h"
#include "ADC_Drv.h"
#include "BatteryLevel.h"
#include "ExtiDrv.h"
#include "RTCDrv.h"



void AppInit(void)
{
   //RCC_ClocksTypeDef rcc_clocks;
   
   
   GLOBAL_DISABLE_IRQ();
   NVIC_Configuration();
   
   SysClockConfig();  
   SysTick_Init();
   RCC_PeriphInit();

   //Uart_Q_Init();
   USART2_Init(FREQ_24MHz, 115200);
   
   
   
   // 主板管脚初始化
   Board_GpioInit();
   
   BSP_Init();
  
   #if 0
   RCC_GetClocksFreq(&rcc_clocks);  // 读取系统时钟
   sys_printf("SysClk = %d, HCLK = %d, PCLK1 = %d, PCLK2 = %d, ADCLK = %d MHz\r\n", rcc_clocks.SYSCLK_Frequency / FREQ_1MHz,
   	               rcc_clocks.HCLK_Frequency / FREQ_1MHz, rcc_clocks.PCLK1_Frequency / FREQ_1MHz, 
   	               rcc_clocks.PCLK2_Frequency / FREQ_1MHz, rcc_clocks.ADCCLK_Frequency / FREQ_1MHz);
   sys_printf("app version: %s %s %s\r\n", SOFT_VERSION, __DATE__, __TIME__);
   #else
   sys_printf("%s %s %s\r\n", SOFT_VERSION, __DATE__, __TIME__);
   #endif
   
   
#if MODULE_USB_EN   
   usb_main();
   FatFs_Demo();
   SDRR_Init();
#endif
   // ITempSensor_Init();
   
   RTCDrv_Init();

#if MODULE_USB_EN
   FILE_ReadConfig();
#endif
   
   USART1_Init(FREQ_48MHz, 9600);  // PM2.5接收口
   
   USART3_Init(FREQ_24MHz, 9600);  // HCHO 接收串口
   GLOBAL_ENABLE_IRQ();

   LCD1602_Init();
   
   
   // 注意其他外设的初始化应该放在FatFs文件系统初始化之后
   BatLev_Init();

   
   
   PM25_Init();
   TVOC_Init();
   SHT20_Init();
   key_gpio_init();
   ExtiDrv_Init();
   
      
  // LED_Init();
  // LED_Flash(8);
}

