
#include "DHT11.h"
#include "delay.h"
#include "SDRR.h"
#include "os_timer.h"
#include "os_global.h"
#include "GlobalDef.h"

// 设置数据口为输入或输出
static void DHT_SetIODir(E_IO_DIR dir)
{
    GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = DHT_IO_Pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	if(dir == INPUT)GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    else GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

	#ifdef USE_STD_LIB
	GPIO_Init(DHT_IO_PORT, &GPIO_InitStructure);
	#else
    STM32_GPIOInit(DHT_IO_PORT, &GPIO_InitStructure);
	#endif
}

static os_timer_t tTimerDHT;  
static os_timer_t tTimerGetTempHumi;  // 定时获取一次温湿度的定时器
	
static uint8_t timing_count = 0;  // 定时计数
static uint8_t dht_sta = DHT_NOT_INIT; 
static E_BOOL dht_is_busy = E_FALSE;  // E_FALSE: 空闲; E_TRUE: 忙
T_DHT tDHT;

static uint8_t DHT_RxByte(uint8_t * out)//接受一个字节
{     
    uint8_t i;    
	uint8_t data = 0;  
	uint16_t timeout;
	uint16_t count = 0;  
	
    for (i = 0; i < 8; i++)    
	{   	 
        timeout = 0xFFFF;
		while((! DHT_IO_READ()) && timeout--) //等待50us低电平开始发送信号
		{ 
           NOP();
		}
        if(! timeout){os_printf("line = %d, PD err\n", __LINE__);  return DHT_PD_TIMEOUT;  }
		
		timeout = 1000;
		count   = 0;
		while( DHT_IO_READ() && timeout--)   // 根据高电平时间判断是 bit 1 or bit0
		{
		   delay_us(10);
		   count++;   //计算高电平时间
		}
		if(! timeout){os_printf("line = %d, PU err\n", __LINE__);  return DHT_PU_TIMEOUT;  }
		data <<= 1;	//  接收数据高位在前   
		if(4 <= count && count < 9)  // 70 us 为 bit 1, 26 - 28 us 为 bit0
		{
		   data |= 1;
		}
		else if(count >= 8)
		{
		   os_printf("dht bit err count = %d\n", count);
		   return DHT_BIT_ERR;
		}
	}
	*out = data;
	return DHT_OK;
}   

#define DHT_LEAVE(sta)  { dht_sta = sta; dht_is_busy = E_FALSE; return;}

static uint32_t dht_time_count = 0;

extern uint8_t is_debug_on; // 调试打印开关

