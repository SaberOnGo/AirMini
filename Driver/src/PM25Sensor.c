
#include "PM25Sensor.h"
#include "LCD1602_Drv.h"
#include "os_timer.h"
#include "uart_queue.h"
#include "LED_Drv.h"
#include "Key_Drv.h"
#include "delay.h"

#include "DHT11.h"
#include "F10X_Flash_If.h"
#include "SDRR.h"





#if PM25_DEBUG_EN
#define PM25_DEBUG(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#else
#define PM25_DEBUG(...)
#endif

#define ON_USING    0
#define ON_IDLE     1

static uint8_t volatile is_using = 1;  // �ź���

#define Take_PM25_Sem()    if(is_using)is_using--  // 
#define Release_PM25_Sem() is_using++
#define TryTakePM25_Sem()  is_using


static volatile T_PM25CmdContent tPM25CmdContent;

#if 1
static const 
#else
static __eeprom	
#endif

uint8_t AqiLevelString[7][16] = {
{"Good          " },
{"Moderate      "},
{"Unhealthy     "},
{"Unhealthy     "},
{"Very Unhealthy"},
{"Just In Hell  "},
{"Heavy In Hell "},
};

//���� E_AQI_STD
static const uint8_t AqiStdString[][7] = {
{"AQI US"},
{"AQI CN"},
};

// AQI �ȼ�, �� 7 ��
static const T_AQI AqiLevel[] = 
{
       #if 0
// C_low_us C_high_us, C_low_cn, C_high_cn, aqi_low,  aqi_high 
  	{0,       15.4,       0,         35,          0,          50  }, 
	{15.5,    40.4,      35.1,      75,          51,         100 }, 
  	{40.5,    65.4,      75.1,      115,         101,       150 },
  	{65.5,    150.4,     115.1,     150,        151,        200 },
    {150.5,   250.4,     150.1,     250,        201,        300},
    {250.5,   350.4,     250.1,     350,        301,        400},
    {350.5,   500.4,     350.1,     500,        401,        500},
    #else
        // real val * 10, Ϊ��ʵֵ��10��
// C_low_us C_high_us, C_low_cn, C_high_cn, aqi_low,  aqi_high 
  	{0,       154,       0,         350,          0,          50  }, 
	{155,    404,      351,      750,          51,         100 }, 
  	{405,    654,      751,      1150,         101,       150 },
  	{655,    1504,     1151,     1500,        151,        200 },
    {1505,   2504,     1501,     2500,        201,        300},
    {2505,   3504,     2501,     3500,        301,        400},
    {3505,   5004,     3501,     5000,        401,        500},
    #endif
};

#define AQI_LEVEL_STR_SIZE  ( sizeof(AqiLevelString) / sizeof(AqiLevelString[0]) )  
#define AQI_LEVEL_SIZE      (sizeof(AqiLevel) / sizeof(AqiLevel[0]))


#define DISPLAY_INTERNAL_SEC     3    // 3 sec ��ʾһ��
#define HCHO_PPB_ARRAY_SIZE      18  // ����Ľ��ռ�ȩ��������������Ŀ

// ��ȩ���������ݴ���
typedef struct
{
     uint16_t rx_ppb[HCHO_PPB_ARRAY_SIZE];  // ���յļ�ȩŨ��ֵ������
     uint16_t rx_ppb_count;     // ���յ����ݰ�����
	 uint16_t new_mass;         // ���µ���������
	 uint16_t aver_mass;        // ƽ��HCHO ��������, ug/m3
	 uint16_t new_ppb;          // ���µļ�ȩֵ
	 uint16_t aver_ppb;         // ƽ�� ppb ֵ = 0.001 ppm
	 uint16_t cali_ppb;         // У��ֵ
	 uint16_t cali_mass;
	 uint8_t  is_disp_hcho;       // �Ƿ���Ļ��ʾ
	 E_BOOL  is_power_first;     // �Ƿ��һ����ʾ
	 uint8_t  is_cal;              // �Ƿ�У��: 0 ��У��; 1: У��
}T_HCHO_Analysis;

// ���������չ���
typedef struct
{
    uint8_t   rx_buf[32];    // ���ջ�����
	volatile uint16_t  rx_count;      // ���ռ���
	volatile uint8_t   last_val;      // ��һ�ν��յ�ֵ
	//uint8_t   existed;       // �������Ƿ����
	//uint32_t  ttl;            // ����ʱ��
	volatile uint8_t   is_rx;         // �����ձ�־: �Ƿ��ѽ��յ�
}T_SensorRx;	  

// ��������ʾ״̬
typedef enum
{
    SENSOR_STA_IDLE = 0,
		
	#if 1
	SENSOR_STA_HCHO = 1,
	SENSOR_STA_PM25 = 2,
	SENSOR_STA_TEMPHUMI = 3,  // ��ʪ����ʾ״̬
	SENSOR_STA_TVOC  = 4, 
	SENSOR_STA_TIME  = 5,   
	SENSOR_STA_NONE,     // �޴�����״̬
	SENSOR_STA_END
	#else
	SENSOR_STA_TEMPHUMI = 1,
    SENSOR_STA_TVOC  = 2, 
    SENSOR_STA_NONE = 3,
    
    
    SENSOR_STA_HCHO,
	SENSOR_STA_PM25,
	SENSOR_STA_END,
	#endif
	
}E_SENSOR_STATE;


#define MAX_MASTER_STATE_SEC    10   // ��״̬��ʾʱ��
#define HOLD_STATE_SEC          6    // ״̬����ʱ��

// ��������ʾ����
typedef struct
{
   uint8_t  master_state;    // ����ʾ״̬
   uint8_t  sub_state;       // ��״̬
   uint8_t  last_master_state; // ��һ����״̬
   uint16_t master_sec;      // ��״̬��ʾ��ʱ��
   uint8_t  hold;            // �Ƿ񱣳���ʾ
}T_SensorDisplay;



T_SensorDisplay SensorDisp;   // ������������ʾ����

// ��ʪ��ֵ
T_TempHumi tTempHumi;
uint16_t g_hcho_mass = 0;
uint16_t  g_pm2p5_ug;
uint16_t   g_pm10_ug;



// ���յļ�ȩ���ݷ�������
static volatile T_HCHO_Analysis tHchoAna;
	
// ���ջ���������
static T_SensorRx HchoRx;
static T_SensorRx PM25Rx;

static os_timer_t tTimerCalculator;  // �������ݵĶ�ʱ��, �Ѽ������ݵ��߼����ַ����ж����洦��
static os_timer_t tTimerPM25Store;   // �洢PM2.5����

//static volatile uint8_t  rx_complete = 0;  // �������

static void LCD_Display_CC(uint16_t pm25, uint16_t pm10)
{
	
	LCD1602_WriteString(0,  0, "PM2.5 ");
    LCD1602_WriteInteger(0, 6, pm25, 3);
    LCD1602_WriteString(0, 10, "ug/m3");

	LCD1602_WriteString(1,  0, "PM10  ");
    LCD1602_WriteInteger(1, 6, pm10, 3);
    LCD1602_WriteString(1, 10, "ug/m3");
}

static void LCD_Display_PC(uint16_t pc0p3um, uint16_t pc2p5um)
{
	
	LCD1602_WriteString(0,  0, "> 0.3um ");
    LCD1602_WriteInteger(0, 8, pc0p3um, 5);

	LCD1602_WriteString(1,  0, "> 2.5um ");
    LCD1602_WriteInteger(1, 8, pc2p5um, 5);
}

