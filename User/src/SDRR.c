

/******************** Sensor Data Record  Repository  ���������ݼ�¼�����ļ�   *********************** */

#include "SDRR.h"
#include "PM25Sensor.h"
#include "os_global.h"
#include "GlobalDef.h"
#include "FatFs_Demo.h"
#include <string.h>


extern uint8_t record_flag;
uint16_t record_gap = 60;  // ��¼���, Ĭ��ÿ�� 60 ���¼һ�δ���������

// ������������ʱ��Ҳ����buf


static uint16_t cur_str_len    = 0;   // ��ǰ�ַ���ռ�õĳ���
//static uint16_t cur_item_count = 0;   // ��������ʱbuf�еĴ�������Ŀ������
static uint32_t total_item_count = 0; // �ܵĴ�������Ŀ��

/**************************
ÿ�����ݵĳ���<= 70 B, ��ʽΪ:
                 ���      ʱ���                    ��ȩ(mg/m3)        PM2.5(ug/m3)           �¶�('C)             ���ʪ��(%)
��0��:    No          TimeStamp                   HCHO(mg/m3)        PM2.5(ug/m3)           Temperature('C)    Humidity
��1��    [00000]    2017-06-01 13:13:59   9.001                        999                              25.7                          30%
��N��    [N-1   ]    2017-06-01 14:15:03   0.072                        008                              24.5                          47%

����ÿ 10 ���� ��һ����¼, ÿ���¼ 24 * 6 = 144 ��
 2MB FLASH (W25X16 )����Լ�¼ :  1768 KB * 1024 /  (24 * 6 * 48 ) = 261 ��
******************************/
//extern FATFS FlashDiskFatFs;

static T_SDRR tSDRR;



// ���������ݼ�¼ģ���ʼ��
void SDRR_Init(void)
{
   
}

// ������������ת��Ϊ�ַ���
// ����: T_SDRR : ���������ݼ�¼�ṹ����
//       uint8_t * buf: ����ַ���
//       uint16_t * size: ������ʱ��buf����󳤶�, ���ʱ���ַ����ĳ���
//  һ��ռ�� 54 �ֽ�
//  "0-65535 Y-Mon-Day  HH:MM:SS  mg/m3  ug/m3  'C"
//  [00005]  17-05-01 23:59:51  0.063    031     -8.21  49%
// "[00005]  17-05-01 23:59:51  0.063    031     23.21  49%"
E_RESULT SDRR_SensorPointToString(T_SDRR *sdrr, uint8_t * buf, uint16_t * size)
{
   #if 1
   //uint8_t mask = (1 << SENSOR_END) - 1;
   uint16_t left_size = *size;   // �����ó���
   uint16_t len = 0;
   
   if(NULL == sdrr || NULL == buf || NULL == size){ INSERT_ERROR_INFO(0);  return APP_FAILED; }

   // ������д������������Ƿ񱣴�
   #if 0 // ��ʱ����
   if(sdrr->sensor_mask < mask)
   {
      os_printf("sensor pointer not save all, sensor_mask = 0x%x\n", sdrr->sensor_mask);
	  return APP_FAILED;
   }
   #endif

   // ��Ŀ ���� ʱ��
   #if 1
   len += os_snprintf(&buf[len], left_size, "[%05d]   %s  %02d-%02d-%02d %02d:%02d:%02d  ", total_item_count, (record_flag ? "tag" : "   "),
                       sdrr->time.year, sdrr->time.month, sdrr->time.day, 
                       sdrr->time.hour, sdrr->time.min,   sdrr->time.sec);
   #else
   len += os_snprintf(&buf[len], left_size, "[%05d]  %ld s  ", total_item_count, os_get_tick() / 100 );
   #endif
   
   left_size = *size - len;
   len += os_snprintf(&buf[len], left_size, "%c.%03d  %3d    %c%2d.%02d  %2d%%\r\n", 
   	                    (sdrr->hcho % 10000 / 1000) + 0x30, sdrr->hcho % 1000, // ��ȩ
   	                    sdrr->pm2p5 % 1000, // PM2.5
   	                    ((sdrr->temp / 10000) ? '-' : ' '), sdrr->temp % 10000 / 100, (sdrr->temp % 100), // �¶�
   	                    sdrr->RH % 100);  // ʪ��

  // ���Դ�ӡ
  do
  {
     uint8_t char_buf[128];

	 os_memset(char_buf, 0, sizeof(char_buf));
	 os_strncpy(char_buf, buf, len);
	 os_printf("%s\r\n", char_buf);
  }while(0);
   
   
   total_item_count += 1;
   *size = len;  // �����ʽ������ַ�������

   #endif
   
   return APP_SUCCESS;
}

