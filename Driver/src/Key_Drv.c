
#include "Key_Drv.h"
#include "board_version.h"



#if   DEBUG_KEY_EN
#define KEY_DEBUG(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#else
#define KEY_DEBUG(...)
#endif



static uint8_t cur_display_mode = DISPLAY_CC;  // ��ǰ��ʾģʽ

uint8_t is_rgb_on = 0;   // RGB �Ƿ��, 0: Ĭ�ϲ���; 1: ��
uint8_t is_debug_on = 1; // ���Դ�ӡ����

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
	uint16_t key_result = 0x0000;	//��8λ��ʾ�Ƿ�S_key/L_key/D_key
	
	key_press = KEY_INPUT;	//��ȡ����IO��ƽ
	
	switch(key_state)
	{
			case key_state_0:	   // ������ʼ̬ 
				if(key_press != NO_KEY)	
				{	
					key_state = key_state_1;	//�������£�״̬ת��������������ȷ��״̬ 
				}
			break;
			case key_state_1:	   // ����������ȷ��̬ 
			   if(key_press != NO_KEY)
			   {
			        key_value = key_press;	 //��¼���ĸ�����������
			        key_time = 0;
			        key_state = key_state_2;  // ������Ȼ���ڰ��£�������ɣ�״̬ת�������¼�ʱ��ļ�ʱ״̬�������صĻ����޼��¼� 
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
				    //��ʱ�����ͷţ�˵���ǲ���һ�ζ̲���������S_key
				    key_result = ((uint16_t)S_key << 8) | (key_value & KEY_MASK);  
				    key_state  = key_state_0;   // ת����������ʼ̬ 
				}
				else if(++key_time >= 200)    // �������£���ʱ��10ms��10msΪ������ѭ��ִ�м���� 
				{
					key_result = ((uint16_t)L_key << 8) | (key_value & KEY_MASK);
					key_state  = key_state_3;  // ת�����ȴ������ͷ�״̬ 
				}
			break;
			case key_state_3:    // �ȴ������ͷ�״̬����״ֻ̬�����ް����¼� 
			    if(key_press == NO_KEY)key_state = key_state_0;	
			break;
	}
	return key_result;	//���ذ������
}

/*============= 
�м�㰴�������������õͲ㺯��һ�Σ�
����˫���¼����жϣ�
�����ϲ���ȷ���޼���������˫��������4�������¼��� 
���������ϲ�ѭ�����ã����10ms 
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
                 key_time_1 = 0;                // ��1�ε����������أ����¸�״̬�жϺ����Ƿ����˫�� 
                 key_value  = key_result;
				 key_result = 0;
                 key_m = key_state_1; 
            } 
            //else 
            //     key_result = key_result;        // �����޼�������������ԭ�¼� 
        break; 

        case key_state_1: 
            if (key_temp == S_key)             // ��һ�ε���������϶�<500ms�� 
            { 
                 key_result = ((uint16_t)D_key << 8) | (key_value & 0x00FF);
                 key_m = key_state_0; 
            } 
            else     // ����500ms�ڿ϶������Ķ����޼��¼�����Ϊ����>1000ms����1sǰ�Ͳ㷵�صĶ����޼�                              
            {                                  
                 if(++key_time_1 >= 50) 
                 { 
                      // 500ms��û���ٴγ��ֵ����¼���������һ�εĵ����¼� 
                      key_result = ((uint16_t)S_key << 8) | (key_value & 0x00FF);   
                      key_m = key_state_0;     // ���س�ʼ״̬ 
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

#if GIZWITS_TYPE
#include "gizwits_port.h"
#include "os_timer.h"
#include "gizwits_protocol.h"
#include "gizwits_product.h"

uint8_t allow_config_wifi = 0;
uint32_t config_wifi_timeout = 0;   // 10 sec ��ʧЧ
os_timer_t tTimerChkAllowConfigWifi;
void TimerChkAllowConfigWifi(void * arg)
{
         allow_config_wifi = 0;
}
#endif

void key_process(uint16_t key)
{ 
    uint8_t key_state;
	uint8_t button;
	
    key_state = (uint8_t)(key >> 8);	//��8λΪ����ģʽ
    button    = (uint8_t)key;         // ��8λΪ�ĸ�����

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

						#if GIZWITS_TYPE
						KEY_DEBUG("allow user to config wifi\r\n");
						allow_config_wifi = 1;
						config_wifi_timeout = OS_SetTimeout(SEC(10));
						os_timer_setfn(&tTimerChkAllowConfigWifi,  TimerChkAllowConfigWifi,  NULL);
						os_timer_arm(&tTimerChkAllowConfigWifi, SEC(10),  0);
                               	       #endif
                               	       
					}break;
					case D_key:
					{
						KEY_DEBUG("D\n");

				       #if GIZWITS_TYPE
				              if(allow_config_wifi)
				              {
    						       KEY_DEBUG("enter WiFi AirLink connect mode \r\n");

    						       allow_config_wifi = 0;
    				                     #if GIZWITS_TYPE == GIZ_MCU			
                                   		 gizwitsSetMode(WIFI_AIRLINK_MODE);
                                                 #elif GIZWITS_TYPE == GIZ_SOC
                                                  gizwits_set_wifi_mode(WIFI_AIRLINK_MODE);
                                                 #endif
                                           }
                                  #endif
                                   
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
                                              BatLev_ClosePower(0);  // �ػ�
						}
						else
						{ 
						    KEY_DEBUG("power up\n");
						    is_5v_power_close = E_FALSE;
							AppInit();
						    //JumpToBootloader();
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

// ��ȡ��ǰ��ʾģʽ
uint8_t key_get_cur_display_mode(void)
{
   return cur_display_mode;
}