// AQI ����
// ����: uint16_t pm25: PM25 Ũ��, 
// E_AQI_STD aqi_std: ��׼ѡ��
// uint8_t * level: �����aqi�ȼ�: 0-6 : ��ʾAQI �ȼ�: 1-7
// ����ֵ: AQI
static uint16_t PM25_AqiCalculate(uint16_t pm25, E_AQI_STD aqi_std, uint8_t *level)
{
   // float c_low = 0, c_high = 0;
      uint16_t c_low = 0, c_high = 0;
	uint8_t i;
   // float deltaC;     // Ũ�Ȳ�ֵ
     uint16_t deltaC;
     
    uint16_t deltaI;  // AQI ��ֵ
	uint16_t aqi = 0;
	
	if(pm25 > 500)pm25 = 500;

	pm25 *= 10;
	if(aqi_std == AQI_CN)
	{
	    for(i = 0; i < AQI_LEVEL_SIZE; i++)
		{
		    if(AqiLevel[i].C_low_cn <= pm25 && pm25 <= AqiLevel[i].C_high_cn)
		    {
		              c_low  = AqiLevel[i].C_low_cn;
				c_high = AqiLevel[i].C_high_cn;
				break;
		    }
		}
	}
	else
	{
	    for(i = 0; i < AQI_LEVEL_SIZE; i++)
		{
		    if(AqiLevel[i].C_low_us <= pm25 && pm25 <= AqiLevel[i].C_high_us)
		    {
		          c_low  = AqiLevel[i].C_low_us;
			   c_high = AqiLevel[i].C_high_us;
			   break;
		    }
		}
	}

    if(i < AQI_LEVEL_SIZE)	
    {
              deltaC = c_high - c_low;
		deltaI = AqiLevel[i].I_high - AqiLevel[i].I_low;
		aqi = (uint16_t)(((double)deltaI) * (pm25 - c_low) / deltaC + AqiLevel[i].I_low);
    }
	else{ i = 0; }

	*level = i;
	return aqi;
}

// ��ʾAQI �ȼ�
static void LCD_Display_AQI(uint16_t pm25, E_AQI_STD aqi_std)
{
    uint16_t aqi;
    uint8_t level = 0;
   
    aqi = PM25_AqiCalculate(pm25, aqi_std, &level);

	//if(is_rgb_on)LED_AqiIndicate((E_AQI_LEVEL)level);
	LCD1602_WriteString(0,  0, (const uint8_t *)(&AqiStdString[(uint8_t)aqi_std][0]) );
    LCD1602_WriteInteger(0, 7, aqi, 3);
	LCD1602_WriteString(1,  0, (const uint8_t *)&AqiLevelString[level][0] );
}

#if 0
static void LCD_Display_HCHO(uint16_t hcho)
{
   uint16_t temp, left;

   LCD1602_ClearScreen();
   
   temp = hcho / 1000;  // mg
   left = hcho % 1000 / 10;  // ����2λ
   LCD1602_WriteString (0,  0, "HCHO ");
   LCD1602_WriteInteger(0,  5, temp, 3);  // ��������
   LCD1602_WriteString (0,  8, ".");
   LCD1602_SetXY(0, 9);
   LCD1602_WriteData(left / 10 + 0x30);
   LCD1602_WriteData(left % 10 + 0x30);
   LCD1602_WriteString (0,  11, "mg/m3");
}
#endif

// ��ʾģʽ��ʼ��
void PM25_Display_Init(uint8_t display_mode)
{
    uint16_t temp;
	LCD1602_ClearScreen();
    switch(display_mode)
    {
        case DISPLAY_CC:  // ��ʾŨ��
        {
			//LED_StopAqiIndicate();
			temp = tPM25CmdContent.pm25_air;  // ���� volatile ��������ľ���
			LCD_Display_CC(temp, tPM25CmdContent.pm10_air);
        }break;
		case DISPLAY_PC:  // ��ʾ���Ӽ���
        {
			//LED_StopAqiIndicate();

			temp = 0;
			temp += tPM25CmdContent.PtCnt_2p5um;  
			temp += tPM25CmdContent.PtCnt_5p0um;
			temp += tPM25CmdContent.PtCnt_10p0um;
			LCD_Display_PC(tPM25CmdContent.PtCnt_0p3um, temp);
        }break;
		case DISPLAY_AQI_US:
        {
			LCD_Display_AQI(tPM25CmdContent.pm25_air, AQI_US);
        }break;
		case DISPLAY_AQI_CN:
        {
			LCD_Display_AQI(tPM25CmdContent.pm25_air, AQI_CN);
        }break;
		
		#if (SENSOR_SELECT == PMS5003S)
		case DISPLAY_HCHO:  // ��ʾ��ȩ, �޼�ȩ����ʾ�汾��
        {
			//LED_StopAqiIndicate();
			//LCD_Display_HCHO(tPM25.extra.hcho); 
        }break;
        #endif
    }
	if(SensorDisp.hold == SENSOR_STA_PM25)
   	  	 LCD1602_WriteString(1, 15, "H");
}
// ��ʾֵ, ֻ�ı���ʾ����ֵ
//static 
void PM25_Display_Value(uint8_t display_mode)
{
     uint16_t temp;
     switch(display_mode)
     {
        case DISPLAY_CC:  // ��ʾŨ��
        {
			//LED_StopAqiIndicate();
			LCD1602_WriteInteger(0, 6, tPM25CmdContent.pm25_air, 3);
			LCD1602_WriteInteger(1, 6, tPM25CmdContent.pm10_air, 3);
        }break;
		case DISPLAY_PC:
		{
			//LED_StopAqiIndicate();
			// ���� volatile ��������ľ���
			temp = 0;
			temp += tPM25CmdContent.PtCnt_2p5um;  
			temp += tPM25CmdContent.PtCnt_5p0um;
			temp += tPM25CmdContent.PtCnt_10p0um;
			LCD1602_WriteInteger(0, 8, tPM25CmdContent.PtCnt_0p3um, 5);
			LCD1602_WriteInteger(1, 8, temp, 5);
		}break;
		case DISPLAY_AQI_US:
		{
			LCD_Display_AQI(tPM25CmdContent.pm25_air, AQI_US); 
		}break;
		case DISPLAY_AQI_CN:
		{
			LCD_Display_AQI(tPM25CmdContent.pm25_air, AQI_CN);
		}break;
		#if (SENSOR_SELECT == PMS5003S)
		case DISPLAY_HCHO:
		{
			//LED_StopAqiIndicate();
        }break;
        #endif
    }
	if(SensorDisp.hold == SENSOR_STA_PM25)
   	  	 LCD1602_WriteString(1, 15, "H");
}



// ���ݽ��մ���
void PM25_Receive(uint8_t * buf, uint16_t  len)
{
    uint16_t *p;   // ������������ʼָ��
	uint8_t i;
	uint16_t new_sum = 0;  // �������У���
	uint16_t sum_len;      // У�鳤��
	
    if(buf[0] != 0x42 && buf[1] != 0x4d)
    {
		return;
    }

	Take_PM25_Sem();
	tPM25CmdContent.len = (((uint16_t)buf[2]) << 8) + buf[3];

	p = (uint16_t *)&tPM25CmdContent.pm1_cf1;
	for(i = 0; i < 14; i++)  // ����У���
	{
	   p[i] = (((uint16_t)buf[4 + i * 2]) << 8) + buf[5 + i * 2];
	}
    
    sum_len = 4 + len - 2;  // У������ݳ���, ��У������
	for(i = 0; i < sum_len; i++)
	{
	   new_sum += buf[i];
	}
    if(new_sum == tPM25CmdContent.sum)
    {
       PM25Rx.is_rx = 1;  // ���յ�PM25����
       os_timer_arm(&tTimerPM25Store, 0, 0);  // �洢PM2.5����
    }
	Release_PM25_Sem();  // �ͷ��ź���
}

