

#include "os_global.h"
#include <string.h>
#include "TimerManager.h"




/*****************************************************************************
 * 函 数 名  : OS_SetTimeout
 * 负 责 人  : pi
 * 创建日期  : 2016年8月18日
 * 函数功能  : 设置超时值
 * 输入参数  : uint32_t timeout  超时tick数, unit: 10ms
 * 输出参数  : 无
 * 返 回 值  :    超时值
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
uint32_t OS_SetTimeout(uint32_t tick)
{
   return OS_GetSysTick() + tick;
}

/*****************************************************************************
 * 函 数 名  : OS_IsTimeout
 * 负 责 人  : pi
 * 创建日期  : 2016年8月18日
 * 函数功能  : 判断是否超时
 * 输入参数  : uint32 timetick  超时值
 * 输出参数  : 无
 * 返 回 值  : 
 * 调用关系  :  超时: E_TRUE, 否: E_FALSE
 * 其    它  : 

*****************************************************************************/
E_BOOL OS_IsTimeout(uint32_t timetick)
{
   return ((OS_GetSysTick() >= timetick)? E_TRUE : E_FALSE);
}

E_BOOL gCpuIsBigEndian;  // 是否大端
// 判断CPU是否大端
// 返回值: E_TRUE: 大端; E_FALSE: 小端
E_BOOL FLASH_SAVE Sys_IsBigEndian(void)
{
   union
   {
      int  w16;
	  char c8;
   }uNum;

   uNum.w16 = 0x1234;
   if(0x12 == uNum.c8)  // low addr, MSB : bigEnddian
   {
      return E_TRUE;
   }
   else
   {
      return E_FALSE;
   }
}

// BSP 初始化, 硬件相关
void FLASH_SAVE BSP_Init(void)
{
   gCpuIsBigEndian = Sys_IsBigEndian();
   os_printf("Sys is %s\n", gCpuIsBigEndian ? "BigEndian" : "LittleEndian");
}

/*****************************************************************************
 * 函 数 名  : Util_NumToString
 * 负 责 人  : pi
 * 创建日期  : 2016年8月18日
 * 函数功能  : 将数字转换为字符串
 * 输入参数  : uint32_t num          数字                       
                             uint8_t placeholder_size  占位符长度
 * 输出参数  : uint8_t * pOutString  输出字符串
 * 返 回 值  : uint8_t: 生成的字符个数, 转换失败返回 0
 * 调用关系  : 
 * 其    它  : 
 uint8_t placeholder_size: 占位符个数, 有6个占位，则只能保存6个字符

*****************************************************************************/
uint8_t Util_NumToString(uint32_t val, uint8_t * out_buf, uint8_t placeholder_size)
{
   uint8_t num_string[12];    
   uint32_t div = 1000000000L;
   uint8_t i, valid_size;  // valid_size 为有效显示的数字个数, 如 00123, 则valid_size = 3
   uint8_t space_size;  // 空白字符个数
   
   if(NULL == out_buf)
   {
      return 0;
   }
   memset(num_string, 0, sizeof(num_string));
   for(i = 0; i < 10; i++)   // 32位最多有10个数字
   {
      num_string[i]  = val / div + 0x30;   // 取整
      val  %= div;         // 求余,取剩下的整数部分
      div /= 10;
   }
   
    // 跳过前面的连续 0
    for(i = 0; i < 10; i++)
    {
       if(num_string[i] != 0x30)break;  
    }
	valid_size = 10 - i;
	if(valid_size == 0){ i = 9, valid_size = 1; }

    /* 占位符位数比有效数字的个数多, 比如占位符个数为6, 数字为00123, 
        则有效数字的位数为3
       */
	if(placeholder_size >= valid_size)  
	{
	   space_size = placeholder_size - valid_size;  // 空白字符个数
	   
	   os_memset(out_buf, '0', space_size);  // 不够补0
	   os_memcpy(&out_buf[space_size], &num_string[i], valid_size);
	}
	else  // 占位符不够, 只显示小值部分
	{
	   i += valid_size - placeholder_size; // 前面部分不显示, 如 123456, 占位符为4, 则显示 3456
	   os_memcpy(out_buf, &num_string[i], placeholder_size);
	}
	
    return  placeholder_size;
}

// 跳转到 bootloader
#include "misc.h"
void JumpToBootloader(void)
{
    RCC_DeInit();
    GLOBAL_DISABLE_IRQ();
	NVIC_SystemReset();
	
	#if 0
	/* Set the Vector Table base location at 0x08000000 */
    //NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
    if (((*(__IO uint32_t*)BOOTLOADER_ADDRESS) & 0x2FFE0000 ) == 0x20000000)
    { 
      /* Jump to user application */
      JumpAddress = *(__IO uint32_t*) (BOOTLOADER_ADDRESS + 4);
      Jump_To_Bootloader = (pFunction) JumpAddress;
	  __set_PSP(*(__IO uint32_t*) BOOTLOADER_ADDRESS);
	  __set_CONTROL(0);  // 进入用户级线程模式 进入软中断后才可以回到特权级线程模式
      /* Initialize user application's Stack Pointer */
      __set_MSP(*(__IO uint32_t*) BOOTLOADER_ADDRESS);
      Jump_To_Bootloader();
    }
	#endif
}



uint32_t Sys_GenSum32(void * data, uint32_t length)
{
   register uint32_t i;
   uint32_t sum = 0;
   uint8_t * p = (uint8_t *)data;
   
   for(i = 0; i < length; i++)
   {
      sum += p[i];
   }
   return sum;
}

