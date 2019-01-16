
#ifndef __GLOBAL_DEF_H__
#define  __GLOBAL_DEF_H__

#include "stm32f10x.h"

/******************************** 编译开关begin *********************************/
#define SYS_DEBUG  //调试

#ifdef SYS_DEBUG
#define UART_DEBUG  //串口调试
#endif

#include <stdio.h>
#include <stdarg.h>


#define DEBUG_VERSION   0



#define  UART_BLOCK  0   // 串口查询等待方式输出
#define  UART_QUEUE  1   // 队列中断方式输出

#if DEBUG_VERSION
#define os_printf(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#define SW_PRINTF_EN    0                  // printf 由软件串口输出(1) 或 由硬件串口输出(0)
#define PRINTF_OUT_SEL  UART_BLOCK       // 串口采用: 0: 查询等待方式输出, 阻塞; 1: 队列中断方式输出: 不阻塞
#else
#define os_printf(...)
#endif

// 模块调试使能开关, 1: 使能; 0: 禁止
#define DEBUG_ADC_EN      0    // ADC 调试
#define BAT_DEBUG_EN      0    // 电池电量调试
#define EXTI_DEBUG_EN     0    // 外部中断调试使能(1), 禁止(0)
#define PM25_DEBUG_EN     0
#define TVOC_DEBUG_EN     0
#define RTC_DEBUG_EN      0
#define DEBUG_KEY_EN      0
#define SHT_DEBUG_EN      0  // 调试使能: 1; 禁止: 0
#define FAT_DEBUG_EN      0   // 文件操作调试使能
#define SYSTEM_DBG_EN     0   // 系统调试输出



#define GIZ_MCU  1
#define GIZ_SOC  2


// 模式使能
#define MODULE_USB_EN     1   // USB, FatFs 模式使能(1), 禁止(0)
#define MODULE_LCD_EN     1   // 是否使能 LCD 液晶模块
#define  GIZWITS_TYPE     0 //GIZ_MCU   // Gizwits 模块使能


#define os_error(err_info)  //{ printf("err: %s, %s, %d\n", ##err_info, __FILE__, __LINE__); }


#if 0
#define INSERT_DEBUG_INFO()   os_printf("debug, file = %s, line = %d\n", __FILE__, __LINE__)
#define INSERT_ERROR_INFO(err)   os_printf("error = %d, file = %s, line = %d\n", err, __FILE__, __LINE__)
#else
#define INSERT_DEBUG_INFO()   
#define INSERT_ERROR_INFO(err)   
#endif

#if DEBUG_VERSION
#define ASSERT_PARAM(expr) ((expr) ? (int)0 : INSERT_ERROR_INFO(0) )   // 检查参数是否为空
#else
#define ASSERT_PARAM(expr) ((void)0)
#endif

#if  SYSTEM_DBG_EN
#define sys_printf(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#else
#define sys_printf(...)
#endif




// 1 物理扇区 = 4KB
// APP1 BIN 备份地址: 128KB, sector 0 - sector 31
#define FLASH_APP1_START_SECTOR  0L

// APP2 BIN 备份地址: 128KB, sector 32 - sector 63
#define FLASH_APP2_START_SECTOR  32L

// 系统环境参数起始扇区地址, sector 64 扇区, 大小 4KB
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



#define CPU_CLOCK       FREQ_48MHz  // CPU 时钟


#define HSIENABLE             1       // HSI 时钟使能, 内部 8 M
#define SYS_USING_BOOTLOADER  1      // 使能 bootloader
#define OS_SYS_TICK_EN                // TIM1 10 ms中断一次 作为 Sys Tick
#define OS_PER_TICK_MS   (10)         // 每个tick的时间: 10ms

//#define  USE_STD_LIB    // 使用标准库函数

#ifndef USE_STD_LIB
#include "RegLib.h"   // 使用寄存器库
#endif

#define MCU_TYPE_STM32F10X   0
#define MCU_TYPE_GD32F10X    1