#include "Uart_Drv.h"

static uint8_t Cmd_Buf[9] = {0xFF, 0x01, 0x78, 0x41, 0x00, 0x00, 0x00, 0x00, 0x46};
static void HCHO_SendCmd(uint8_t buf_2, uint8_t buf_3)
{
    Cmd_Buf[2] = buf_2;
	Cmd_Buf[3] = buf_3;
	Cmd_Buf[8] = HCHO_CheckSum(Cmd_Buf, 9);
	Uart_SendDatas(Cmd_Buf, 9);
}
// �л�����ѯģʽ
void HCHO_SwitchToQueryMode(void)
{
    HCHO_SendCmd(0x78, 0x41);
}
// �л�������ģʽ
void HCHO_SwitchToActiveMode(void)
{
    HCHO_SendCmd(0x78, 0x40);
}
// ��ѯ����Ũ��
void HCHO_QueryHCHOConcentration(void)
{
    HCHO_SendCmd(0x86, 0x00);
}
static uint16_t CalculateHchoAver(uint8_t start, uint8_t div)
{
    uint32_t sum = 0;
    uint8_t i;		   
	for(i = start; i < HCHO_PPB_ARRAY_SIZE; i++)
	{
	  sum += tHchoAna.rx_ppb[i];
	}
	return (uint16_t)((double)sum / div);
}
// uint8_t div: ����
static uint16_t HCHO_CalculateAverageVal(uint8_t div)
{
	return CalculateHchoAver(0, div);
	// ��ʽ: X = C * M / 22.4
	// X: ug/m3, C: ppb, M: ������Է�������, 22.4: ��׼����ѹ�¿�����Է�������
	//mass_fraction =	(uint16_t)((double)aver_ppb * 30 / 22.4); 
}


static void TimerPM25Store_CallBack(void * arg)
{
    PM25_DEBUG("pm2.5 = %d ug/m3\n", tPM25CmdContent.pm25_air);
    SDRR_SaveSensorPoint(SENSOR_PM25, (void *)&tPM25CmdContent.pm25_air);

    g_pm2p5_ug = tPM25CmdContent.pm25_air;
    g_pm10_ug   = tPM25CmdContent.pm10_air;
}

// ��ȩŨ�����߱仯ƽ������
// �仯���Ʋ�Ҫ̫����ұ仯, ƽ���仯����, ���ƴ�������������
#if 0
static
uint16_t TrendCurveSmooth(uint16_t new_val)
{
      double delta = 0.0;	         // ���µĴ�����������֮ǰƽ��ֵ�Ĳ�ֵ, ��ֵ
	  uint8_t is_negative = 0;	 // �Ƿ�Ϊ��ֵ, 1: ��ֵ; 0: �Ǹ�ֵ
	  uint16_t average = 0;
   	
	  average = HCHO_CalculateAverageVal(HCHO_PPB_ARRAY_SIZE);
	  if(new_val >= average)  // ��ȩŨ�ȳ���������
   {
		 delta = new_val - average;
	  }
	  else	// ���µ�Ũ�ȱ�ƽ��ֵŨ��С, ��ȩŨ�ȳ��½�����
	  {
		 delta = average - new_val;
		 is_negative = 1;
	  }
      // 10 ���ڱ仯���ܳ��� 1 ug
      // (M + 1)* M * delta / (2N) <= 1ug, ���� M Ϊ ��ʾ����ڻ�ȡ�Ĵ���������, ������� 1 �� ��ȡһ������, N Ϊ��������
      //max_change_val = ((double)2 * HCHO_PPB_ARRAY_SIZE) / ((DISPLAY_INTERNAL_SEC + 1) * DISPLAY_INTERNAL_SEC);
	  if(delta >= 1.5)
	  {
	     delta = 1.5;
	  }
      if(is_negative)
      {
          if(average >= delta)average = (uint16_t)(average - delta);
		  else { average = 0;  }
      }
	  else
	  {
	      average = (uint16_t)(average + delta);
	  }
	  return average;
}
#endif

static const u16 U16_MAX = 0xFFFF;

// ���㴫�������ݵĶ�ʱ�ص�
static void TimerCalculator_CallBack(void * arg)
{
    uint8_t i;
    uint16_t smoothed_ppb = 0;  // ƽ��������µĲ�������
    uint32_t sec = OS_GetSysTick() / 100;  // ��ǰϵͳ������ֵ
    uint8_t  flush_hcho = E_FALSE; // �Ƿ�ˢ�¼�ȩ����
	
	#if 0
	if( sec > 120)  // ϵͳ������120 s��
	{
	     smoothed_ppb = TrendCurveSmooth(new_ppb);
	}
    else
    {
         smoothed_ppb = new_ppb;
    }
	#else
    smoothed_ppb = tHchoAna.new_ppb;
	#endif

	 if(is_debug_on)os_printf("tick = %ld, new_ppb = %d ug/m3\n", OS_GetSysTick(), tHchoAna.new_ppb);
	
	for(i = 1; i < HCHO_PPB_ARRAY_SIZE; i++)
	{
	  tHchoAna.rx_ppb[i - 1] = tHchoAna.rx_ppb[i];
	}
	tHchoAna.rx_ppb[HCHO_PPB_ARRAY_SIZE - 1] = smoothed_ppb; // ���µ�ֵ�������, FIFO
    tHchoAna.rx_ppb_count++;
	
	if(tHchoAna.is_power_first)
	{
	    if(tHchoAna.rx_ppb_count > 8)  // ���� 8 �μ���ʼ����ƽ��ֵ
	    {
	       // ��ʽ: X = C * M / 22.4
	       // X: ug/m3, C: ppb, M: ������Է�������, 22.4: ��׼����ѹ�¿�����Է�������
	       tHchoAna.aver_ppb  = CalculateHchoAver(HCHO_PPB_ARRAY_SIZE - 3, 3);  // ֻ�������3��
		   tHchoAna.aver_mass =	(uint16_t)((double)tHchoAna.aver_ppb * 30 / 22.4); 
		   flush_hcho = E_TRUE;
		   tHchoAna.is_disp_hcho = E_TRUE;  // ��ʾ��ȩֵ
	    }
		if(tHchoAna.rx_ppb_count >= HCHO_PPB_ARRAY_SIZE)
	    {
			tHchoAna.is_power_first = E_FALSE;
			tHchoAna.rx_ppb_count = 0;
		}
	}
	else
	{
	    if(tHchoAna.rx_ppb_count >= DISPLAY_INTERNAL_SEC)  // ���������ʾһ�����¼�����ļ�ȩֵ
	    {
	        // ��ʽ: X = C * M / 22.4
	        // X: ug/m3, C: ppb, M: ������Է�������, 22.4: ��׼����ѹ�¿�����Է�������
	        if(sec < 120)
	        {
	           tHchoAna.aver_ppb = CalculateHchoAver(HCHO_PPB_ARRAY_SIZE - 3, 3);
	        }
			else
			{
			   tHchoAna.aver_ppb = HCHO_CalculateAverageVal(HCHO_PPB_ARRAY_SIZE);
			}
	        tHchoAna.aver_mass =	(uint16_t)((double)tHchoAna.aver_ppb * 30 / 22.4); 
			flush_hcho = E_TRUE;
            tHchoAna.rx_ppb_count = 0;
	        tHchoAna.is_disp_hcho = E_TRUE;  // ��ʾ ��ȩֵ
		}
	}

	if(is_debug_on)PM25_DEBUG("aver_ppb = %d, aver_mass = %d ug/m3\n", tHchoAna.aver_ppb, tHchoAna.aver_mass);
		
    HchoRx.is_rx     = 0;  // 
	tHchoAna.new_ppb = 0;  // ������0

    if(flush_hcho)
    {
    #if USE_TVOC_CAL
		 if(tHchoAna.aver_mass < 30)
		 {
            if(tTempHumi.tvoc < U16_MAX)
            {
               if(tTempHumi.tvoc <= 100)
			   	  tHchoAna.aver_mass = tTempHumi.tvoc / 10;  // 10 ug ����
               else if(tTempHumi.tvoc < 500)  // 500 ug
                  tHchoAna.aver_mass = tTempHumi.tvoc / 6;  // 30 ug ����
               else
			   	  tHchoAna.aver_mass = tTempHumi.tvoc / 8;
            }
			else
			{
			    if(tHchoAna.aver_mass < 4)tHchoAna.aver_mass = 10;
				else tHchoAna.aver_mass += 5;
			}
		 }
		 else if(tHchoAna.aver_mass < 1000)
		 {
		    if(tTempHumi.tvoc < U16_MAX)
		       tHchoAna.aver_mass = (tHchoAna.aver_mass + (tTempHumi.tvoc / 5.0)) / 2.0;
		 }
		 
    #else
		if(tHchoAna.aver_mass < 30)
		{
		    if(tTempHumi.tvoc < U16_MAX)
		    {
		       if(tTempHumi.tvoc < 150)
			   	  tHchoAna.aver_mass = tTempHumi.tvoc / 4;  // 10 ug ����
			   else if(tTempHumi.tvoc < 500)  // 500 ug
                  tHchoAna.aver_mass = tTempHumi.tvoc / 6;  // 30 ug ����
               else
			   	  tHchoAna.aver_mass = tTempHumi.tvoc / 10;
		    }
			else
			{
			   if(tHchoAna.aver_mass < 4)tHchoAna.aver_mass = 10;
			   else tHchoAna.aver_mass += 5;
			}
		}
	#endif
		tHchoAna.aver_ppb = (uint16_t)((double)tHchoAna.aver_mass * 22.4 / 30); 
	
        SDRR_SaveSensorPoint(SENSOR_HCHO, (void *)&tHchoAna.aver_mass);
    }

    g_hcho_mass = tHchoAna.aver_mass;

    
    //if(is_disp_hcho)
	   //os_printf("tick = %ld, aver_ppb = %d ppm, aver_mass = %d ug/m3\n", Sys_GetRunTime(), aver_ppb, mass_fraction);
}

