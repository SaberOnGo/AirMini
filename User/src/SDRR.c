

/******************** Sensor Data Record  Repository  传感器数据记录处理文件   *********************** */

#include "SDRR.h"
#include "PM25Sensor.h"
#include "os_global.h"
#include "GlobalDef.h"
#include "FatFs_Demo.h"
#include <string.h>


extern uint8_t record_flag;
uint16_t record_gap = 60;  // 记录间隔, 默认每隔 60 秒记录一次传感器数据

// 此数组升级的时候也共用buf


static uint16_t cur_str_len    = 0;   // 当前字符串占用的长度
//static uint16_t cur_item_count = 0;   // 保存在临时buf中的传感器条目的总数
static uint32_t total_item_count = 0; // 总的传感器条目数

/**************************
每条数据的长度<= 70 B, 格式为:
                 编号      时间戳                    甲醛(mg/m3)        PM2.5(ug/m3)           温度('C)             相对湿度(%)
第0行:    No          TimeStamp                   HCHO(mg/m3)        PM2.5(ug/m3)           Temperature('C)    Humidity
第1行    [00000]    2017-06-01 13:13:59   9.001                        999                              25.7                          30%
第N行    [N-1   ]    2017-06-01 14:15:03   0.072                        008                              24.5                          47%

其中每 10 分钟 存一条记录, 每天记录 24 * 6 = 144 条
 2MB FLASH (W25X16 )多可以记录 :  1768 KB * 1024 /  (24 * 6 * 48 ) = 261 天
******************************/
//extern FATFS FlashDiskFatFs;

static T_SDRR tSDRR;



// 传感器数据记录模块初始化
void SDRR_Init(void)
{
   
}

// 将传感器数据转化为字符串
// 参数: T_SDRR : 传感器数据记录结构变量
//       uint8_t * buf: 输出字符串
//       uint16_t * size: 作输入时是buf的最大长度, 输出时是字符串的长度
//  一条占用 54 字节
//  "0-65535 Y-Mon-Day  HH:MM:SS  mg/m3  ug/m3  'C"
//  [00005]  17-05-01 23:59:51  0.063    031     -8.21  49%
// "[00005]  17-05-01 23:59:51  0.063    031     23.21  49%"
E_RESULT SDRR_SensorPointToString(T_SDRR *sdrr, uint8_t * buf, uint16_t * size)
{
   #if 1
   //uint8_t mask = (1 << SENSOR_END) - 1;
   uint16_t left_size = *size;   // 最大可用长度
   uint16_t len = 0;
   
   if(NULL == sdrr || NULL == buf || NULL == size){ INSERT_ERROR_INFO(0);  return APP_FAILED; }

   // 检查所有传感器的数据是否保存
   #if 0 // 暂时屏蔽
   if(sdrr->sensor_mask < mask)
   {
      os_printf("sensor pointer not save all, sensor_mask = 0x%x\n", sdrr->sensor_mask);
	  return APP_FAILED;
   }
   #endif

   // 条目 日期 时间
   #if 1
   len += os_snprintf(&buf[len], left_size, "[%05d]   %s  %02d-%02d-%02d %02d:%02d:%02d  ", total_item_count, (record_flag ? "tag" : "   "),
                       sdrr->time.year, sdrr->time.month, sdrr->time.day, 
                       sdrr->time.hour, sdrr->time.min,   sdrr->time.sec);
   #else
   len += os_snprintf(&buf[len], left_size, "[%05d]  %ld s  ", total_item_count, os_get_tick() / 100 );
   #endif
   
   left_size = *size - len;
   len += os_snprintf(&buf[len], left_size, "%c.%03d  %3d    %c%2d.%02d  %2d%%\r\n", 
   	                    (sdrr->hcho % 10000 / 1000) + 0x30, sdrr->hcho % 1000, // 甲醛
   	                    sdrr->pm2p5 % 1000, // PM2.5
   	                    ((sdrr->temp / 10000) ? '-' : ' '), sdrr->temp % 10000 / 100, (sdrr->temp % 100), // 温度
   	                    sdrr->RH % 100);  // 湿度

  // 调试打印
  do
  {
     uint8_t char_buf[128];

	 os_memset(char_buf, 0, sizeof(char_buf));
	 os_strncpy(char_buf, buf, len);
	 os_printf("%s\r\n", char_buf);
  }while(0);
   
   
   total_item_count += 1;
   *size = len;  // 输出格式化后的字符串长度

   #endif
   
   return APP_SUCCESS;
}

static uint8_t  sector_buf[256];     // 以一个FLASH的物理扇区大小为容量, 减少对FLASH的擦写次数

/***********************************
功能: 往指定文件中写入传感器记录
参数: char * file_name: 文件名
************************************/
E_RESULT SDRR_WriteRecordToFile(char * file_name)
{
    uint16_t item_len = 0; // 本次数据转化为字符串后的长度
	uint16_t left_len = 0;
	E_RESULT res = APP_FAILED;
	
	item_len = sizeof(sector_buf) - cur_str_len;  // 缓冲区剩余可用的长度 
	res = SDRR_SensorPointToString(&tSDRR, &sector_buf[cur_str_len], &item_len);
    //os_memset(&tSDRR, 0, sizeof(tSDRR));  // 清除记录的传感器数据
	
	cur_str_len += item_len;
	left_len = sizeof(sector_buf) - cur_str_len;

	#if 1
	if(left_len < item_len) // 剩下的不够一条数据长度
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
	   
	  //  将数据保存到FAT 文件中
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
功能: 保存一条传感器的数据
参数: E_SensorType type: 传感器数据点类型
             void  * data: 传感器数据
 返回值: 保存成功: APP_SUCCESS; 失败: APP_FAILED            
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
   // if(tSDRR.sensor_mask == ( (1 << (SENSOR_END)) - 1))  // 各传感器节点已全部保存
	{
	   if(OS_IsTimeout(save_time_out))
	   {		   
	       //os_memset(&tSDRR, 0, sizeof(tSDRR));
	       save_time_out = OS_SetTimeout(SEC(record_gap));  // 默认 60 sec 保存一次记录
	       os_printf("ready to wirte data point to file, tick = %ld\n", OS_GetSysTick());
	       SDRR_WriteRecordToFile(SENSOR_TXT_FILE_NAME);
	   }
	}

	return APP_SUCCESS;
}





