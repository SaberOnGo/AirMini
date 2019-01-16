
#ifndef __KEY_DRV_H__
#define  __KEY_DRV_H__

#include "stm32f10x.h"
#include "GlobalDef.h"



/****************************硬件定义 begin  ****************************************/

/****************************硬件定义 end ****************************************/

#define key_state_0 	0
#define key_state_1		1
#define key_state_2		2
#define key_state_3	    3
#define key_state_4		4	//按键需退出检测


//按键状态
#define N_key       0             //无键 
#define S_key       1             //单键
#define D_key       2             //双击
#define L_key       3             //长键 


#define KEY_MASK    0x06  // 只有2个按键, bit2-bit1
#define NO_KEY      0x06  // 

#define KEY_1       0x04  // bit1 为 0, 而 KEY2 未被按下, 所以bit2 还是为1  --> 0x04
#define KEY_2       0x02  // bit2 为 0, 而 KEY1 未被按下, 所以bit1 还是为1  --> 0x02

#define FUNC_KEY    KEY_1
#define NEXT_KEY    KEY_2



#define DISPLAY_CC      0   // 显示 PM2.5 PM10 浓度
#define DISPLAY_PC      1   // 显示 PM2.5 PM10 粒子数
#define DISPLAY_AQI_US  2   // 显示 AQI US 标准
#define DISPLAY_AQI_CN  3   // 显示 AQI CN 标准
#define DISPLAY_HCHO    4   // 显示 甲醛浓度

#define HCHO_DISPLAY_MASSFRACT   0  // 显示质量分数, ug/m3
#define HCHO_DISPLAY_PPM         1  // 显示体积分数, ppm


extern uint8_t is_rgb_on;
extern uint8_t is_debug_on; // 调试打印开关

extern uint8_t record_flag;

//#if  (SENSOR_SELECT == HCHO_SENSOR)
extern void HCHO_DisplayInit(uint8_t display_mode);
//#else
extern void PM25_Display_Init(uint8_t display_mode);
//#endif
void Sensor_KeepDisplayState(void);
void Sensor_HoldDisplay(void);

void key_gpio_init(void);
uint16_t key_scan(void);
uint16_t key_read(void);

void key_process(uint16_t keyval);
uint8_t key_get_cur_display_mode(void);

#endif