void HCHO_Receive(uint8_t * buf, uint16_t len)
{
   if(buf[0] == 0xFF)
   {
      if(buf[1] == 0x17)  // ΪHCHO Sensor �������͵Ĵ���������
      {
		 tHchoAna.new_ppb = ((uint16_t)(buf[4] << 8)) + buf[5];  
		 HchoRx.is_rx = 1;  // HCHO Sensor �������͵Ĵ���������
		 os_timer_arm(&tTimerCalculator, 0, 0);  // ���������ص�
      }
	  else if(buf[1] == 0x86)
	  {
	     HchoRx.is_rx  = 2; // HCHO Sensor �������͵Ĵ�������Ӧ����
		 tHchoAna.new_mass = ((uint16_t)buf[2] << 8) + buf[3]; // ����Ũ��, ��λ: ug / m3
	  }
   }
}
// ��ʾ��������, mg/m3
#if 0
#define HCHO_DisplayMassFract(val)  LCD1602_WriteInteger(0,  5, val, 5)
#else
static void HCHO_DisplayMassFract(uint16_t val)
{
   LCD1602_SetXY(0, 5);
   LCD1602_WriteData(val % 10000 / 1000 + 0x30);  // ��������
   LCD1602_WriteData('.');
   LCD1602_WriteData(val % 1000 / 100 + 0x30);
   LCD1602_WriteData(val % 100 / 10 + 0x30);
  // LCD1602_WriteData(val % 10 + 0x30);
}

#endif
// ��ʾ��ȩ���������ĳ�ʼ��
static void HCHO_DisplayMassFractInit(uint16_t val)
{
   LCD1602_ClearScreen();
   LCD1602_WriteString (0,  0, "HCHO ");
   HCHO_DisplayMassFract(val);     
   LCD1602_WriteString (0,  10, " mg/m3");
}
// ��ʾ��ȩ��PPM��ֵ: x.xx ppm
static void HCHO_DisplayPPM(uint16_t val)
{
   LCD1602_SetXY(0, 5);
   LCD1602_WriteData(val % 10000 / 1000 + 0x30);  // ��������
   LCD1602_WriteData('.');
   LCD1602_WriteData(val % 1000 / 100 + 0x30);
   LCD1602_WriteData(val % 100 / 10 + 0x30);
   //LCD1602_WriteData(val % 10 + 0x30);
}

// ��ʾ��ȩ�����������ʼ��, ppm
// val: ppb ֵ
static void HCHO_DisplayPPMInit(uint16_t val)
{
   LCD1602_ClearScreen();
   LCD1602_WriteString (0,  0, "HCHO ");
   HCHO_DisplayPPM(val);
   LCD1602_WriteString (0,  10, " ppm  ");
}

// ����ʪ����ʾ
// 0 - 39%d: Too Dry
// 40 - 65:  Comfort
// 66 - 100%: Humidity
void TempHumi_Indicate(uint16_t val)
{
    if(val < 20)
    {
       LCD1602_WriteString(1,  1, "Too Dry    ");
    }
	else if(val < 40)
	{
	   LCD1602_WriteString(1,  1, "Dry        ");
	}
	else if(val < 66)
	{
	   LCD1602_WriteString(1,  1, "Comfort    ");
	}
	else if(val < 75)
	{
	   LCD1602_WriteString(1,  1, "Wet        ");
	}
	else
	{
	   LCD1602_WriteString(1,  1, "Too Wet    ");
	}
}

// ��ʾ��ʪ��
// ��8λΪ�¶�ֵ, ��8λΪʪ��ֵ
static void TempHumi_DisplayValue(uint8_t display_mode)
{
   uint8_t val = 0;
   
   LCD1602_SetXY(0, 0);
   val = tTempHumi.temp & 0x8000;   // �¶���������
   if(val)LCD1602_WriteData('-'); // �¶�Ϊ��ֵ
   else  LCD1602_WriteData(' ');

   tTempHumi.temp &= 0x7FFF;  // ȥ�����ܵĸ���
   LCD1602_WriteData(tTempHumi.temp / 1000 + 0x30);  // �¶�ʮλ
   LCD1602_WriteData(tTempHumi.temp % 1000 / 100 + 0x30);    // �¶ȸ�λ
   
   LCD1602_SetXY(0, 4);
   LCD1602_WriteData(tTempHumi.temp % 100 / 10 + 0x30);    // �¶�С����� 1 λ
  // LCD1602_WriteData(tTempHumi.temp % 10 + 0x30);          // �¶�С����� 2 λ
   
   LCD1602_SetXY(0, 13);
   LCD1602_WriteData(tTempHumi.humi % 10000 / 1000 + 0x30);   // ʪ��ʮλ
   LCD1602_WriteData(tTempHumi.humi % 1000 / 100 + 0x30);              // ʪ�ȸ�λ

   if(SensorDisp.hold == SENSOR_STA_TEMPHUMI)
   	  LCD1602_WriteString(1, 15, "H");
   TempHumi_Indicate(tTempHumi.humi % 10000 / 100);
}