// MCU 兼容类型选择
#define MCU_TYPE_SEL   MCU_TYPE_GD32F10X  


#define  FLASH_DISK_EN    1   // SPI FLASH DISK 存储使能(1)
#define  ROM_DISK_EN      0   // ROM FLASH DISK 存储使能
#define  SD_DISK_EN       0   // SD 卡存储使能

#define  MAX_LUN          0   // MAX_LUN + 1 个可移动磁盘 SD卡+FLASH + ROM FLASH


// 是否使用 TVOC 进行校正 甲醛值
#define  USE_TVOC_CAL    0




typedef enum
{
   FLASH_DISK = 0,
   ROM_DISK   = 1,
   SD_DISK    = 2,
}DISK_ENUM;




/******************************** 编译开关end *********************************/

/*********************************内存分配begin ******************************/
//用户可根据需要更改


/*********************************内存分配end ******************************/
/***************************** 类型定义begin  ************************************/
typedef enum
{
   INPUT  =  0,
   OUTPUT =  1,
}E_IO_DIR;   // IO 输出或输入方向


typedef enum
{
   SW_OPEN  = 0,
   SW_CLOSE = 1,
}E_SW_STATE;  // IO 开关状态

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
   LESS_STATE = 0,  //小于
   EQUAL_STATE,     //等于
   MORE_STATE,      //大于
}Compare_State;  //比较状态值
//信号量的值
typedef enum
{
    UART_IDLE = 0,  //解锁
    UART_BUSY = !UART_IDLE
}Sema_State;
typedef ErrorStatus UartStatus; 
typedef uint8_t  QueueMemType;       //队列缓冲区存储类型
typedef uint16_t QueuePointerType;  //队列指针类型
typedef struct
{
   QueueMemType *Base;  //队列缓冲区存储位置
   QueuePointerType QHead;  //队列头指针, 向队列写入数据
   QueuePointerType QTail;  //队列尾指针, 对发送来说,输出数据是向串口发送; 对接收来说, 输出是从接收队列中取
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
/***************************** 类型定义end  **************************************************************/

#define LITTLE_ENDIAN   0
#define BIG_ENDIAN      1


#define PMS5003      0    // 不带甲醛测量
#define PMS5003S     1     // 带甲醛测量
#define HCHO_SENSOR  2
#define SENSOR_SELECT  HCHO_SENSOR


#define HCHO_EN     1
#define PM25_EN     1

/********************************* 宏 函数begin **********************************************/
#define NOP()   __NOP()
#define GLOBAL_DISABLE_IRQ()      __disable_irq()  //Global interrupt disable
#define GLOBAL_ENABLE_IRQ()       __enable_irq()   //Global interrupt enable

#define WEAK_ATTR   __attribute__((weak))

#define READ_REG_32_BIT(reg,  b)      ((uint32_t)((reg)&((uint32_t)b)))      //b必须为整数, reg为32 bit 寄存器
#define CLEAR_REG_32_BIT(reg, b)      ((reg)&=(uint32_t)(~((uint32_t)b)))   //b必须为整数, reg为32 bit 寄存器
#define SET_REG_32_BIT(reg,   b)      ((reg)|=(uint32_t)(b))                  //b必须为整数, reg为32 bit 寄存器

#define READ_REG_8_BIT(reg,b)     ((uint8_t)((reg)&((uint8_t)b)))           //b必须为8bit整数, reg为寄存器
#define CLEAR_REG_8_BIT(reg,b)    ((reg)&=(uint8_t)(~((uint8_t)b)))        //b必须为8bit整数, reg为寄存器
#define SET_REG_8_BIT(reg,b)      ((reg)|=(uint8_t)(b))                      //b必须为8bit整数, reg为寄存器

// FLASH 存储对齐后的长度: 按4字节对齐
#define WORD_ALIGNED_LEN(len)   ( (sizeof(len) + sizeof(int) - 1) &  ~(sizeof(int) - 1) ) 

/********************************* 宏 函数end  **********************************************/
#endif  // __GLOBAL_DEF_H__


