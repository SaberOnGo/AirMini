
#include "RTCDrv.h"







#if RTC_DEBUG_EN
#define RTC_DEBUG(fmt, ...)  printf(fmt, ##__VA_ARGS__) 
#else
#define RTC_DEBUG(...)
#endif


T_Calendar_Obj calendar;

#ifndef USE_STD_LIB
#define  RCC_APB1PeriphClockCmd_Enable_PWR_BKP() \
	SET_REG_32_BIT(RCC->APB1ENR, RCC_APB1Periph_PWR | RCC_APB1Periph_BKP)
	
#define RTCDRV_EnterConfigMode()     (RTC->CRL |= RTC_CRL_CNF)
#define RTCDRV_ExitConfigMode()      (RTC->CRL &= (uint16_t)~((uint16_t)RTC_CRL_CNF))
#define SCB_AIRCR_VECTKEY_MASK       ((uint32_t)0x05FA0000)

/* Alias word address of DBP bit */
#define RTCDRV_PWR_OFFSET               (PWR_BASE - PERIPH_BASE)

#define RTCDRV_CR_OFFSET                (RTCDRV_PWR_OFFSET + 0x00)
#define RTCDRV_DBP_BitNumber            0x08
#define RTCDRV_CR_DBP_BB                (PERIPH_BB_BASE + (RTCDRV_CR_OFFSET * 32) + (RTCDRV_DBP_BitNumber * 4))

#define RTCDRV_RCC_OFFSET                (RCC_BASE - PERIPH_BASE)

/* Alias word address of RTCEN bit */
#define RTCDRV_BDCR_OFFSET               (RTCDRV_RCC_OFFSET + 0x20)
#define RTCDRV_RTCEN_BitNumber           0x0F
#define RTCDRV_BDCR_RTCEN_BB             (PERIPH_BB_BASE + (RTCDRV_BDCR_OFFSET * 32) + (RTCDRV_RTCEN_BitNumber * 4))

/* Alias word address of BDRST bit */
#define RTCDRV_BDRST_BitNumber           0x10
#define RTCDRV_BDCR_BDRST_BB             (PERIPH_BB_BASE + (RTCDRV_BDCR_OFFSET * 32) + (RTCDRV_BDRST_BitNumber * 4))

#define RTCDRV_BDCR_ADDRESS              (PERIPH_BASE + RTCDRV_BDCR_OFFSET)



#define RTCDRV_BKP_WriteBackupRegister(BKP_DR, Data)  (*(__IO uint32_t *) ((uint32_t)BKP_BASE + BKP_DR) = Data)
#define RTCDRV_BKP_ReadBackupRegister(BKP_DR)        (*(__IO uint16_t *) (((uint32_t)BKP_BASE) + (BKP_DR)))

#define RTCDRV_WAIT_TIMEOUT          ((uint32_t) 0x00020000)


static uint8_t RTCDRV_WaitForLastTask(void)
{
  uint32_t count = 0;
  
  /* Loop until RTOFF flag is set */
  while ( (! (RTC->CRL & RTC_CRL_RTOFF)) && (count++ < RTCDRV_WAIT_TIMEOUT))
  {
  }
  return ( (count >= RTCDRV_WAIT_TIMEOUT) ? 1 : 0);
}

static uint8_t RTCDRV_WaitForSynchro(void)
{
  uint32_t count = 0;
  
  /* Clear RSF flag */
  RTC->CRL &= (uint16_t)~RTC_CRL_RSF;
  /* Loop until RSF flag is set */
  while ((! (RTC->CRL & RTC_CRL_RSF)) && (count++ < RTCDRV_WAIT_TIMEOUT) )
  {
  }
  return ( (count >= RTCDRV_WAIT_TIMEOUT) ? 1 : 0);
}

#endif