// �¶���ʾ��ʼ��
// ��8λΪ�¶�ֵ, ��8λΪʪ��ֵ
static void TempHumi_DisplayInit(uint8_t display_mode)
{
   LCD1602_ClearScreen();
   TempHumi_DisplayValue(display_mode);
   LCD1602_SetXY(0, 3);
   LCD1602_WriteData('.');
   LCD1602_WriteString (0,  6, "'C");
   LCD1602_WriteString (0,  10, "RH ");
   LCD1602_WriteString (0,  15, "%");

   if(SensorDisp.hold == SENSOR_STA_TEMPHUMI)
   	  LCD1602_WriteString(1, 15, "H");

   TempHumi_Indicate(tTempHumi.humi % 10000 / 100);
}

#include "RTCDrv.h"
#include "BatteryLevel.h"
#include "ExtiDrv.h"
#include "board_version.h"


static os_timer_t tTimerChargingDisplay;  // ��ʾ���״̬
static uint8_t bat_icon_step = 0;  
static uint8_t is_enter_time_mode = E_FALSE;  // �Ƿ��ѽ���TIME��ʾģʽ

static void TimerChargingDisplay_CallBack(void * arg)
{
   uint8_t i;
   
   if(is_enter_time_mode)
   {
     bat_icon_step++;
     if(bat_icon_step > 4)bat_icon_step = 1;

      #if 1
      LCD1602_WriteString(1, 9, "    ");
	  LCD1602_SetXY(1, 9);
      for(i = 0; i < bat_icon_step; i++)
      {
         LCD1602_WriteData(0xFF);
      }
	  #endif
	  
    // printf("t = %ld\n", os_get_tick());   
	 os_timer_arm(&tTimerChargingDisplay, 30, 0);
   }
   else
   {
      bat_icon_step = 0;
   }
}

void TIME_StopChargeDisplay(void)
{
    os_timer_disarm(&tTimerChargingDisplay);
	bat_icon_step = 0;
	LCD1602_WriteString(1, 9, "    ");
}

// ���������ʾ��ʱ��
static void TIME_StartChargeDisplayTimer(void)
{
    if(battery_is_charging & USB_PLUGED_MASK)
    {
        if(BatteryIsCharging())  // ���ڳ��
        {
            if(bat_icon_step == 0)  // �����ʾ��ʱ����û����
	        {
	            bat_icon_step = 1;

				LCD1602_SetXY(1, 9);
		        LCD1602_WriteData(0xFF);
		        os_timer_arm(&tTimerChargingDisplay, 30, 0);
	        }
            return;
        }
    }
	TIME_StopChargeDisplay();
}

static void TIME_DisplayValue(uint8_t display_mode)
{
    #if 0
    LCD1602_WriteInt(0, 0, calendar.year % 100);
	LCD1602_WriteInt(0, 3, calendar.month);
	LCD1602_WriteInt(0, 6, calendar.day);
	LCD1602_WriteInt(1, 0, calendar.hour);
	LCD1602_WriteInt(1, 3, calendar.min);
	LCD1602_WriteInt(1, 6, calendar.sec);
    #endif
	
    LCD1602_WriteInteger(0, 12, bat_lev_percent, 3);  // ��ص���
    TIME_StartChargeDisplayTimer();
	if(SensorDisp.hold == SENSOR_STA_TIME)
   	    LCD1602_WriteString(1, 15, "H");
}

// ��ʾʱ��, ��ص���
static void TIME_DisplayInit(uint8_t display_mode)
{
    LCD1602_ClearScreen();
	
	is_enter_time_mode = E_TRUE;
	
	TIME_DisplayValue(display_mode);
    
	#if 0 
	LCD1602_SetXY(0, 2);
    LCD1602_WriteData('-');
	LCD1602_SetXY(0, 5);
    LCD1602_WriteData('-');
	
	LCD1602_SetXY(1, 2);
    LCD1602_WriteData(':');
	LCD1602_SetXY(1, 5);
    LCD1602_WriteData(':');
    #endif
	
    LCD1602_WriteString(0, 9, "BAT");
	LCD1602_WriteString(0, 15, "%");
	
	if(SensorDisp.hold == SENSOR_STA_TIME)
   	  LCD1602_WriteString(1, 15, "H");
	//TIME_StartChargeDisplayTimer();
}

static void TIME_DisplayExit(uint8_t dislay_mode)
{
   is_enter_time_mode = E_FALSE; 
   bat_icon_step = 0;
}


static uint8_t is_enter_tvoc_mode = E_FALSE;  // �Ƿ��ѽ���TVOC��ʾģʽ
static uint8_t is_real_tvoc_val = E_FALSE;    // �Ƿ�Ϊ��ʵ��TVOCֵ
static uint16_t tvoc_left_heat_sec = 180;           // ʣ�µļ���ʱ��

static os_timer_t tTVOCCountDownTimer;
static void TVOCCountDownTimer_CallBack(void * arg)
{
   // ʣ�����ʱ�仹�� �� ��δ�˳�����״̬ ���� TVOC ��ʾģʽ
   os_printf("tvoc CtDwTim_Cb, step 1, left sec = %d: tick = %ld\r\n", tvoc_left_heat_sec, os_get_tick());
   if(tvoc_left_heat_sec)
   {
      os_printf("tvoc CtDwTim_Cb, step 2, is_real_tvoc_val == %d: tick = %ld\r\n",  is_real_tvoc_val, os_get_tick());
      if(! is_real_tvoc_val)
      {
         tvoc_left_heat_sec--;

         os_printf("tvoc CtDwTim_Cb, step 3, is_enter_tvoc_mode = %d: tick = %ld\r\n",  is_enter_tvoc_mode, os_get_tick());
		 if(is_enter_tvoc_mode)
		 {
		    LCD1602_WriteInteger(1, 5, tvoc_left_heat_sec, 3);
			os_printf("tvoc CtDwTim_Cb, step 4: tick = %ld\r\n", os_get_tick());
		 }
		 os_timer_arm(&tTVOCCountDownTimer, 100, 0);  // 1 s 
      }
   } 
}


// ��ʾTVOC
static void TVOC_DisplayValue(uint8_t display_mode)
{
   uint16_t tvoc = tTempHumi.tvoc;
   	
   
   
   if(tvoc == 0xFFFF)  // ˵�����ڼ���
   {
      tvoc = 0;
	  return;
   }
   else if(! is_real_tvoc_val)
   {
      is_real_tvoc_val = E_TRUE;  // TVOC ���������ѽ���
      LCD1602_WriteString(1, 0, "             ");
   }
   tvoc %= 10000;  
   LCD1602_SetXY(0, 6);
   if((tvoc / 1000))
   {
      LCD1602_WriteData(tvoc / 1000 + 0x30);           // TVOCʮλ
   }
   else
   {
      LCD1602_WriteData(' ');
   }
   LCD1602_WriteData(tvoc % 1000 / 100 + 0x30);    // TVOC��λ
   
   LCD1602_SetXY(0, 9);
   LCD1602_WriteData(tvoc % 100 / 10 + 0x30);      // tvocС����� 1 λ

   if(SensorDisp.hold == SENSOR_STA_TVOC)
   	  LCD1602_WriteString(1, 15, "H");
   // ָʾ����ʾ
}



