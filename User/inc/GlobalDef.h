
#ifndef __GLOBAL_DEF_H__
#define  __GLOBAL_DEF_H__

#include "stm32f10x.h"

/******************************** ���뿪��begin *********************************/
#define SYS_DEBUG  //����

#ifdef SYS_DEBUG
#define UART_DEBUG  //���ڵ���
#endif

#include <stdio.h>
#include <stdarg.h>


#define DEBUG_VERSION   0



#define  UART_BLOCK  0   // ���ڲ�ѯ�ȴ���ʽ���
#define  UART_QUEUE  1   // �����жϷ�ʽ���

#if DEBUG_VERSION
#define os_printf(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#define SW_PRINTF_EN    0                  // printf ������������(1) �� ��Ӳ���������(0)
#define PRINTF_OUT_SEL  UART_BLOCK       // ���ڲ���: 0: ��ѯ�ȴ���ʽ���, ����; 1: �����жϷ�ʽ���: ������
#else
#define os_printf(...)
#endif

// ģ�����ʹ�ܿ���, 1: ʹ��; 0: ��ֹ
#define DEBUG_ADC_EN      0    // ADC ����
#define BAT_DEBUG_EN      0    // ��ص�������
#define EXTI_DEBUG_EN     0    // �ⲿ�жϵ���ʹ��(1), ��ֹ(0)
#define PM25_DEBUG_EN     0
#define TVOC_DEBUG_EN     0
#define RTC_DEBUG_EN      0
#define DEBUG_KEY_EN      0
#define SHT_DEBUG_EN      0  // ����ʹ��: 1; ��ֹ: 0
#define FAT_DEBUG_EN      0   // �ļ���������ʹ��
#define SYSTEM_DBG_EN     0   // ϵͳ�������



#define GIZ_MCU  1
#define GIZ_SOC  2


// ģʽʹ��
#define MODULE_USB_EN     1   // USB, FatFs ģʽʹ��(1), ��ֹ(0)
#define MODULE_LCD_EN     1   // �Ƿ�ʹ�� LCD Һ��ģ��
#define  GIZWITS_TYPE     0 //GIZ_MCU   // Gizwits ģ��ʹ��


#define os_error(err_info)  //{ printf("err: %s, %s, %d\n", ##err_info, __FILE__, __LINE__); }


#if 0
#define INSERT_DEBUG_INFO()   os_printf("debug, file = %s, line = %d\n", __FILE__, __LINE__)
#define INSERT_ERROR_INFO(err)   os_printf("error = %d, file = %s, line = %d\n", err, __FILE__, __LINE__)
#else
#define INSERT_DEBUG_INFO()   
#define INSERT_ERROR_INFO(err)   
#endif

#if DEBUG_VERSION
#define ASSERT_PARAM(expr) ((expr) ? (int)0 : INSERT_ERROR_INFO(0) )   // �������Ƿ�Ϊ��
#else
#define ASSERT_PARAM(expr) ((void)0)
#endif

#if  SYSTEM_DBG_EN
#define sys_printf(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#else
#define sys_printf(...)
#endif




// 1 �������� = 4KB
// APP1 BIN ���ݵ�ַ: 128KB, sector 0 - sector 31
#define FLASH_APP1_START_SECTOR  0L

// APP2 BIN ���ݵ�ַ: 128KB, sector 32 - sector 63
#define FLASH_APP2_START_SECTOR  32L

// ϵͳ����������ʼ������ַ, sector 64 ����, ��С 4KB
#define FLASH_SYS_ENV_START_SECTOR    64L  


/* Define the address from where user application will be loaded.
   Note: the 1st sector 0x08000000-0x080023FF is reserved for the IAP code */
#define BOOT_START_ADDR         ((uint32_t)0x08000000)
#define BOOT_FLASH_END_ADDR     ((uint32_t)0x08001FFF)
#define BOOT_FLASH_SIZE         (BOOT_FLASH_END_ADDR - BOOT_START_ADDR + 1)

#define APP_START_ADDR          ((uint32_t)(BOOT_FLASH_END_ADDR + 1))  
#define APP_FLASH_END_ADDR      ((uint32_t)0x0801FFFF)  // medium desity 128KB 
#define APP_FLASH_SIZE          (APP_FLASH_END_ADDR - APP_START_ADDR + 1)
#define APP_VECTOR_OFFSET       ((uint32_t)BOOT_FLASH_SIZE)

/* Define the address from where user application will be loaded.
   Note: the 1st sector 0x08000000-0x08002FFF is reserved for the IAP code */
#define APPLICATION_ADDRESS   (uint32_t)APP_START_ADDR 

/* End of the Flash address */
#define USER_FLASH_END_ADDRESS        APP_FLASH_END_ADDR
/* Define the user application size */
#define USER_FLASH_SIZE   (USER_FLASH_END_ADDRESS - APPLICATION_ADDRESS + 1)

#define FREQ_512KHz       (512000L)
#define FREQ_1MHz        (1000000L)
#define FREQ_2MHz        (2000000L)
#define FREQ_8MHz        (8000000L)
#define FREQ_16MHz      (16000000L)
#define FREQ_24MHz      (24000000L)
#define FREQ_48MHz      (48000000L)



#define CPU_CLOCK       FREQ_48MHz  // CPU ʱ��


