
#ifndef __KEY_DRV_H__
#define  __KEY_DRV_H__

#include "stm32f10x.h"
#include "GlobalDef.h"



/****************************Ӳ������ begin  ****************************************/

/****************************Ӳ������ end ****************************************/

#define key_state_0 	0
#define key_state_1		1
#define key_state_2		2
#define key_state_3	    3
#define key_state_4		4	//�������˳����


//����״̬
#define N_key       0             //�޼� 
#define S_key       1             //����
#define D_key       2             //˫��
#define L_key       3             //���� 


#define KEY_MASK    0x06  // ֻ��2������, bit2-bit1
#define NO_KEY      0x06  // 

#define KEY_1       0x04  // bit1 Ϊ 0, �� KEY2 δ������, ����bit2 ����Ϊ1  --> 0x04
#define KEY_2       0x02  // bit2 Ϊ 0, �� KEY1 δ������, ����bit1 ����Ϊ1  --> 0x02

#define FUNC_KEY    KEY_1
#define NEXT_KEY    KEY_2



#define DISPLAY_CC      0   // ��ʾ PM2.5 PM10 Ũ��
#define DISPLAY_PC      1   // ��ʾ PM2.5 PM10 ������
#define DISPLAY_AQI_US  2   // ��ʾ AQI US ��׼
#define DISPLAY_AQI_CN  3   // ��ʾ AQI CN ��׼
#define DISPLAY_HCHO    4   // ��ʾ ��ȩŨ��

#define HCHO_DISPLAY_MASSFRACT   0  // ��ʾ��������, ug/m3
#define HCHO_DISPLAY_PPM         1  // ��ʾ�������, ppm


extern uint8_t is_rgb_on;
extern uint8_t is_debug_on; // ���Դ�ӡ����

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

