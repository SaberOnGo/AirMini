

#include "gizwits_port.h"
#include "gizwits_protocol.h"
#include "gizwits_product.h"
#include "os_global.h"
#include "board_version.h"
#include "gizwits_uart.h"
#include "uart_drv.h"
#include "pm25sensor.h"
#include "sw_uart.h"


static  char product_key[64];
static char product_secret[64];

char * PRODUCT_KEY_STRING(void)
{
      return product_key;
}
char *PRODUCT_SECRET_STRING(void)
{
     return product_secret;
}

/**
* Data point initialization function

* In the function to complete the initial user-related data
* @param none
* @return none
* @note The developer can add a data point state initialization value within this function
*/
void userInit(void)
{
    os_memset((uint8_t*)&currentDataPoint, 0, sizeof(dataPoint_t));
    
    /** Warning !!! DataPoint Variables Init , Must Within The Data Range **/ 
    /*
      currentDataPoint.valueRelay_Ctrl = ;
      currentDataPoint.valueLED0_Ctrl = ;
      currentDataPoint.valueCO2 = ;
      currentDataPoint.valuetemp = ;
      currentDataPoint.valuehcho_ppb = ;
      currentDataPoint.valuehumi = ;
      currentDataPoint.valuetvoc_ppb = ;
      currentDataPoint.valuepm10_ug = ;
      currentDataPoint.valuepm2p5_ug = ;
    */

       // currentDataPoint.valueRelay_Ctrl = LED1_Read()  ? false : true;
        //currentDataPoint.valueLED0_Ctrl  = LED0_Read() ? false : true;
}




/**
* User data acquisition

* Here users need to achieve in addition to data points other than the collection of data collection, can be self-defined acquisition frequency and design data filtering algorithm

* @param none
* @return none
*/
void userHandle(void)
{

    float tmp = 0;
    uint16_t val = 0;
    
    val  = tTempHumi.temp;
    tmp = (val & 0x7FFF) / 10;
    if(val & 0x8000)  // 负值
    {
         tmp = -tmp;
    }
    
   // currentDataPoint.valueCO2 = ;//Add Sensor Data Collection
    currentDataPoint.valuetemp = tmp;  //Add Sensor Data Collection

    
    currentDataPoint.valuehcho_ppb = g_hcho_mass;//Add Sensor Data Collection

    
    currentDataPoint.valuehumi = tTempHumi.humi / 100;//Add Sensor Data Collection

    val = tTempHumi.tvoc;
    val *= 100;
    currentDataPoint.valuetvoc_ppb = val;  //  Add Sensor Data Collection
    
    currentDataPoint.valuepm10_ug = g_pm10_ug;//Add Sensor Data Collection
    currentDataPoint.valuepm2p5_ug = g_pm2p5_ug;//Add Sensor Data Collection

    
         

}

/**
* @brief mcuRestart

* MCU Reset function

* @param none
* @return none
*/
void mcuRestart(void)
{
      GIZWITS_LOG("gizwits module ready to reboot mcu ! \r\n");
      // close all interrupt
      GLOBAL_DISABLE_IRQ();
     // __set_FAULTMASK(1);

      NVIC_SystemReset();
}

//#if GIZWITS_TYPE == GIZ_MCU
#include "os_timer.h"
//static 
os_timer_t tTimerWiFiRoutine;

static uint8_t wifi_connect_sta = 0xFF;  // WIFI 连接状态: 0: 已连接;  1: 未连接; 2: No Resp
static uint8_t recv_wifi_ip = 0;         // 是否接收到WIFI IP:  1: 接受到
static uint16_t recv_timeout_cnt = 0;   // 接收超时计数，  超过 40 sec, WIFI 无响应, 则任认为断网

//static 
void TimerWiFiRoutine_CallBack(void * arg)
{
      static uint8_t cnt = 0;
      
      cnt++;
      if(cnt > 10)
      {
               cnt = 0;
                recv_timeout_cnt++;
               if( recv_timeout_cnt % 20 == 0)  // 每 20 sec 查询一次
               {
                      gizwitsGetModuleInfo();
               }
               if(recv_timeout_cnt > 40)
               {
                        //WIFI_TextDisplay(0, "WIFI Disconnect");
                        //WIFI_TextDisplay(1, "WiFi No Resp");
                        recv_timeout_cnt = 0;
               }
               if(recv_wifi_ip == 0xFF)
              {
                      gizwitsGetModuleInfo();
              }
      }
}
//#endif

// gizwits init
void gizwits_user_init(void)
{
#if GIZWITS_TYPE == GIZ_MCU
       //WIFI_USART_Init(FREQ_24MHz, 9600);          //  usart for mcu <-> wifi
       SWUART1_Init();
       userInit();
       gizwitsInit();
       os_timer_setfn(&tTimerWiFiRoutine,   TimerWiFiRoutine_CallBack,  NULL);
       os_timer_arm(&tTimerWiFiRoutine,   10,  1);
#elif GIZWITS_TYPE == GIZ_SOC
       WIFI_USART_Init(115200);
       gizwits_uart_init();
#endif
}