// TVOC��ʾ��ʼ��
static void TVOC_DisplayInit(uint8_t display_mode)
{
   uint16_t tvoc = tTempHumi.tvoc;
   
   LCD1602_ClearScreen();
   is_enter_tvoc_mode = E_TRUE;

   LCD1602_WriteString (0,  0, "TVOC");
   if(tvoc == 0xFFFF)
   {
       tvoc = 0;
	   LCD1602_WriteString(1, 0, "wait");
	   LCD1602_WriteString(1, 9, "s");
   }
   else
   {
       TVOC_DisplayValue(display_mode);
       LCD1602_SetXY(0, 8);
       LCD1602_WriteData('.');
       LCD1602_WriteString (0,  11, "ppm");
   }
   
   if(! is_real_tvoc_val)  // TVOC ���ڼ���״̬
   {
       LCD1602_WriteInteger(1, 5, tvoc_left_heat_sec, 3);
       os_timer_setfn(&tTVOCCountDownTimer, TVOCCountDownTimer_CallBack, NULL);
	   os_timer_arm(&tTVOCCountDownTimer, 100, 0);  // 1 s 
   }
   if(SensorDisp.hold == SENSOR_STA_TVOC)
   	  LCD1602_WriteString(1, 15, "H");

   // ָʾ����ʾ
}



static void TVOC_DisplayExit(uint8_t dislay_mode)
{
   is_enter_tvoc_mode = E_FALSE;
   
}

static void TVOC_NotExisted(uint8_t display_mode)
{
   os_printf("tvoc not existed: tick = %ld\r\n", os_get_tick());
   is_real_tvoc_val = E_FALSE;
   tvoc_left_heat_sec = 180;
}

static void NoSensor_DisplayInit(uint8_t display_mode)
{
    LCD1602_ClearScreen();
	LCD1602_WriteString(0, 0, "No Sensor");
}
static void NoSensor_DisplayValue(uint8_t display_mode)
{

}


// ����У����Ľ��, ����: result: ����ֵ; aver: ƽ��ֵ, ����; cali: ���õĻ�������ֵ, ����
#define HCHO_GetCaliResult(result, aver, cali)  {\
	if(cali){\
		result = ( (aver > cali) ? (aver - cali) : 0); }\
		}\


typedef void (*DisplayFunc_UINT16)(uint16_t val);

// ����ָ������
//DisplayFunc_UINT16 HCHODisplayInit[2] = {HCHO_DisplayMassFractInit, HCHO_DisplayPPMInit};


void HCHO_DisplayInit(uint8_t display_mode)
{
   uint16_t mass = tHchoAna.aver_mass;
   uint16_t aver = tHchoAna.aver_ppb;
   
   switch(display_mode)
   {
      case HCHO_DISPLAY_MASSFRACT:
	  {
		 HCHO_GetCaliResult(mass, mass, tHchoAna.cali_mass);
	  	 HCHO_DisplayMassFractInit(mass);
	  }break;
	  case HCHO_DISPLAY_PPM:
	  {
		 HCHO_GetCaliResult(aver, aver, tHchoAna.cali_ppb);
	  	 HCHO_DisplayPPMInit(aver);
	  }break;
   }
   if(tHchoAna.is_cal && SensorDisp.hold == SENSOR_STA_HCHO)
   	   LCD1602_WriteString(1, 13, "C&H");
   else if(tHchoAna.is_cal)LCD1602_WriteString(1, 13, "CAL");
   //TEMP_DisplayInit();
   if(is_rgb_on)LED_IndicateColorOfHCHO();
   HCHO_Indicate(mass);
}
//static 
void HCHO_DisplayValue(uint8_t display_mode)
{
   uint16_t mass = tHchoAna.aver_mass;
   uint16_t aver = tHchoAna.aver_ppb;
   
   switch(display_mode)
   {
      case HCHO_DISPLAY_MASSFRACT:
	  {
		 HCHO_GetCaliResult(mass, mass, tHchoAna.cali_mass);
	  	 HCHO_DisplayMassFract(mass);
	  }break;
	  case HCHO_DISPLAY_PPM:
	  {
	  	 
		 HCHO_GetCaliResult(aver, aver, tHchoAna.cali_ppb);
	  	 HCHO_DisplayPPM(aver);
	  }break;
   }
    if(tHchoAna.is_cal && SensorDisp.hold == SENSOR_STA_HCHO)
   	   LCD1602_WriteString(1, 13, "C&H");
    else if(tHchoAna.is_cal)LCD1602_WriteString(1, 13, "CAL");
   if(is_rgb_on)LED_IndicateColorOfHCHO();
   HCHO_Indicate(mass);
}

// ���ݼ�ȩֵ��ʾ
// < 0.08 mg/m3 : Good
// 0.08 - 0.2 mg /m3: Unhealthy
// 0.2 - 0.5 mg /m3: Very Unhealthy
// > 0.5 mg/m3: Dangerous
void HCHO_Indicate(uint16_t val)
{
    #if 1
    if(val < 100)
    {
       LCD1602_WriteString(1,  0, "Good       ");
	   //if(is_rgb_on)LED_AqiIndicate(AQI_GOOD);             // �̵�
    }
	else if(val < 500)
	{
	   LCD1602_WriteString(1,  0, "Unhealthy  ");
	   //if(is_rgb_on)LED_AqiIndicate(AQI_Moderate);        // �Ƶ�
	}
	else
	{
	   LCD1602_WriteString(1,  0, "Badly      ");
	  // if(is_rgb_on)LED_AqiIndicate(AQI_VeryUnhealthy);  // ��ʾ���
	}
	#endif
	//else
	//{
	   //LCD1602_WriteString(1,  0, "Dangerous     ");
	   //LED_AqiIndicate(AQI_HeavyInHell);
	//}
}

// ����У���, ȥ��ͷβ 2 ���ֽ�
uint8_t HCHO_CheckSum(uint8_t * buf, uint16_t len)
{
    uint16_t j, newSum = 0;
	if(len < 2)return 0;
	buf += 1;
    len -= 2;
	for(j = 0; j < len; j++)
	{
	   newSum += buf[j];
	}
	newSum = (~newSum) + 1;
	return newSum;
}


// ��ȩУ������
void HCHO_CaliSet(void)
{
    T_SYS_ENV env;

	if(SensorDisp.master_state != SENSOR_STA_HCHO)return;
	os_printf("HCHO CAL\n");
	
    tHchoAna.is_cal ^= 1;
	if(tHchoAna.is_cal)
    {
        if(SensorDisp.hold != SENSOR_STA_HCHO)
           LCD1602_WriteString(1, 13, "CAL");
		else 
		   LCD1602_WriteString(1, 13, "C&H");
		tHchoAna.cali_ppb  = tHchoAna.aver_ppb;  // ��ΪУ��
		tHchoAna.cali_mass = (uint16_t)((double)tHchoAna.cali_ppb * 30 / 22.4); 
	}
    else
    {
        if(SensorDisp.hold)
           LCD1602_WriteString(1, 13, "  H");
		else
		   LCD1602_WriteString(1, 13, "   ");
		tHchoAna.cali_ppb  = 0;
		tHchoAna.cali_mass = 0;
    }
    F10X_FLASH_ReadSysEnv(&env);
	
	env.is_hcho_cal = tHchoAna.is_cal;
	env.cali_mass = tHchoAna.cali_mass;
	env.cali_ppb  = tHchoAna.cali_ppb;
	F10X_FLASH_WriteSysEnv(&env);
}


static os_timer_t tTimerSensorTask;  // �������ܹ���Ķ�ʱ��



typedef void (*SensorDisplayFunc)(uint8_t index);
typedef struct
{
  void (*SensorDisplayInit)(uint8_t display_mode);
  void (*SensorDisplayValue)(uint8_t display_mode);
  void (*SensorDisplayExit)(uint8_t display_mode);  // �˳��������
  void (*SensorNotExisted)(uint8_t display_mode);  // ������������, ���ʼ��������Ϊ��ʼ״̬
  uint8_t max_sub_state;   // ������״ֵ̬
  uint8_t existed;          // �������Ƿ����
  uint8_t backup_sub_state; // ���ݵ���״̬
  uint32_t ttl;              // ����ʱ��
}T_SensorMap;