// 回调函数, 用于 DHT11 传感器的ms级延时的回调
static void TimerDHT_CallBack(void * arg)
{
   if(timing_count == 0)
   {
      timing_count++;
      DHT_IO_L();
	  os_timer_arm(&tTimerDHT, 2, 0);  // DH11起始信号需要至少18ms   
   }
   else if(timing_count == 1)
   {
      uint32_t timeout = 1000;  // 1 ms 超时计数
	  
      timing_count++;
	  DHT_IO_H();
	  delay_us(39);  // 主机 拉高 20-40us, 等待 DHT 响应
      DHT_SetIODir(INPUT);

	  timeout = 0xFFFF;
      while( ( DHT_IO_READ()) && timeout--);  
	  if(! timeout){ DHT_LEAVE(DHT_PU_TIMEOUT); }
      if(! DHT_IO_READ())  // 如果 DHT 拉低了, 则等待它拉高
      {
         timeout = 0xFFFF;
         while( (! DHT_IO_READ()) && timeout--)  // DHT 拉低80 us 左右
         {
            NOP();
         }
		 if(! timeout){ DHT_LEAVE(DHT_PD_TIMEOUT); }
		 dht_sta = DHT_INIT_OK;
		 do
		 {
		    uint8_t Temp_H, Temp_L, Humi_H, Humi_L, newSum = 0, Sum = 0;
            uint8_t res = 0;
			
			DHT_IO_H();
			timeout = 0xFFFF;
			while((DHT_IO_READ()) && timeout--)  //等待DHT80us后拉低总线
			{
			   NOP();
			}
			if(! timeout){ DHT_LEAVE(DHT_PU_TIMEOUT);}
			res = DHT_RxByte(&Humi_H);
			if(res){ INSERT_ERROR_INFO(0); DHT_LEAVE(DHT_ERR); }
			res = DHT_RxByte(&Humi_L);
			if(res){ INSERT_ERROR_INFO(0); DHT_LEAVE(DHT_ERR); }
			res = DHT_RxByte(&Temp_H);
			if(res){ INSERT_ERROR_INFO(0); DHT_LEAVE(DHT_ERR); }
			res = DHT_RxByte(&Temp_L);
			if(res){ INSERT_ERROR_INFO(0); DHT_LEAVE(DHT_ERR); }
			res = DHT_RxByte(&Sum);
			if(res){ INSERT_ERROR_INFO(0); DHT_LEAVE(DHT_ERR); }
			newSum = Humi_H + Humi_L + Temp_H + Temp_L;
			if(newSum != Sum)
			{
			   os_printf("---dht ck err----: new = 0x%x, sum = 0x%x, H_H = %d\tH_L = %d\tT_H = %d\tT_L = %d\n", 
			   	           newSum, Sum, Humi_H, Humi_L, Temp_H, Temp_L);
			   DHT_LEAVE(DHT_CHECK_ERR);
			}
			// 数据正确
			if(OS_IsTimeout(dht_time_count))
			{
			    dht_time_count = OS_SetTimeout(SEC(10));
				if(is_debug_on)
				{	
				    os_printf("tick = %ld, dht ok: H_H = %d, H_L = %d, T_H = %d, T_L = %d\n", 
			            	    OS_GetSysTick(), Humi_H, Humi_L, Temp_H, Temp_L);
				}
		 	}
			// 保存数据
			tDHT.humi_H = Humi_H;
			tDHT.humi_L = Humi_L;
			tDHT.temp_H = Temp_H;
			tDHT.temp_L = Temp_L;
			tDHT.sum    = Sum;
			DHT_LEAVE(DHT_OK);
		 }while(0);
      }
	  else
	  {
		 DHT_LEAVE(DHT_NO_RESP);
	  }
   }
}

static uint32_t save_tick = 0;  // 保存时间
static void TimerGetTempHumi_CallBack(void * arg)
{
	if(dht_sta == DHT_OK)  // 获取温湿度成功
    {
       uint16_t val;

	   if(OS_IsTimeout(save_tick))
	   {
	      save_tick = OS_SetTimeout(SEC(2 * 1));
		  
		  // 保存温湿度
          val = tDHT.temp_H * 10;
          SDRR_SaveSensorPoint(SENSOR_TEMP, &val);
	      SDRR_SaveSensorPoint(SENSOR_HUMI, &tDHT.humi_H);
	   }
	   TempHumi_SetSensorExisted(E_TRUE);
    }
	DHT_Start();
	os_timer_arm(&tTimerGetTempHumi, 50, 0);  // 500 ms 定时
}

void DHT_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
  
	DHT_RCC_APBPeriphClockCmdEnable();    

    GPIO_InitStructure.GPIO_Pin   = DHT_IO_Pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;

	#ifdef USE_STD_LIB
    GPIO_Init(DHT_IO_PORT, &GPIO_InitStructure);
    #else
	STM32_GPIOInit(DHT_IO_PORT, &GPIO_InitStructure);
	#endif
	
	os_timer_setfn(&tTimerDHT, TimerDHT_CallBack, NULL);
	os_timer_setfn(&tTimerGetTempHumi, TimerGetTempHumi_CallBack, NULL);
	os_timer_arm(&tTimerGetTempHumi, 500, 0);  // 5 s 后

	DHT_Start();
}

// 启动 DHT
void DHT_Start(void)
{
   if(dht_is_busy)return;
   timing_count = 0;         // 定时计数 清 0
   dht_sta = DHT_NOT_INIT;
   DHT_SetIODir(OUTPUT);  // IO 管脚 为输出
   DHT_IO_H();
   os_timer_disarm(&tTimerDHT);
   os_timer_arm(&tTimerDHT, 1, 0);  // 10 ms
   dht_is_busy = E_TRUE;
}