#define HSIENABLE             1       // HSI ʱ��ʹ��, �ڲ� 8 M
#define SYS_USING_BOOTLOADER  1      // ʹ�� bootloader
#define OS_SYS_TICK_EN                // TIM1 10 ms�ж�һ�� ��Ϊ Sys Tick
#define OS_PER_TICK_MS   (10)         // ÿ��tick��ʱ��: 10ms

//#define  USE_STD_LIB    // ʹ�ñ�׼�⺯��

#ifndef USE_STD_LIB
#include "RegLib.h"   // ʹ�üĴ�����
#endif

#define MCU_TYPE_STM32F10X   0
#define MCU_TYPE_GD32F10X    1

// MCU ��������ѡ��
#define MCU_TYPE_SEL   MCU_TYPE_GD32F10X  


#define  FLASH_DISK_EN    1   // SPI FLASH DISK �洢ʹ��(1)
#define  ROM_DISK_EN      0   // ROM FLASH DISK �洢ʹ��
#define  SD_DISK_EN       0   // SD ���洢ʹ��

#define  MAX_LUN          0   // MAX_LUN + 1 �����ƶ����� SD��+FLASH + ROM FLASH


// �Ƿ�ʹ�� TVOC ����У�� ��ȩֵ
#define  USE_TVOC_CAL    0




typedef enum
{
   FLASH_DISK = 0,
   ROM_DISK   = 1,
   SD_DISK    = 2,
}DISK_ENUM;




/******************************** ���뿪��end *********************************/

/*********************************�ڴ����begin ******************************/
//�û��ɸ�����Ҫ����


/*********************************�ڴ����end ******************************/
/***************************** ���Ͷ���begin  ************************************/
typedef enum
{
   INPUT  =  0,
   OUTPUT =  1,
}E_IO_DIR;   // IO ��������뷽��


typedef enum
{
   SW_OPEN  = 0,
   SW_CLOSE = 1,
}E_SW_STATE;  // IO ����״̬

typedef enum
{
    E_FALSE = 0,
	E_TRUE = !E_FALSE
}E_BOOL;

typedef enum
{
   APP_SUCCESS = 0,
   APP_FAILED = 1,
   APP_NULL = 0xFF,
}E_RESULT;

typedef enum
{
   SYS_SUCCESS = 0,
   SYS_FAILED  = 1,
}SYS_RESULT;

typedef enum
{
   LESS_STATE = 0,  //С��
   EQUAL_STATE,     //����
   MORE_STATE,      //����
}Compare_State;  //�Ƚ�״ֵ̬
//�ź�����ֵ
typedef enum
{
    UART_IDLE = 0,  //����
    UART_BUSY = !UART_IDLE
}Sema_State;
typedef ErrorStatus UartStatus; 
typedef uint8_t  QueueMemType;       //���л������洢����
typedef uint16_t QueuePointerType;  //����ָ������
typedef struct
{
   QueueMemType *Base;  //���л������洢λ��
   QueuePointerType QHead;  //����ͷָ��, �����д������
   QueuePointerType QTail;  //����βָ��, �Է�����˵,����������򴮿ڷ���; �Խ�����˵, ����Ǵӽ��ն�����ȡ
}T_UartQueue;
typedef struct struc_rtc_time
{
   uint8_t year;
   uint8_t month;
   uint8_t day;
   uint8_t week;
   uint8_t hour;
   uint8_t min;
   uint8_t sec;
   uint8_t century;
}T_RTC_TIME;
/***************************** ���Ͷ���end  **************************************************************/

#define LITTLE_ENDIAN   0
#define BIG_ENDIAN      1


#define PMS5003      0    // ������ȩ����
#define PMS5003S     1     // ����ȩ����
#define HCHO_SENSOR  2
#define SENSOR_SELECT  HCHO_SENSOR


#define HCHO_EN     1
#define PM25_EN     1

/********************************* �� ����begin **********************************************/
#define NOP()   __NOP()
#define GLOBAL_DISABLE_IRQ()      __disable_irq()  //Global interrupt disable
#define GLOBAL_ENABLE_IRQ()       __enable_irq()   //Global interrupt enable

#define WEAK_ATTR   __attribute__((weak))

#define READ_REG_32_BIT(reg,  b)      ((uint32_t)((reg)&((uint32_t)b)))      //b����Ϊ����, regΪ32 bit �Ĵ���
#define CLEAR_REG_32_BIT(reg, b)      ((reg)&=(uint32_t)(~((uint32_t)b)))   //b����Ϊ����, regΪ32 bit �Ĵ���
#define SET_REG_32_BIT(reg,   b)      ((reg)|=(uint32_t)(b))                  //b����Ϊ����, regΪ32 bit �Ĵ���

#define READ_REG_8_BIT(reg,b)     ((uint8_t)((reg)&((uint8_t)b)))           //b����Ϊ8bit����, regΪ�Ĵ���
#define CLEAR_REG_8_BIT(reg,b)    ((reg)&=(uint8_t)(~((uint8_t)b)))        //b����Ϊ8bit����, regΪ�Ĵ���
#define SET_REG_8_BIT(reg,b)      ((reg)|=(uint8_t)(b))                      //b����Ϊ8bit����, regΪ�Ĵ���

// FLASH �洢�����ĳ���: ��4�ֽڶ���
#define WORD_ALIGNED_LEN(len)   ( (sizeof(len) + sizeof(int) - 1) &  ~(sizeof(int) - 1) ) 

/********************************* �� ����end  **********************************************/
#endif  // __GLOBAL_DEF_H__


