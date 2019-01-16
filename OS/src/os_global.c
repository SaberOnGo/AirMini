

#include "os_global.h"
#include <string.h>
#include "TimerManager.h"




/*****************************************************************************
 * �� �� ��  : OS_SetTimeout
 * �� �� ��  : pi
 * ��������  : 2016��8��18��
 * ��������  : ���ó�ʱֵ
 * �������  : uint32_t timeout  ��ʱtick��, unit: 10ms
 * �������  : ��
 * �� �� ֵ  :    ��ʱֵ
 * ���ù�ϵ  : 
 * ��    ��  : 

*****************************************************************************/
uint32_t OS_SetTimeout(uint32_t tick)
{
   return OS_GetSysTick() + tick;
}

/*****************************************************************************
 * �� �� ��  : OS_IsTimeout
 * �� �� ��  : pi
 * ��������  : 2016��8��18��
 * ��������  : �ж��Ƿ�ʱ
 * �������  : uint32 timetick  ��ʱֵ
 * �������  : ��
 * �� �� ֵ  : 
 * ���ù�ϵ  :  ��ʱ: E_TRUE, ��: E_FALSE
 * ��    ��  : 

*****************************************************************************/
E_BOOL OS_IsTimeout(uint32_t timetick)
{
   return ((OS_GetSysTick() >= timetick)? E_TRUE : E_FALSE);
}

E_BOOL gCpuIsBigEndian;  // �Ƿ���
// �ж�CPU�Ƿ���
// ����ֵ: E_TRUE: ���; E_FALSE: С��
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

// BSP ��ʼ��, Ӳ�����
void FLASH_SAVE BSP_Init(void)
{
   gCpuIsBigEndian = Sys_IsBigEndian();
   os_printf("Sys is %s\n", gCpuIsBigEndian ? "BigEndian" : "LittleEndian");
}

/*****************************************************************************
 * �� �� ��  : Util_NumToString
 * �� �� ��  : pi
 * ��������  : 2016��8��18��
 * ��������  : ������ת��Ϊ�ַ���
 * �������  : uint32_t num          ����                       
                             uint8_t placeholder_size  ռλ������
 * �������  : uint8_t * pOutString  ����ַ���
 * �� �� ֵ  : uint8_t: ���ɵ��ַ�����, ת��ʧ�ܷ��� 0
 * ���ù�ϵ  : 
 * ��    ��  : 
 uint8_t placeholder_size: ռλ������, ��6��ռλ����ֻ�ܱ���6���ַ�

*****************************************************************************/
uint8_t Util_NumToString(uint32_t val, uint8_t * out_buf, uint8_t placeholder_size)
{
   uint8_t num_string[12];    
   uint32_t div = 1000000000L;
   uint8_t i, valid_size;  // valid_size Ϊ��Ч��ʾ�����ָ���, �� 00123, ��valid_size = 3
   uint8_t space_size;  // �հ��ַ�����
   
   if(NULL == out_buf)
   {
      return 0;
   }
   memset(num_string, 0, sizeof(num_string));
   for(i = 0; i < 10; i++)   // 32λ�����10������
   {
      num_string[i]  = val / div + 0x30;   // ȡ��
      val  %= div;         // ����,ȡʣ�µ���������
      div /= 10;
   }
   
    // ����ǰ������� 0
    for(i = 0; i < 10; i++)
    {
       if(num_string[i] != 0x30)break;  
    }
	valid_size = 10 - i;
	if(valid_size == 0){ i = 9, valid_size = 1; }

    /* ռλ��λ������Ч���ֵĸ�����, ����ռλ������Ϊ6, ����Ϊ00123, 
        ����Ч���ֵ�λ��Ϊ3
       */
	if(placeholder_size >= valid_size)  
	{
	   space_size = placeholder_size - valid_size;  // �հ��ַ�����
	   
	   os_memset(out_buf, '0', space_size);  // ������0
	   os_memcpy(&out_buf[space_size], &num_string[i], valid_size);
	}
	else  // ռλ������, ֻ��ʾСֵ����
	{
	   i += valid_size - placeholder_size; // ǰ�沿�ֲ���ʾ, �� 123456, ռλ��Ϊ4, ����ʾ 3456
	   os_memcpy(out_buf, &num_string[i], placeholder_size);
	}
	
    return  placeholder_size;
}

// ��ת�� bootloader
#include "misc.h"
void JumpToBootloader(void)
{
    GLOBAL_DISABLE_IRQ();
    RCC_DeInit();
    
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
	  __set_CONTROL(0);  // �����û����߳�ģʽ �������жϺ�ſ��Իص���Ȩ���߳�ģʽ
      /* Initialize user application's Stack Pointer */
      __set_MSP(*(__IO uint32_t*) BOOTLOADER_ADDRESS);
      Jump_To_Bootloader();
    }
	#endif
}


uint8_t sys_gen_sum_8(uint8_t * buf,  uint16_t len)
{
          register uint16_t i;
          uint8_t sum = 0;

          for(i = 0;  i  < len; i++)
          {
                 sum += buf[i];
          }
          return sum;
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