#ifdef USE_STD_LIB
static void RTC_NVIC_Config(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Configure one bit for preemption priority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
  
  /* Enable the RTC Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}
#else
#define RTC_NVIC_Config()  STM32_NVICInit(RTC_IRQn, 3, 6, 0)   	// ��3�����ȼ�, 3λ��ռ���ȼ�, 1λ��Ӧ���ȼ�
#endif

/**************************
����: RTC ��ʼ������
����: None
����ֵ: 1: ��ʼ��ʧ��; 0: �ɹ�
***************************/
#include "delay.h"



static uint8_t RTC_Configuration(void)
{
  uint16_t count = 0;
  uint8_t ret = 0;
  
#ifdef USE_STD_LIB
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);  /* Enable PWR and BKP clocks */
  PWR_BackupAccessCmd(ENABLE);  /* Allow access to BKP Domain */
  BKP_DeInit();                   /* Reset Backup Domain */
  RCC_LSEConfig(RCC_LSE_ON);    /* Enable LSE */
#else
  RCC_APB1PeriphClockCmd_Enable_PWR_BKP();                /* Enable PWR and BKP clocks */
  *(__IO uint32_t *) RTCDRV_CR_DBP_BB = (uint32_t)ENABLE;  /* Allow access to BKP Domain */

  /* Reset Backup Domain */
  *(__IO uint32_t *) RTCDRV_BDCR_BDRST_BB = (uint32_t)ENABLE;
  *(__IO uint32_t *) RTCDRV_BDCR_BDRST_BB = (uint32_t)DISABLE;
  
   /* Enable LSE */
  *(__IO uint8_t *) RTCDRV_BDCR_ADDRESS = RCC_LSE_OFF;
  *(__IO uint8_t *) RTCDRV_BDCR_ADDRESS = RCC_LSE_OFF;
  *(__IO uint8_t *) RTCDRV_BDCR_ADDRESS = RCC_LSE_ON;
#endif
  
  /* Wait till LSE is ready */
#ifdef USE_STD_LIB
  while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET && count < 350)
#else
  while((! READ_REG_32_BIT(RCC->BDCR, RCC_BDCR_LSERDY)) && count < 350)
#endif
  {
     count++;
	 delay_ms(10);
  }
  if(count >= 350)
  {
     RTC_DEBUG("RTC LSE Init failed, %s, %d\r\n", __FILE__, __LINE__);
     return 1;
  }
  
  
#ifdef USE_STD_LIB
   RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);                       /* Select LSE as RTC Clock Source */
   RCC_RTCCLKCmd(ENABLE);                                          /*  Enable RTC Clock */
  
   /* Wait for RTC registers synchronization */
   ret = RTC_WaitForSynchro();
   if(ret)return 1;
  
   /* Wait until last write operation on RTC registers has finished */
   ret = RTC_WaitForLastTask();
   if(ret)return 1;
 
   RTC_ITConfig(RTC_IT_SEC, ENABLE);  /* Enable the RTC Second */

    /* Wait until last write operation on RTC registers has finished */
   ret = RTC_WaitForLastTask();
   if(ret)return 1;

   /* Set RTC prescaler: set RTC period to 1sec */
    RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */
   
    /* Wait until last write operation on RTC registers has finished */
    ret = RTC_WaitForLastTask();
    if(ret)return 1;
#else
   SET_REG_32_BIT(RCC->BDCR, RCC_RTCCLKSource_LSE);             /* Select LSE as RTC Clock Source */
   *(__IO uint32_t *) RTCDRV_BDCR_RTCEN_BB = (uint32_t)ENABLE;   /*  Enable RTC Clock */

   /* Wait for RTC registers synchronization */
   ret = RTCDRV_WaitForSynchro();
   if(ret)return 1;
  
   /* Wait until last write operation on RTC registers has finished */
   ret = RTCDRV_WaitForLastTask();
   if(ret)return 1;

   RTC->CRH |= RTC_IT_SEC;   /* Enable the RTC Second */

   /* Wait until last write operation on RTC registers has finished */
   ret = RTCDRV_WaitForLastTask();
   if(ret)return 1;

   /* Set RTC prescaler: set RTC period to 1sec */
   /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */
   RTCDRV_EnterConfigMode();
   RTC->PRLH = (32767 & ((uint32_t)0x000F0000)) >> 16;
   RTC->PRLL = (32767 & ((uint32_t)0x0000FFFF));
   RTCDRV_ExitConfigMode();

   /* Wait until last write operation on RTC registers has finished */
   ret = RTCDRV_WaitForLastTask();
   if(ret)return 1;
