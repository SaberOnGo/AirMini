
/*-----------------------------电池电量显示操作------------------------------------------*/

#include "BatteryLevel.h"
#include "os_timer.h"
#include "os_global.h"
#include "ADC_Drv.h"
#include "board_version.h"
#include "PowerCtrl.h"


#if BAT_DEBUG_EN
#define BAT_DEBUG(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#else
#define BAT_DEBUG(...)
#endif


// 电池电量对应的电量百分比
typedef struct
{
    uint8_t  percent;  // 剩余电量百分比, 如: 15 表示: 15%
	uint16_t volt;    // 电压: 如 3345 表示: 3.345V, 即单位: mV
}T_BAT_LEVEL_MAP;

// 
static const T_BAT_LEVEL_MAP BatLevMap[] = 
{
     {0,  3300},   // 0%, 3.300 V: 表示: <= 3.300 V时, 电池电量为 0%
     {10, 3450},   // 10 %, 3.45 V,
     {25, 3500},   // 25 %, 3.50 V
     {30, 3550},   // 30 %, 3.55 V
     {40, 3600},   // 40 %, 3.60 V
     {50, 3650},   // 50 %, 3.65 V
     {60, 3700},   // 60 %, 3.70 V
     {80, 3850},   // 80 %, 3.85 V
     {90, 4050},   // 90 %, 4.05 V
     {100, 4200}, // 100 %, 4.20 V
};

#define BAT_LEVELS   (sizeof(BatLevMap) / sizeof(BatLevMap[0]))

uint8_t bat_lev_percent = 0;  // 电池电量
E_BOOL is_5v_power_close = E_FALSE;




void BatLev_ClosePower(uint8_t pwr_close)
{
    BAT_DEBUG("close power\n");
	
	
    
	LCD_Ctrl_Set(SW_CLOSE);
	XR1151_EN_Close(); 
	PowerCtrl_LowerPower();
	
	//PWR_SW_Close();
}

void BatLev_OpenPower(void)
{
    BAT_DEBUG("open power\n");
	LCD_Ctrl_Set(SW_OPEN);
    XR1151_EN_Open();
	is_5v_power_close = E_FALSE;
}

static void BatLev_GetPercent(uint16_t bat_volt)
{
   uint8_t i;
   //uint8_t usb_sta = 0;

   if(bat_volt <= BatLevMap[0].volt){ bat_lev_percent  = 0; }
   else if(bat_volt >= BatLevMap[BAT_LEVELS - 1].volt )
   { 
       bat_lev_percent  = 100; 
   }
   else
   {
      for(i = 1; i < BAT_LEVELS; i++)
      {
          if(BatLevMap[i - 1].volt <= bat_volt && bat_volt < BatLevMap[i].volt)
          {
             bat_lev_percent = BatLevMap[i - 1].percent;
             //BAT_DEBUG("bat lev = %d, bat_volt = %d.%03d V \r\n", i, bat_volt / 1000, bat_volt % 1000);
			 break;
          }
      }
   }
   BAT_DEBUG("bat=%02d%%, t = %ld\n", bat_lev_percent, os_get_tick());
   //usb_sta = VIN_DETECT_Read();
  
   if( (bat_lev_percent < 30) && (VIN_DETECT_Read() == 0) )  // 电量小于 30%, 关闭所有传感器
   {
       
       BatLev_ClosePower(1);
	   
	   //PowerCtrl_LowerPower();
   }
}


extern void ADCDrv_StartBatteryMeasure(void  (*end_exe_func)(uint16_t arg));

#if 0
static os_timer_t tTimerBatTest;

#include "board_version.h"


static uint8_t bat_flag = 0;

static void TimerBatTest_CallBack(void * arg)
{
   bat_flag ^= 1;
   if(bat_flag){
   	XR1151_EN_Close();
	printf("XR1151 close\n");
   }
   else 
   {
      XR1151_EN_Open();
	  printf("XR1151 open\n");
   }
}
#endif

uint8_t BatteryIsCharging(void)
{
    uint8_t sta = 0;
	uint16_t count = 0xFFF;

    if( BAT_CE_Read() == 0)  // 关闭了充电
    {
       BAT_CE_Set(SW_OPEN);
	   while((! BAT_CE_Read()) && count--);
	   sta = CHRG_Indicate_Read();
	   BAT_CE_Set(SW_CLOSE);
	   if(sta == 0)return E_TRUE;
	   else{ return E_FALSE;  }
    }
	else 
	{
	   if(CHRG_Indicate_Read() == 0)  // 说明正在充电
	      return E_TRUE;
	   else return E_FALSE;
	}
}

void BatLev_Init(void)
{
     ADCDrv_StartBatteryMeasure(BatLev_GetPercent);

	 //os_timer_setfn(&tTimerBatTest, TimerBatTest_CallBack, NULL);
	 //os_timer_arm(&tTimerBatTest, SEC(30), 1);
}

