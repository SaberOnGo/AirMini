
#include "LED_Drv.h"
#include "os_timer.h"
#include "delay.h"





void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = LED_RED_GPIO_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(LED_RED_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LED_GREEN_GPIO_Pin;
    GPIO_Init(LED_GREEN_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LED_YELLOW_GPIO_Pin;
    GPIO_Init(LED_YELLOW_PORT, &GPIO_InitStructure);

	PWR_BackupAccessCmd(ENABLE);  //允许修改RTC 和后备寄存器
    RCC_LSEConfig(RCC_LSE_OFF);   //关闭外部低速外部时钟信号功能 后，PC13 PC14 PC15 才可以当普通IO用。
    BKP_TamperPinCmd(DISABLE);    //关闭入侵检测功能，也就是 PC13，也可以当普通IO 使用
    PWR_BackupAccessCmd(DISABLE); //禁止修改后备寄存器

	LED_AllClose();
}

void LED_Config(E_LED_SELECT led, E_LED_STATE state)
{
   switch(led)
   {
       case LED_RED:
	   	  LED_RED_Set(state);
		 break;
	   case LED_GREEN:
	   	  LED_GREEN_Set(state);
		 break;
	   case LED_YELLOW:
	   	  LED_YELLOW_Set(state);
		break;
   }
}

void LED_AllSet(E_LED_STATE state)
{
   LED_Config(LED_RED, state);
   LED_Config(LED_GREEN, state);
   LED_Config(LED_YELLOW, state);
}


static os_timer_t tLedTimer;
static uint8_t led_state = LED_CLOSE;

// 停止LED 指示
void LED_StopAqiIndicate(void)
{
    os_timer_disarm(&tLedTimer);
	LED_AllClose();
}

static void LedTimer_CallBack(void * arg)
{
    E_AQI_LEVEL *level = (E_AQI_LEVEL *)arg;

	if(*level == AQI_JustInHell)
	{
	   LED_AllClose();
	   if(led_state == LED_CLOSE)  // 上一次关灯
	   {
	      led_state = LED_OPEN;
		  LED_Config(LED_YELLOW, LED_OPEN);
		  LED_Config(LED_RED, LED_OPEN);  
	   }
	   else 
	   {
	      led_state = LED_CLOSE;
	   }
	   os_timer_arm(&tLedTimer, 50, 0);  // 500 ms 定时
	}
	else
	{
	   LED_AllClose();
	   if(led_state == LED_CLOSE)
	   {
	      led_state = LED_OPEN;
          LED_AllOpen();
	   }
	   else{ led_state = LED_CLOSE; }
	   os_timer_arm(&tLedTimer, 15, 0);  // 150 ms 定时
	}
}

void LED_AqiIndicate(E_AQI_LEVEL level)
{
   os_timer_disarm(&tLedTimer);
   switch(level)
   {
       case AQI_GOOD:
	   {
	   	  LED_AllClose();
		  LED_Config(LED_GREEN, LED_OPEN);
	   }break;
	   case AQI_Moderate:
	   {
	   	  LED_Config(LED_RED, LED_CLOSE);
		  LED_Config(LED_YELLOW, LED_OPEN);
		  LED_Config(LED_GREEN, LED_OPEN);
	   }break;
	   case AQI_LightUnhealthy:
	   case AQI_MidUnhealthy:
	   {
	   	  LED_AllClose();
		  LED_Config(LED_YELLOW, LED_OPEN);
	   }break;
	   case AQI_VeryUnhealthy:
	   {
	   	  LED_AllClose();
	   	  LED_Config(LED_RED, LED_OPEN);
	   }break;
	   case AQI_JustInHell:
	   {
	   	  led_state = LED_OPEN;
		  LED_AllClose();
		  LED_Config(LED_YELLOW, LED_OPEN);
		  LED_Config(LED_RED, LED_OPEN);
	   	  os_timer_setfn(&tLedTimer, LedTimer_CallBack, &level);
		  os_timer_arm(&tLedTimer, 50, 0);  // 500 ms 定时
	   }break;
	   case AQI_HeavyInHell:  
	   {
	   	  led_state = LED_OPEN;
		  LED_AllOpen();
	   	  os_timer_setfn(&tLedTimer, LedTimer_CallBack, &level);
		  os_timer_arm(&tLedTimer, 15, 0);  // 150 ms 定时
	   }break;
   }
}

static os_timer_t tLedFlashTimer;  // LED 闪烁定时器
static uint16_t flashTimes = 0;    
static uint8_t  flashFlag = 0;
static void LedFlashTimer_CallBack(void * arg)
{
   if(! flashFlag)
   {
	  LED_AllSet(LED_OPEN);
	  if(flashTimes)
	  {
	  	 flashTimes--;
	  }
   }
   else
   {
      LED_AllSet(LED_CLOSE);
   }
   if(flashTimes)os_timer_arm(&tLedFlashTimer, 50, 0);
   else { LED_AllClose(); }
   flashFlag ^= 1;
}

// LED 闪烁
// 参数: uint16_t times: 闪烁次数
void LED_Flash(uint16_t times)
{
   if(times)
   {
      flashTimes = times;
      os_timer_setfn(&tLedFlashTimer, LedFlashTimer_CallBack, NULL);
	  os_timer_arm(&tLedFlashTimer, 50, 0);
   }  
}

void LEDTest(void)
{
   uint8_t i;

   for(i = 0; i < (uint8_t)AQI_LEVEL_END; i++)
   {
      LED_AqiIndicate((E_AQI_LEVEL)i);
	  os_printf("aqi level = %d\n", i);
	  delay_ms(5000);
   }
}