#endif

   return 0;  
}


void RTCDrv_Init(void)
{
    uint8_t ret;

#ifdef USE_STD_LIB
	RTC_NVIC_Config();
	if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
    {
       RTC_DEBUG("\r\n\n RTC not yet configured....");

       /* RTC Configuration */
       RTC_Configuration();

       RTC_DEBUG("\r\n RTC configured....");

       /* Adjust time by values entered by the user on the hyperterminal */
       RTCDrv_SetTime(2017, 11, 14, 12, 0, 30);

       BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
   }
   else
   {
       /* Check if the Power On Reset flag is set */
       if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET)
       {
           RTC_DEBUG("\r\n\n Power On Reset occurred....");
       }
       /* Check if the Pin Reset flag is set */
       else if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET)
       {
           RTC_DEBUG("\r\n\n External Reset occurred....");
       }
       RTC_DEBUG("\r\n No need to configure RTC....");
       /* Wait for RTC registers synchronization */
       ret = RTC_WaitForSynchro();
       if(ret){ RTC_DEBUG("WaitForSync Failed: %s, %d\r\n", __FILE__, __LINE__); }
	   
       /* Enable the RTC Second */
       RTC_ITConfig(RTC_IT_SEC, ENABLE);
	   
       /* Wait until last write operation on RTC registers has finished */
       ret = RTC_WaitForLastTask();
	   if(ret){ RTC_DEBUG("WaitForLastTask Failed: %s, %d\r\n", __FILE__, __LINE__); }
    }
#ifdef RTCClockOutput_Enable
  /* Enable PWR and BKP clocks */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
  
  /* Allow access to BKP Domain */
  PWR_BackupAccessCmd(ENABLE);

  /* Disable the Tamper Pin */
  BKP_TamperPinCmd(DISABLE); /* To output RTCCLK/64 on Tamper pin, the tamper
                                 functionality must be disabled */

  /* Enable RTC Clock Output on Tamper Pin */
  BKP_RTCOutputConfig(BKP_RTCOutputSource_CalibClock);
#endif

  /* Clear reset flags */
  RCC_ClearFlag();

#else  // #ifdef USE_STD_LIB
    RTC_NVIC_Config();

	RCC_APB1PeriphClockCmd_Enable_PWR_BKP();				/* Enable PWR and BKP clocks */
	*(__IO uint32_t *) RTCDRV_CR_DBP_BB = (uint32_t)ENABLE;	/* Allow access to BKP Domain */

	if (RTCDRV_BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
    {
       RTC_DEBUG("\r\n\n RTC not yet configured....");

       /* RTC Configuration */
       RTC_Configuration();

       RTC_DEBUG("\r\n RTC configured....");

       /* Adjust time by values entered by the user on the hyperterminal */
       RTCDrv_SetTime(2017, 10, 18, 10, 54, 25);

       RTCDRV_BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
   }
   else
   {
       /* Check if the Power On Reset flag is set */
       if(READ_REG_32_BIT(RCC->CSR, RCC_CSR_PORRSTF))
       {
           RTC_DEBUG("\r\n\n Power On Reset occurred....");
       }
       else if(READ_REG_32_BIT(RCC->CSR, RCC_CSR_PINRSTF))  /* Check if the Pin Reset flag is set */
       {
           RTC_DEBUG("\r\n\n External Reset occurred....");
       }
       RTC_DEBUG("\r\n No need to configure RTC....");

	   RCC->CSR |= RCC_CSR_RMVF;
       ret = RTCDRV_WaitForSynchro();   /* Wait for RTC registers synchronization */
       if(ret){ RTC_DEBUG("WaitForSync Failed: %s, %d\r\n", __FILE__, __LINE__); }
	   
       /* Enable the RTC Second */
	   RTC->CRH |= RTC_IT_SEC;
	   
       ret = RTCDRV_WaitForLastTask();  /* Wait until last write operation on RTC registers has finished */
	   if(ret){ RTC_DEBUG("WaitForLastTask Failed: %s, %d\r\n", __FILE__, __LINE__); }
   }
   /* Clear reset flags */
   RCC->CSR |= RCC_CSR_RMVF;

#endif
}		 		


 			
// ���������: �ܱ�4�����Ҳ��ܱ�100 ����, �����ܱ�400 ����
#define  IS_LEAP_YEAR(year)  (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0))