static uint8_t  sector_buf[256];     // ��һ��FLASH������������СΪ����, ���ٶ�FLASH�Ĳ�д����

/***********************************
����: ��ָ���ļ���д�봫������¼
����: char * file_name: �ļ���
************************************/
E_RESULT SDRR_WriteRecordToFile(char * file_name)
{
    uint16_t item_len = 0; // ��������ת��Ϊ�ַ�����ĳ���
	uint16_t left_len = 0;
	E_RESULT res = APP_FAILED;
	
	item_len = sizeof(sector_buf) - cur_str_len;  // ������ʣ����õĳ��� 
	res = SDRR_SensorPointToString(&tSDRR, &sector_buf[cur_str_len], &item_len);
    //os_memset(&tSDRR, 0, sizeof(tSDRR));  // �����¼�Ĵ���������
	
	cur_str_len += item_len;
	left_len = sizeof(sector_buf) - cur_str_len;

	#if 1
	if(left_len < item_len) // ʣ�µĲ���һ�����ݳ���
	{
	   if(left_len > 2)
	   {
	      os_memset(&sector_buf[cur_str_len], ' ', left_len - 2);
		  os_strncpy(&sector_buf[sizeof(sector_buf) - 2], "\r\n", 2);
	   }
	   else if(left_len == 2)
	   {
	      os_strncpy(&sector_buf[sizeof(sector_buf) - 2], "\r\n", 2);
	   }
	   
	  //  �����ݱ��浽FAT �ļ���
	   res = FILE_Write(file_name, (char *)sector_buf);
	   os_memset(sector_buf, 0, sizeof(sector_buf));
	   cur_str_len  = 0;
	}
	#else
    res = FILE_Write(file_name, (char *)sector_buf);
	   os_memset(sector_buf, 0, sizeof(sector_buf));
	   cur_str_len  = 0;
	#endif
	
	return res;
}
	

/***************************
����: ����һ��������������
����: E_SensorType type: ���������ݵ�����
             void  * data: ����������
 ����ֵ: ����ɹ�: APP_SUCCESS; ʧ��: APP_FAILED            
******************************/
#include "RTCDrv.h"

E_RESULT SDRR_SaveSensorPoint(E_SensorType type, void  * data)
{
	static uint32_t save_time_out = 0;
	
	switch(type)
	{
	    case SENSOR_HCHO:
	   	{
			tSDRR.hcho = *((uint16_t *)data);
		}break;
		case SENSOR_PM25:
		{
		   tSDRR.pm2p5 = *((uint16_t *)data);
		}break;
		case SENSOR_TEMP:
		{
		   tSDRR.temp = *((uint16_t *)data);
		}break;
		case SENSOR_HUMI:
		{
			tSDRR.RH  = *((uint16_t *)data);
		}break;
		case SENSOR_TIME:
		{
			os_memcpy(&tSDRR.time, data, sizeof(T_RTC_TIME));
		}break;
		default:
		  break;
	}

    if(type < ((uint8_t)SENSOR_END) )
    {
       tSDRR.sensor_mask |= (1 << type);
    }
	
    tSDRR.time.year  = calendar.year % 2000;
	tSDRR.time.month = calendar.month;
	tSDRR.time.day   = calendar.day;
	tSDRR.time.hour  = calendar.hour;
	tSDRR.time.min   = calendar.min;
	tSDRR.time.sec   = calendar.sec;
	tSDRR.sensor_mask |= 1 << SENSOR_TIME;
	tSDRR.sensor_mask |= 1 << SENSOR_HUMI;
	tSDRR.sensor_mask |= 1 << SENSOR_TEMP;
   // if(tSDRR.sensor_mask == ( (1 << (SENSOR_END)) - 1))  // ���������ڵ���ȫ������
	{
	   if(OS_IsTimeout(save_time_out))
	   {		   
	       //os_memset(&tSDRR, 0, sizeof(tSDRR));
	       save_time_out = OS_SetTimeout(SEC(record_gap));  // Ĭ�� 60 sec ����һ�μ�¼
	       os_printf("ready to wirte data point to file, tick = %ld\n", OS_GetSysTick());
	       SDRR_WriteRecordToFile(SENSOR_TXT_FILE_NAME);
	   }
	}

	return APP_SUCCESS;
}