T_SensorMap SensorMap[SENSOR_STA_END] =
{
    {AppVersionInfo,          AppVersionInfo,          NULL,                NULL,               0},
	#if 1
   	{HCHO_DisplayInit,        HCHO_DisplayValue,      NULL,                NULL,               HCHO_DISPLAY_PPM},

	
    {PM25_Display_Init,       PM25_Display_Value,     NULL,                NULL,               DISPLAY_AQI_CN },
    #endif
    {TempHumi_DisplayInit,    TempHumi_DisplayValue, NULL,                NULL,               0},
    
	
    {TVOC_DisplayInit,        TVOC_DisplayValue,      TVOC_DisplayExit,  TVOC_NotExisted,  0},
    {TIME_DisplayInit,        TIME_DisplayValue,      TIME_DisplayExit,  NULL,               0},
    {NoSensor_DisplayInit,   NoSensor_DisplayValue,  NULL,                 NULL,              0},
};

void  TempHumi_SetSensorExisted(uint8_t state)
{
   SensorMap[SENSOR_STA_TEMPHUMI].existed = state;
   SensorMap[SENSOR_STA_TEMPHUMI].ttl      = OS_SetTimeout(SEC(MAX_MASTER_STATE_SEC));
}

void TVOC_SetSensorExisted(uint8_t state)
{
   SensorMap[SENSOR_STA_TVOC].existed = state;
   SensorMap[SENSOR_STA_TVOC].ttl      = OS_SetTimeout(SEC(MAX_MASTER_STATE_SEC));
}

void TIME_SetSensorExisted(uint8_t state)
{
   SensorMap[SENSOR_STA_TIME].existed = state;
   SensorMap[SENSOR_STA_TIME].ttl      = OS_SetTimeout(SEC(MAX_MASTER_STATE_SEC));
}

// һֱ���ָô�������ʾ��״̬
void Sensor_HoldDisplay(void)
{
    T_SYS_ENV env;
	
    if(SensorDisp.hold == 0 || SensorDisp.hold != SensorDisp.master_state)
    {

        SensorDisp.hold = SensorDisp.master_state;  // ����Ϊ��ʾ����

		if(SensorDisp.master_state == SENSOR_STA_HCHO)
		{
		    if(tHchoAna.is_cal)
			    LCD1602_WriteString(1, 13, "C&H");	
			else
                LCD1602_WriteString(1, 13, "  H"); 			
		}
		else
		{
		    LCD1602_WriteString(1, 15, "H");
		}
    }
	else if(SensorDisp.hold == SensorDisp.master_state)
	{
	    SensorDisp.hold = 0;  // ȡ����ʾ����
	    
		if(SensorDisp.master_state == SENSOR_STA_HCHO)
		{
		    if(tHchoAna.is_cal)
			    LCD1602_WriteString(1, 13, "CAL");	
			else
                LCD1602_WriteString(1, 13, "   "); 			
		}
		else
		{
		    LCD1602_WriteString(1, 15, " ");
		}
	}
	F10X_FLASH_ReadSysEnv(&env);
	env.hold = SensorDisp.hold;
	os_printf("save hold = %d\n", env.hold);
	F10X_FLASH_WriteSysEnv(&env);
}

// ������ʾ״̬����
void Sensor_KeepDisplayState(void)
{
   SensorDisp.master_sec = HOLD_STATE_SEC;
}

// ��ʾ������״̬
// ����: uint8_t sub_state: ��״̬��ʾ
static void Sensor_DisplayState(uint8_t master_state)
{
   if(master_state != SensorDisp.master_state)
   {
        SensorMap[SensorDisp.master_state].backup_sub_state = SensorDisp.sub_state;  // ������״̬
		SensorDisp.last_master_state = SensorDisp.master_state;  // ������״̬
		SensorDisp.master_state = master_state;  // �����µ���״̬
		SensorDisp.sub_state = SensorMap[SensorDisp.master_state].backup_sub_state; // ȡ���µ���״̬
		SensorMap[SensorDisp.master_state].SensorDisplayInit(SensorDisp.sub_state);  
   }
   else
   {
	   SensorMap[SensorDisp.master_state].SensorDisplayValue(SensorDisp.sub_state);
   }
}

// NEXT ������ 1, ͬʱ���Ӹ���ģʽ�µ���ʾʱ�䳤��
void Sensor_SubStateToNext(void)
{
  
   
   if(SensorDisp.master_sec < HOLD_STATE_SEC)  // ���� 5s 
   {
       SensorDisp.master_sec = HOLD_STATE_SEC;
   }
   SensorDisp.sub_state++;
   if(SensorDisp.sub_state > SensorMap[SensorDisp.master_state].max_sub_state)
   {
       SensorDisp.sub_state = 0;	   
   }
   os_printf("sub sta = %d, master sta = %d\n", SensorDisp.sub_state, SensorDisp.master_state);
   SensorMap[SensorDisp.master_state].SensorDisplayInit(SensorDisp.sub_state);  
}

static void TimerSensorTask_CallBack(void * arg)
{
   uint8_t i, j, new_state = SensorDisp.master_state;

  

   if(SensorDisp.master_sec)
   {
       if(! SensorMap[SensorDisp.master_state].existed)
	   {
	       SensorDisp.master_sec = 0;
		   SensorDisp.hold = 0;
       }
       else
	   {
	       if(SensorDisp.hold  && SensorDisp.hold == SensorDisp.master_state)
	       {
	           SensorDisp.master_sec = HOLD_STATE_SEC;  // һֱ������ʾ  
	       }
	   	   else 
		   	   SensorDisp.master_sec--;
		   Sensor_DisplayState(SensorDisp.master_state);
	   }
   }
   os_printf("cur sensor %d, left_sec = %d\n", SensorDisp.master_state, SensorDisp.master_sec);

   if(! SensorDisp.master_sec)  // ��ʾʱ����Ϊ0, ת����һ����������ʾģʽ
   {
       // ִ���������
       if(SensorMap[SensorDisp.master_state].SensorDisplayExit)
       {
          SensorMap[SensorDisp.master_state].SensorDisplayExit(SensorDisp.sub_state); 
       }
	   
	   for(i = 1; i < SENSOR_STA_NONE; i++)
	   {
	      if(SensorMap[i].existed)
	      {
	         break;
	      }
	   }
	  
	   if(i == SENSOR_STA_NONE)  // �޴�����
	   {
	       if(SensorDisp.master_state != i)
	       {
	           Sensor_DisplayState(i);
	       }
	   }
	   else  // �д�����
	   {
	      for(j = SensorDisp.master_state + 1; j < SENSOR_STA_NONE; j++)
	      {
	         if(SensorMap[j].existed)
	         {
	           break;
	         }
	      }
		  if(j == SENSOR_STA_NONE)
		  {
		     new_state = 1;
		  }
		  else 
		  	 new_state++;
		  if(new_state >= SENSOR_STA_NONE)
		  {
		      new_state = 1;
		  }
		  for(; new_state < (uint8_t)SENSOR_STA_NONE; new_state++)
		  {
		     if(! SensorMap[new_state].existed)  // ������
		     {
		        continue;
		     }
			 else  // ��ʾ�ô�����������
			 {
			      os_printf("1: old sta = %d, new sta = %d, tick = %ld\n", SensorDisp.master_state, 
		 			new_state, os_get_tick());
				  SensorDisp.master_sec = MAX_MASTER_STATE_SEC;
				  Sensor_DisplayState(new_state);
				  break;
			 }
		  }  
	   }
   }  
  
   
    // ���յ�һ������֮��10s, ���û���ٽ��յ���2������, ����Ϊ�ô������Ѳ�����
   for(i = 1; i < (uint8_t)SENSOR_STA_NONE; i++)
   {
      if(SensorMap[i].ttl && OS_IsTimeout(SensorMap[i].ttl))
      {
          SensorMap[i].ttl = 0;
		  SensorMap[i].existed = E_FALSE;
		  
		  // ִ�д������˳� ��Ĭ�ϳ�ʼ�� ����
          if(SensorMap[SensorDisp.master_state].SensorNotExisted)
          {
             SensorMap[SensorDisp.master_state].SensorNotExisted(SensorDisp.sub_state); 
          }
		  os_printf("sensor %d not exist tick = %ld\n", i, os_get_tick());
      }
   }
   
   os_timer_arm(&tTimerSensorTask, 100, 0);  // 1 s ��ʱ
}


	