//�·����ݱ�											 
//uint8_t const table_week[12] = { 0,3,3,6,1,4,6,2,5,0,3,5 }; //���������ݱ�	  

//ƽ����·����ڱ�
const uint8_t mon_table[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

/************************
����: �����ĳ���1��1��0ʱ0��0�뿪ʼ�����ڵ�����
����: uint16_t fromYear: ָ����ĳ�꿪ʼ
             T_RTC_TIME * cal: ��ǰ��ʱ��
����ֵ: ��ָ�����1��1��0ʱ0��0�뿪ʼ�����ڵ�����
*************************/
static uint32_t RTCDrv_CalendarToSec(uint16_t fromYear, T_Calendar_Obj * cal)
{
      uint16_t t;
      uint32_t seccount = 0;
      uint8_t month_idx;
      
    #if RTC_DEBUG_EN
	if(fromYear < 1970 || fromYear > 2099 || NULL == cal)
		{ RTC_DEBUG("param err: %s, %d\r\n", __FILE__, __LINE__);  return 1; }
	#endif
	
	for(t = fromYear; t < cal->year; t++)	//��������ݵ��������
	{
		if(IS_LEAP_YEAR(t))seccount += 31622400;  //�����������
		else seccount += 31536000;			       //ƽ���������
	}

	month_idx = cal->month - 1;
	for(t = 0; t < month_idx; t++)	   //��ǰ���·ݵ����������
	{
		seccount += (uint32_t)mon_table[t] * 86400;  //�·����������, 86400 Ϊһ�������
		if(IS_LEAP_YEAR(cal->year) && t == 1)seccount += 86400;  //����2�·�����һ���������	   
	}
	seccount += (uint32_t)(cal->day - 1) * 86400;  //��ǰ�����ڵ���������� 
	seccount += (uint32_t)cal->hour * 3600;        //Сʱ������
    seccount += (uint32_t)cal->min * 60;	          //����������
	seccount += cal->sec;                          //�������Ӽ���ȥ

	return seccount;
}

//������������ڼ�
//��������:���빫�����ڵõ�����(ֻ����1901-2099��)
//������������������� 
//����ֵ�����ں�: 0 - 6, ��ʾ ������ - ������		
#if 0
uint8_t RTCDrv_GetWeek(uint16_t year, uint8_t month, uint8_t day)
{	
	uint16_t temp;
	uint8_t yearH, yearL;

    #if RTC_DEBUG_EN
	if(year < 1901 || year > 2099)
		{ RTC_DEBUG("param err: %s, %d\r\n", __FILE__, __LINE__);  return 0; }
	#endif
	
	yearH = year / 100;	
	yearL = year % 100; 
	
	// ���Ϊ21����,�������100  
	if (yearH > 19) yearL+= 100;
	
	temp = yearL + yearL / 4;  // ����������ֻ��1900��֮���  
	temp %= 7; 
	temp = temp + day + table_week[month-1];  // ���������
	if (yearL % 4 == 0 && month < 3)temp--;
	return ( temp % 7 );
}	
#endif
	
/************************
����: �����ָ�����1��1��0ʱ0��0�뿪ʼ�����ڵ�����ת��Ϊ��ǰʱ��
�������:  uint16_t fromYear: ָ����ĳ�꿪ʼ
                         uint32_t sec: ��Ҫת������ֵ
                         uint8_t   one_more_day:  �����Ƿ�ֹһ��, �����ֹһ�������¼���������
�������:  T_RTC_TIME * cal: ��ǰ��ʱ��
����ֵ:       1: ʧ��; 0: �ɹ�
*************************/
static void RTCDrv_SecToCalendar(uint16_t fromYear, uint32_t sec, T_Calendar_Obj * cal, uint8_t one_more_day)
{
	uint32_t temp = 0;
	uint16_t y    = 0;	  

    #if RTC_DEBUG_EN
    if(fromYear < 1970 || fromYear > 2099 || NULL == cal)
		{ RTC_DEBUG("param err: %s, %d\r\n", __FILE__, __LINE__);  return; }
	#endif
	
 	temp = sec / 86400;   //�õ�����(��������Ӧ��)
	if(one_more_day)
	{	  
		y       = fromYear;	      // ��1970�꿪ʼ
		while(temp >= 365)  // ���� 1 ��
		{				 
			if(IS_LEAP_YEAR(y))  //������
			{
				if(temp >= 366)temp -= 366;  //���������
				else
				{  
				   y++; break;
				}  
			}
			else temp-= 365; // ƽ�� 
			y++;  
		}   
		cal->year = y;       // �õ����
		y = 0;
		while(temp >= 28)  // ������ֹһ���µ�����
		{
			if(IS_LEAP_YEAR(cal->year) && y == 1)  //�Ƿ�Ϊ�����2�·�
			{
				if(temp >= 29)temp -= 29;  //��ȥ2�·ݵ�����
				else break; 
			}
			else // Ϊ���굫����2�·�, ������ƽ����·�
			{
				if(temp >= mon_table[y])temp -= mon_table[y];  
				else break;
			}
			y++;  
		}
		cal->month = y    + 1;	// �õ��·�
		cal->day   = temp + 1;  	// �õ����� 
	}
	temp       = sec % 86400;     //�õ�������   	   
	cal->hour = temp / 3600;            //Сʱ
	cal->min  = (temp % 3600 ) / 60;   //����	
	cal->sec  = (temp % 3600 ) % 60;   //����
	//cal->week = RTCDrv_GetWeek(cal->year, cal->month, cal->day);  //��ȡ����   
}
	
//����ʱ��
//�������ʱ��ת��Ϊ����
//��1970��1��1��Ϊ��׼
//1970~2099��Ϊ�Ϸ����
//����ֵ:0,�ɹ�;����:�������.
uint8_t RTCDrv_SetTime(uint16_t syear,uint8_t smon, uint8_t sday, uint8_t hour, uint8_t min, uint8_t sec)
{
	uint32_t seccount;
	T_Calendar_Obj cal;

	cal.year = syear; cal.month = smon; cal.day = sday; 
	cal.hour = hour;  cal.min   = min;   cal.sec = sec;
	seccount = RTCDrv_CalendarToSec(1970, &cal);

#ifdef USE_STD_LIB
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);	//ʹ��PWR��BKP����ʱ��  
	PWR_BackupAccessCmd(ENABLE);	  // ʹ��RTC�ͺ󱸼Ĵ������� 
	RTC_SetCounter(seccount);	  // ����RTC��������ֵ

	if(RTC_WaitForLastTask())   // �ȴ����һ�ζ�RTC�Ĵ�����д�������  	
		{ RTC_DEBUG("RTC_SET() failed: %s, %d\r\n", __FILE__, __LINE__); return 1; }   
#else
    RCC_APB1PeriphClockCmd_Enable_PWR_BKP();
	*(__IO uint32_t *) RTCDRV_CR_DBP_BB = (uint32_t)ENABLE;

	RTCDRV_EnterConfigMode();
    RTC->CNTH = seccount >> 16;                         /* Set RTC COUNTER MSB word */
    RTC->CNTL = (seccount & ((uint32_t)0x0000FFFF));  /* Set RTC COUNTER LSB word */
    RTCDRV_ExitConfigMode();

	if(RTCDRV_WaitForLastTask())   // �ȴ����һ�ζ�RTC�Ĵ�����д�������  	
		{ RTC_DEBUG("RTC_SET() failed: %s, %d\r\n", __FILE__, __LINE__); return 1; }   
#endif
	
	return 0;	    
}

