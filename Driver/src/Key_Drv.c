
#include "Key_Drv.h"
#include "board_version.h"



#if   DEBUG_KEY_EN
#define KEY_DEBUG(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#else
#define KEY_DEBUG(...)
#endif



static uint8_t cur_display_mode = DISPLAY_CC;  // 当前显示模式

uint8_t is_rgb_on = 0;   // RGB 是否打开, 0: 默认不打开; 1: 打开
uint8_t is_debug_on = 0; // 调试打印开关

void key_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin = KEY1_GPIO_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;


	STM32_GPIOInit(KEY1_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = KEY2_GPIO_Pin;
	STM32_GPIOInit(KEY2_PORT, &GPIO_InitStructure);
	
	//GPIO_InitStructure.GPIO_Pin = KEY1_GPIO_Pin;
	//GPIO_Init(KEY3_PORT, &GPIO_InitStructure);
}

uint16_t key_scan(void)
{
	static uint8_t key_state = key_state_0;
	static uint8_t key_time = 0;
	static uint8_t key_value = 0;
	uint8_t key_press = 0;
	//uint8_t key_return = N_key;
	uint16_t key_result = 0x0000;	//高8位表示是否S_key/L_key/D_key
	
	key_press = KEY_INPUT;	//读取按键IO电平
	
	switch(key_state)
	{
			case key_state_0:	   // 按键初始态 
				if(key_press != NO_KEY)	
				{	
					key_state = key_state_1;	//键被按下，状态转换到按键消抖和确认状态 
				}
			break;
			case key_state_1:	   // 按键消抖与确认态 
			   if(key_press != NO_KEY)
			   {
			        key_value = key_press;	 //记录是哪个按键被按下
			        key_time = 0;
			        key_state = key_state_2;  // 按键仍然处于按下，消抖完成，状态转换到按下键时间的计时状态，但返回的还是无键事件 
			   }
			   else
			   {
			        key_state = key_state_0;
			   }
			break;
			case key_state_2:
				Sensor_KeepDisplayState();
				if(key_press == NO_KEY)
				{
				    //此时按键释放，说明是产生一次短操作，回送S_key
				    key_result = ((uint16_t)S_key << 8) | (key_value & KEY_MASK);  
				    key_state  = key_state_0;   // 转换到按键初始态 
				}
				else if(++key_time >= 200)    // 继续按下，计时加10ms（10ms为本函数循环执行间隔） 
				{
					key_result = ((uint16_t)L_key << 8) | (key_value & KEY_MASK);
					key_state  = key_state_3;  // 转换到等待按键释放状态 
				}
			break;
			case key_state_3:    // 等待按键释放状态，此状态只返回无按键事件 
			    if(key_press == NO_KEY)key_state = key_state_0;	
			break;
	}
	return key_result;	//返回按键结果
}

/*============= 
中间层按键处理函数，调用低层函数一次，
处理双击事件的判断，
返回上层正确的无键、单键、双键、长键4个按键事件。 
本函数由上层循环调用，间隔10ms 
===============*/ 

uint16_t key_read(void) 
{ 
    static uint8_t key_m = key_state_0;
	static uint8_t key_time_1 = 0; 
    uint16_t key_result = N_key;
	uint8_t  key_temp; 
    static uint16_t key_value;
	
	key_result = key_scan();
    key_temp  = (uint8_t)(key_result >> 8); 
     
    switch(key_m) 
    { 
        case key_state_0: 
            if (key_temp == S_key ) 
            { 
                 key_time_1 = 0;                // 第1次单击，不返回，到下个状态判断后面是否出现双击 
                 key_value  = key_result;
				 key_result = 0;
                 key_m = key_state_1; 
            } 
            //else 
            //     key_result = key_result;        // 对于无键、长键，返回原事件 
        break; 

        case key_state_1: 
            if (key_temp == S_key)             // 又一次单击（间隔肯定<500ms） 
            { 
                 key_result = ((uint16_t)D_key << 8) | (key_value & 0x00FF);
                 key_m = key_state_0; 
            } 
            else     // 这里500ms内肯定读到的都是无键事件，因为长键>1000ms，在1s前低层返回的都是无键                              
            {                                  
                 if(++key_time_1 >= 50) 
                 { 
                      // 500ms内没有再次出现单键事件，返回上一次的单键事件 
                      key_result = ((uint16_t)S_key << 8) | (key_value & 0x00FF);   
                      key_m = key_state_0;     // 返回初始状态 
                 } 
             } 
        break; 
    }
    return key_result; 
}     

#include "PM25Sensor.h"
#include "LED_Drv.h"
#include "os_global.h"
#include "BatteryLevel.h"
#include "LCD1602_Drv.h"
#include "Application.h"

uint8_t record_flag = 0;  

void key_process(uint16_t key)
{ 
    uint8_t key_state;
	uint8_t button;
	
    key_state = (uint8_t)(key >> 8);	//高8位为按键模式
    button    = (uint8_t)key;         // 低8位为哪个按键

	if(key_state != N_key)
	{
	   //KEY_DEBUG("key = 0x%x\n", key);
	   switch(button)
	   {
	      	case FUNC_KEY:
			{
				KEY_DEBUG("FUNC ");
				switch(key_state)
				{
				    case S_key:  
					{
						KEY_DEBUG("S\n");
					}break;
					case D_key:
					{
						KEY_DEBUG("D\n");
						record_flag ^= 1;
						if(record_flag)LCD1602_WriteCmd(0x0F);  // 光标闪烁
						else { LCD1602_WriteCmd(0x0C); }
					}break;
					case L_key:
					{
						KEY_DEBUG("L\n");
						Sensor_HoldDisplay();
					}break;
				}
			}break;
			case NEXT_KEY:
			{
				KEY_DEBUG("NEXT ");
				switch(key_state)
				{
				    case S_key:  
					{
						KEY_DEBUG("S\n");
						Sensor_SubStateToNext();
					}break;
					case D_key:
					{
						KEY_DEBUG("D\n");
						if(is_5v_power_close == E_FALSE)
						{
						    KEY_DEBUG("power down\n");
						    is_5v_power_close = E_TRUE;
                            BatLev_ClosePower(0);  // 关机
						}
						else
						{ 
						    KEY_DEBUG("power up\n");
						    is_5v_power_close = E_FALSE;
						    JumpToBootloader();
						}
						
					}break;
					case L_key:
					{
						KEY_DEBUG("L\n");
						HCHO_CaliSet();
					}break;
				}
			}break;
	   }
	}
}

// 获取当前显示模式
uint8_t key_get_cur_display_mode(void)
{
   return cur_display_mode;
}