void LED_IndicateColorOfHCHO(void)
{
#if 0
   E_AQI_LEVEL level;
   uint16_t mass = tHchoAna.aver_mass;
   
   HCHO_GetCaliResult(mass, tHchoAna.aver_mass, tHchoAna.cali_mass)
   
   if(tHchoAna.aver_mass < 80)
   {
      level = AQI_GOOD;
   }
   else if(tHchoAna.aver_mass < 200)
   {
      level = AQI_LightUnhealthy;
   }
   else if(tHchoAna.aver_mass < 500)
   {
      level = AQI_VeryUnhealthy;
   }
   else
   {
      level = AQI_HeavyInHell;
   }
   LED_AqiIndicate(level);
#endif
}

#if 1
static os_timer_t tTimerDelayDispVersion;  // ��ʱ��ʾ�汾��
static void TimerDelayDispVersion(void * arg)
{
    AppVersionInfo(0);
    os_timer_arm(&tTimerSensorTask, 600, 0);  // 8 s��
}
#endif

void PM25_Init(void)
{
   T_SYS_ENV env;
   
   #if (SENSOR_SELECT == HCHO_SENSOR)
   //HCHO_SwitchToActiveMode();
   //HCHO_SwitchToActiveMode();
   #endif

   os_memset(&tHchoAna, 0, sizeof(tHchoAna));
   os_memset(&SensorDisp, 0, sizeof(SensorDisp));

   
   tHchoAna.is_power_first = E_TRUE;
   
   if(F10X_FLASH_ReadSysEnv(&env) == FLASH_SUCCESS)
   {
      os_printf("read SYS ENV OK: cal = %d, hold = %d\n", env.is_hcho_cal, env.hold);
      tHchoAna.is_cal    = env.is_hcho_cal;
	  tHchoAna.cali_mass = env.cali_mass;
	  tHchoAna.cali_ppb  = env.cali_ppb;

	  SensorDisp.hold  = env.hold;  
   }
   else
   {
      os_printf("read SYS ENV err\n");
      os_memset(&env, 0, sizeof(env));
	  F10X_FLASH_WriteSysEnv(&env);
	  SensorDisp.hold = 0;
   }
   os_timer_setfn(&tTimerChargingDisplay, TimerChargingDisplay_CallBack, NULL);
   os_timer_setfn(&tTimerCalculator, TimerCalculator_CallBack, NULL);  // ��ʱ����ʼ��, ���ûص�����
   os_timer_setfn(&tTimerPM25Store,  TimerPM25Store_CallBack, NULL);
   
   
   os_timer_setfn(&tTimerSensorTask, TimerSensorTask_CallBack, NULL);
   os_timer_setfn(&tTimerDelayDispVersion, TimerDelayDispVersion, NULL);
   
   SensorDisp.master_state = SENSOR_STA_NONE;
   SensorDisp.master_sec   = MAX_MASTER_STATE_SEC;

   os_timer_arm(&tTimerDelayDispVersion, 150, 0);
   //Sensor_DisplayState(SensorDisp.hold);
   //os_timer_arm(&tTimerSensorTask, 800, 0);  // 8 s��
}

	
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ��� 
void PM25_UART_IRQHandler(void)	// PM25�жϷ������
{
   uint8_t data = 0;

   #if 0
   if(USART_GetITStatus(PM25_UART, USART_IT_RXNE) != RESET)  // ���շǿձ�־��1
   #else
   if(READ_REG_32_BIT(PM25_UART->SR, USART_SR_RXNE))
   #endif
   {
        //data = USART_ReceiveData(PM25_UART);
	    data = (uint16_t)(PM25_UART->DR & (uint16_t)0x01FF);
	    
        if(PM25Rx.last_val == 0x42 && data == 0x4d)  // PM25 Sensor ����ʼͷ
		{
			PM25Rx.rx_count = 0;
			PM25Rx.rx_buf[PM25Rx.rx_count++] = 0x42;
		}
		else if(PM25Rx.rx_count >= sizeof(PM25Rx.rx_buf))
		{
			PM25Rx.rx_count = 0;
			// ����������Խ�ֹ�����ж�
		}
		PM25Rx.rx_buf[PM25Rx.rx_count++] = data;
		PM25Rx.last_val = data;
		if(PM25Rx.rx_count > 3)
		{
		   uint16_t len;
		   
		   len = (((uint16_t)PM25Rx.rx_buf[2]) << 8) + PM25Rx.rx_buf[3];
		   if((len + 4) == PM25Rx.rx_count)
		   {
			 //DisableRxPM25Sensor();  // ��ͣ�����ж�
			 SensorMap[SENSOR_STA_PM25].existed = E_TRUE;  // �����������ӵ�����
			 SensorMap[SENSOR_STA_PM25].ttl      = OS_SetTimeout(SEC(3));
			 PM25_Receive(PM25Rx.rx_buf, len);
			 PM25Rx.rx_count = 0;
		   }
	   }
   }
}

//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ��� 
void HCHO_UART_IRQHandler(void)	// HCHO�жϷ������
{
   uint8_t data = 0;

   #if 0
   if(USART_GetITStatus(HCHO_UART, USART_IT_RXNE) != RESET)  // ���շǿձ�־��1
   #else
   if(READ_REG_32_BIT(HCHO_UART->SR, USART_SR_RXNE))
   #endif
   {
        //data = USART_ReceiveData(HCHO_UART);
		data = (uint16_t)(HCHO_UART->DR & (uint16_t)0x01FF);
		
        if(HchoRx.last_val == 0xFF && (data == 0x17 || data == 0x86) )  // hcho Sensor ����ʼͷ
		{
			HchoRx.rx_count = 0;
			HchoRx.rx_buf[HchoRx.rx_count++] = 0xFF;
		}
		else if(HchoRx.rx_count >= sizeof(HchoRx.rx_buf))
		{
			HchoRx.rx_count = 0;
			// ����������Խ�ֹ�����ж�
		}
		HchoRx.rx_buf[HchoRx.rx_count++] = data;
		HchoRx.last_val = data;
		if(HchoRx.rx_count == 9)  // һ�� 9 ���ֽ�
		{
		     if(HCHO_CheckSum(HchoRx.rx_buf, 9) == HchoRx.rx_buf[8])  // У�����ȷ
		     {
		        SensorMap[SENSOR_STA_HCHO].existed = E_TRUE;  // �������ѽӵ�����
		        SensorMap[SENSOR_STA_HCHO].ttl     =  OS_SetTimeout(SEC(3));
		        HCHO_Receive(HchoRx.rx_buf, 9);
		     }
			 HchoRx.rx_count = 0;
	    }
   }
}