//��ʼ������		  
//��1970��1��1��Ϊ��׼
//1970~2099��Ϊ�Ϸ����
//syear,smon,sday,hour,min,sec�����ӵ�������ʱ����   
//����ֵ:0,�ɹ�;����:�������.
uint8_t RTCDrv_SetAlarm(uint16_t syear, uint8_t smon, uint8_t sday, uint8_t hour, uint8_t min, uint8_t sec)
{
	uint32_t seccount;
	T_Calendar_Obj cal;

    //���������Ǳ����!
	cal.year = syear; cal.month = smon; cal.day = sday; 
	cal.hour = hour;  cal.min   = min;   cal.sec = sec;
	seccount = RTCDrv_CalendarToSec(1970, &cal);
	
	//����ʱ��
#ifdef USE_STD_LIB
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);	//ʹ��PWR��BKP����ʱ��   
	PWR_BackupAccessCmd(ENABLE);	//ʹ�ܺ󱸼Ĵ�������  
	
	RTC_SetAlarm(seccount);

	if(RTC_WaitForLastTask())   // �ȴ����һ�ζ�RTC�Ĵ�����д�������  	
		{ RTC_DEBUG("RTC_SET() failed: %s, %d\r\n", __FILE__, __LINE__); return 1; }  	
#else
	RCC_APB1PeriphClockCmd_Enable_PWR_BKP();
    *(__IO uint32_t *) RTCDRV_CR_DBP_BB = (uint32_t)ENABLE;
	
	RTCDRV_EnterConfigMode();
    RTC->ALRH = seccount >> 16;                          /* Set the ALARM MSB word */
    RTC->ALRL = (seccount & ((uint32_t)0x0000FFFF));   /* Set the ALARM LSB word */
    RTCDRV_ExitConfigMode();

	if(RTCDRV_WaitForLastTask())   // �ȴ����һ�ζ�RTC�Ĵ�����д�������  	
		{ RTC_DEBUG("RTC_SET() failed: %s, %d\r\n", __FILE__, __LINE__); return 1; }  	
