
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
#define REG_CR_PLLON_BB            ((uint32_t)0x42420060)


#define ACR_LATENCY_Mask         ((uint32_t)0x00000038)
#define ACR_PRFTBE_Mask           ((uint32_t)0xFFFFFFEF)

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


// ����: state: ENABLE  or DISABLE
#define STM32_RCC_HSICmd(state) \
	(*(__IO uint32_t *) REG_HSI_CR_HSION_BB = (uint32_t)state)  //���ڲ�����ʱ��

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

// ����: state: ENABLE  or DISABLE
#define STM32_RCC_PLLCmd(state) \
	(*(__IO uint32_t *) REG_CR_PLLON_BB = (uint32_t)state)


#define STM32_RCC_SYSCLKConfig(RCC_SYSCLKSource) \
	(RCC->CFGR = (RCC->CFGR & CFGR_SW_Mask) | RCC_SYSCLKSource)



#endif

/*******************************************************************************
* Function Name  : RCC_Configuration
* Description    : ���ò�ͬ��ϵͳʱ��
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

    #if    HSIENABLE                //��ʹ��HSI�����ڲ�ʱ����Ϊϵͳʱ��ʱ
        RCC_HSICmd(ENABLE);        //���ڲ�����ʱ��
        
        //�ȴ�HSI׼����
        #if USE_STD_LIB
        while(RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET);
		#else
        while(! (READ_REG_32_BIT(RCC->CR, RCC_CR_HSIRDY)));
		#endif

        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);        //����FLASHԤȡָ����
        //FLASHʱ�����
        //�Ƽ�ֵ:SYSCLK = 0~24MHz   Latency=0
        //       SYSCLK = 24~48MHz  Latency=1
        //       SYSCLK = 48~72MHz  Latency=2
        FLASH_SetLatency(FLASH_Latency_2);
        RCC_HCLKConfig(RCC_SYSCLK_Div1);        //����HCLK(AHBʱ��)=SYSCLK
        RCC_PCLK2Config(RCC_HCLK_Div1);                //PCLK2(APB2) = HCLK
        RCC_PCLK1Config(RCC_HCLK_Div2);                //PCLK1(APB1) = HCLK / 2= 24 MHz

        //PLL���� SYSCLK: HSI / 2 * 12 = 4*12 = 48MHz
        RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_12);
        //����PLL
        RCC_PLLCmd(ENABLE);//���PLL������ϵͳʱ��,���ܱ�DISABLE
        //�ȴ�PLL�ȶ�
        while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET){;}

        //����ϵͳʱ��SYSCLK = PLL���
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

        //�ȴ�PLL�ɹ�������ϵͳʱ�ӵ�ʱ��Դ,���ȴ��ȶ�
        // 0x00:HSI��Ϊϵͳʱ��
        // 0x04:HSE��Ϊϵͳʱ��
        // 0x08:PLL��Ϊϵͳʱ��
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
     STM32_RCC_HSICmd(ENABLE);  //���ڲ�����ʱ��
     while(! (READ_REG_32_BIT(RCC->CR, RCC_CR_HSIRDY)));
     STM32_FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);    //����FLASHԤȡָ����
     STM32_FLASH_SetLatency(FLASH_Latency_2);                          // FLASHʱ�����, SYSCLK = 48~72MHz  Latency=2
     STM32_RCC_HCLKConfig(RCC_SYSCLK_Div1);                            // ����HCLK(AHBʱ��)=SYSCLK
     STM32_RCC_PCLK2Config(RCC_HCLK_Div1);                             // PCLK2(APB2) = HCLK
     STM32_RCC_PCLK1Config(RCC_HCLK_Div2);                             // PCLK1(APB1) = HCLK / 2= 24 MHz

     //PLL���� SYSCLK: HSI / 2 * 12 = 4*12 = 48MHz
     STM32_RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_12);
     STM32_RCC_PLLCmd(ENABLE); // ����PLL, ���PLL������ϵͳʱ��,���ܱ�DISABLE

	 //�ȴ�PLL�ȶ�
	 while( ! READ_REG_32_BIT(RCC->CR, RCC_CR_PLLRDY));
	 //����ϵͳʱ��SYSCLK = PLL���
     STM32_RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
	 
	 //�ȴ�PLL�ɹ�������ϵͳʱ�ӵ�ʱ��Դ,���ȴ��ȶ�
     // 0x00:HSI��Ϊϵͳʱ��
     // 0x04:HSE��Ϊϵͳʱ��
     // 0x08:PLL��Ϊϵͳʱ��
	 while( ((uint8_t)READ_REG_32_BIT(RCC->CFGR, CFGR_SWS_Mask)) != 0x08);
	 SystemCoreClock = CPU_CLOCK;	 
#endif

}

void PowerCtrl_SetClock(void)
{
     RCC_DeInit();
     STM32_RCC_HSICmd(ENABLE);  //���ڲ�����ʱ��
     while(! (READ_REG_32_BIT(RCC->CR, RCC_CR_HSIRDY)));
     STM32_FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);    //����FLASHԤȡָ����
     STM32_FLASH_SetLatency(FLASH_Latency_2);                          // FLASHʱ�����, SYSCLK = 48~72MHz  Latency=2
     STM32_RCC_HCLKConfig(RCC_SYSCLK_Div8);                            // ����HCLK(AHBʱ��)=SYSCLK
     STM32_RCC_PCLK2Config(RCC_HCLK_Div8);                             // PCLK2(APB2) = HCLK
     STM32_RCC_PCLK1Config(RCC_HCLK_Div8);                             // PCLK1(APB1) = HCLK / 2= 4 MHz

     //PLL���� SYSCLK: HSI / 2 * 12 = 4*12 = 48MHz
     //STM32_RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_12);
     //STM32_RCC_PLLCmd(ENABLE); // ����PLL, ���PLL������ϵͳʱ��,���ܱ�DISABLE

	 //�ȴ�PLL�ȶ�
	// while( ! READ_REG_32_BIT(RCC->CR, RCC_CR_PLLRDY));
	 //����ϵͳʱ��SYSCLK = PLL���
     STM32_RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
	 
	 //�ȴ�PLL�ɹ�������ϵͳʱ�ӵ�ʱ��Դ,���ȴ��ȶ�
     // 0x00:HSI��Ϊϵͳʱ��
     // 0x04:HSE��Ϊϵͳʱ��
     // 0x08:PLL��Ϊϵͳʱ��
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

// ����ʱ�ӳ�ʼ��
static void RCC_PeriphInit(void)
{
    #if USE_STD_LIB
    RCC_APB2PeriphClockCmd(  RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC
		                      | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);  // ��ֹJTAG
    //GPIO_PinRemapConfig(GPIO_Remap_PD01, ENABLE);  // ʹ��PD0 PD1
    #else
	STM32_RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC
		                      | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);
    //SET_REG_32_BIT(AFIO->MAPR, AFIO_MAPR_PD01_REMAP);   // ʹ�� PD0, PD1

       #if GIZWITS_TYPE
	SET_REG_32_BIT(AFIO->MAPR, AFIO_MAPR_SWJ_CFG_DISABLE); 
	#else
       SET_REG_32_BIT(AFIO->MAPR, AFIO_MAPR_SWJ_CFG_JTAGDISABLE); 
	#endif
	
	#endif
}

#include "sfud_demo.h"
#include "UsbApp.h"
#include "FatFs_Demo.h"
#include "ff.h"
#include "DHT11.h"
#include "stm32f1_temp_sensor.h"

#define  SOFT_VERSION    "HV1.127 SV1.51.5"

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
#include "gizwits_port.h"


void AppInit(void)
{
   //RCC_ClocksTypeDef rcc_clocks;
   
   
   //GLOBAL_DISABLE_IRQ();
   NVIC_Configuration();
   
   SysClockConfig();  
   SysTick_Init();
   RCC_PeriphInit();

   //Uart_Q_Init();
   USART2_Init(FREQ_24MHz, 115200);
   
#if GIZWITS_TYPE
       gizwits_user_init();  
#endif
   
   // ����ܽų�ʼ��
   Board_GpioInit();
   
   BSP_Init();
  
   #if 0
   RCC_GetClocksFreq(&rcc_clocks);  // ��ȡϵͳʱ��
   sys_printf("SysClk = %d, HCLK = %d, PCLK1 = %d, PCLK2 = %d, ADCLK = %d MHz\r\n", rcc_clocks.SYSCLK_Frequency / FREQ_1MHz,
   	               rcc_clocks.HCLK_Frequency / FREQ_1MHz, rcc_clocks.PCLK1_Frequency / FREQ_1MHz, 
   	               rcc_clocks.PCLK2_Frequency / FREQ_1MHz, rcc_clocks.ADCCLK_Frequency / FREQ_1MHz);
   sys_printf("app version: %s %s %s\r\n", SOFT_VERSION, __DATE__, __TIME__);
   #else
   sys_printf("%s %s %s\r\n", SOFT_VERSION, __DATE__, __TIME__);
   #endif
   
   
#if MODULE_USB_EN   
   FatFs_Demo();
   delay_ms(1000);
   usb_main();
   SDRR_Init();
#endif
   // ITempSensor_Init();
   
   RTCDrv_Init();

#if MODULE_USB_EN
   FILE_ReadConfig();
#endif
   
   USART1_Init(FREQ_48MHz, 9600);  // PM2.5���տ�
   
   USART3_Init(FREQ_24MHz, 9600);  // HCHO ���մ���
   //GLOBAL_ENABLE_IRQ();

   LCD1602_Init();
   
   
   // ע����������ĳ�ʼ��Ӧ�÷���FatFs�ļ�ϵͳ��ʼ��֮��
   BatLev_Init();

   
   
   PM25_Init();
   TVOC_Init();
   SHT20_Init();
   key_gpio_init();
   ExtiDrv_Init();
   
      
  // LED_Init();
  // LED_Flash(8);
}