// 主循环任务
void gizwits_user_task(void)
{
#if GIZWITS_TYPE == GIZ_MCU
      userHandle();
      gizwitsHandle((dataPoint_t *)&currentDataPoint);
#elif GIZWITS_TYPE == GIZ_SOC

#endif
}

// cloud command mcu to do some process to ctrl
void gizwits_user_event_process( dataPoint_t * dataPointPtr,  uint8_t event)
{
       if(dataPointPtr == NULL)
       {
                GIZWITS_LOG("dataPointPtr is null \r\n");
                return;
       }
       
       switch(event)
       {
                case EVENT_Relay_Ctrl:
                        currentDataPoint.valueRelay_Ctrl = dataPointPtr->valueRelay_Ctrl;
                        GIZWITS_LOG("Evt: EVENT_Relay_Ctrl %d \n", currentDataPoint.valueRelay_Ctrl);
                        if(0x01 == currentDataPoint.valueRelay_Ctrl)
                        {
                                  //user handle
                                  GIZWITS_LOG("cloud set Relay_Ctrl  to 0\r\n");
                                  //LED1(0);
                                  
                        }
                        else
                        {
                                  GIZWITS_LOG("cloud set Relay_Ctrl  to 1\r\n");
                                   //user handle    
                                   //LED1(1);
                        }
                        //GIZWITS_LOG("LED1 state = %d\r\n", LED1_Read());
                break;
                case EVENT_LED0_Ctrl:
                        currentDataPoint.valueLED0_Ctrl = dataPointPtr->valueLED0_Ctrl;
                        GIZWITS_LOG("Evt: EVENT_LED0_Ctrl %d \n", currentDataPoint.valueLED0_Ctrl);
                        if(0x01 == currentDataPoint.valueLED0_Ctrl)
                        {
                                   GIZWITS_LOG("cloud set LED0_Ctrl  to 0\r\n");
                                   //user handle
                                  // LED0(0);
                        }
                        else
                        {
                                  GIZWITS_LOG("cloud set LED1_Ctrl  to 0\r\n");
                                   //user handle    
                                   //LED0(1);
                        }
                        //GIZWITS_LOG("LED0 state = %d\r\n", LED0_Read());
                break;
        }
}



#if 0
#include "key_drv.h"
void key0_hook(uint8_t key_state)
{
     switch(key_state)
	{
	        case S_key:  
		{
			KEY_DEBUG("S\n");
			KEY_DEBUG("enter WiFi AirLink connect mode \r\n");
#if GIZWITS_TYPE == GIZ_MCU			
			gizwitsSetMode(WIFI_AIRLINK_MODE);
#elif GIZWITS_TYPE == GIZ_SOC
                     gizwits_set_wifi_mode(WIFI_AIRLINK_MODE);
#endif			
		}break;
		case D_key:
		{
			KEY_DEBUG("D\n");
			KEY_DEBUG("wifi reset, please reconfig wifi account and pawd\r\n");
#if GIZWITS_TYPE == GIZ_MCU	
			gizwitsSetMode(WIFI_RESET_MODE);
#elif GIZWITS_TYPE == GIZ_SOC
                     gizwits_set_wifi_mode(WIFI_RESET_MODE);
#endif
		}break;
		case L_key:
		{
			KEY_DEBUG("L\n");
		}break;
	}
}

void key1_hook(uint8_t key_state)
{
     switch(key_state)
	{
	        case S_key:  
		{
			KEY_DEBUG("S\n");
			//KEY_DEBUG("enter WiFi SoftAP connect mode \r\n");
#if GIZWITS_TYPE  == GIZ_MCU
			gizwitsSetMode(WIFI_SOFTAP_MODE);
#elif GIZWITS_TYPE == GIZ_SOC
                     gizwits_set_wifi_mode(WIFI_SOFTAP_MODE);
#endif
		}break;
		case D_key:
		{
		      KEY_DEBUG("D\n");
		      //KEY_DEBUG("enter WiFi AirKiss connect mode \r\n");
#if GIZWITS_TYPE  == GIZ_MCU
			
#elif GIZWITS_TYPE == GIZ_SOC
                     gizwits_set_wifi_mode(WIFI_AIRKISS_MODE);
#endif
		}break;
		case L_key:
		{
			KEY_DEBUG("L\n");
		}break;
	}
}
#endif

// 设置网络连接状态
void  WIFI_SetConnectStatus(uint8_t conn_sta)
{
      wifi_connect_sta = conn_sta;
}


// 获取网络连接状态
uint8_t WIFI_GetConnectStatus(void)
{
       return wifi_connect_sta;
}

// 设置接收到IP 状态
void WIFI_SetIPRecvStatus(uint8_t recv_ip)
{
     recv_wifi_ip = recv_ip;
     recv_timeout_cnt = 0;
}