#endif
	
	return 0;	    
}

//�õ���ǰ��ʱ��
//����ֵ:0,�ɹ�;����:�������.
uint8_t RTCDrv_GetTime(T_Calendar_Obj * cal)
{
	uint32_t sec = 0;
    static uint16_t day_count = 0;  // ��¼��һ�α��������
	uint16_t cur_day_count = 0;
    uint8_t  one_more_day = E_FALSE;

    sec = RTC_GetCounter();	 
	cur_day_count = sec / 86400;
    if(day_count != cur_day_count)
    {
        day_count    = cur_day_count;
		one_more_day = E_TRUE;
    }
 	RTCDrv_SecToCalendar(1970, sec, cal, one_more_day);
#if (! USE_TVOC_CAL)
	TIME_SetSensorExisted(E_TRUE);
#endif
	return 0;
}	 	  

//RTCʱ���ж�
//ÿ�봥��һ��  
void RTC_IRQHandler(void)
{		 
    #if 0
	if (RTC_GetITStatus(RTC_IT_SEC))  //�����ж�
	#else
    if( (RTC->CRH & RTC_IT_SEC) && (RTC->CRL & RTC_IT_SEC)) //�����ж�
	#endif
	{							
		RTCDrv_GetTime(&calendar);   // ����ʱ��
		RTC_DEBUG("RTC Time:%04d-%02d-%02d %02d:%02d:%02d, week = %d\n", calendar.year, calendar.month, calendar.day, 
  	                calendar.hour, calendar.min, calendar.sec, calendar.week);
 	}
	
	#if 0
	if(RTC_GetITStatus(RTC_IT_ALR))   //�����ж�
	#else
    if((RTC->CRH & RTC_IT_ALR) && (RTC->CRL & RTC_IT_ALR)) //�����ж�
	#endif
	{ 	
		RTC->CRL &= (uint16_t)~RTC_IT_ALR;  //�������ж�	

	    RTCDrv_GetTime(&calendar);   // ����ʱ��  

		//�������ʱ��	
  	    RTC_DEBUG("Alarm Time:%04d-%02d-%02d %02d:%02d:%02d, week = %d\n", calendar.year, calendar.month, calendar.day, 
  	                calendar.hour, calendar.min, calendar.sec, calendar.week);
  	} 				  								 
	RTC->CRL &= (uint16_t)~ (RTC_IT_ALR | RTC_IT_SEC);  // ���жϱ�־λ
	
	//RTC_WaitForLastTask();	  	    						 	   	 
}

