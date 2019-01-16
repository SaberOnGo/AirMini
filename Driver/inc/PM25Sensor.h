
#ifndef  __PM25SENSOR_H__
#define   __PM25SENSOR_H__

#include "GlobalDef.h"



// PM25 Sensor的数据

#pragma pack(1)
// HCHO 浓度(PMS5003S)或版本号(PMS5003)
typedef union
{
   uint16_t hcho;   // 甲醛浓度, 真实浓度 = 本数值 / 1000, unit: mg/m3
   uint8_t  ver[2]; // ver[0] 为版本号, ver[1] 为错误代码
}U_HCHO_VERSION;

// PM25 传感器串口协议规定的内容
typedef struct
{
   uint8_t start_char_0;
   uint8_t start_char_1;
   uint16_t len;   // 数据长度: 2 x 13 + 2
   
   uint16_t pm1_cf1;     // PM1.0 ug/m3
   uint16_t pm25_cf1;    // PM25   ug/m3
   uint16_t pm10_cf1;    // PM10   ug/m3
   uint16_t pm1_air;     // PM1.0, 大气环境下
   uint16_t pm25_air;    // PM25, 大气环境下
   uint16_t pm10_air;    // PM10, 大气环境下
   
   uint16_t PtCnt_0p3um;      // Particle Count, 0.1 L 空气中0.3um以上颗粒物个数
   uint16_t PtCnt_0p5um;
   uint16_t PtCnt_1p0um;
   uint16_t PtCnt_2p5um;
   uint16_t PtCnt_5p0um;
   uint16_t PtCnt_10p0um;
   
   U_HCHO_VERSION extra;  // 额外数据
   uint16_t sum;             // 校验和
}T_PM25CmdContent;

// AQI 标准
typedef enum
{
   AQI_US = 0,
   AQI_CN,
}E_AQI_STD;

// AQI 计算
typedef struct
{
   #if 0
   float C_low_us;     // 浓度低值, 美国标准
   float C_high_us;    // 浓度高值, 美国标准
   float C_low_cn;     // 浓度低值, 中国标准
   float C_high_cn;    // 浓度高值, 中国标准
   #else
   uint16_t C_low_us;     // 浓度低值, 美国标准
   uint16_t C_high_us;    // 浓度高值, 美国标准
   uint16_t C_low_cn;     // 浓度低值, 中国标准
   uint16_t C_high_cn;    // 浓度高值, 中国标准
   #endif
   
   uint16_t I_low;     // AQI 低值
   uint16_t I_high;    // AQI 高值
}T_AQI;

// HCHO 数据
typedef struct
{
    uint8_t start_char_0;   // 起始头 0xFF
    uint8_t gas_name;       // 气体名称
    uint8_t unit;           // 单位： Ppb
    uint8_t decimal_digits;   // 小数位数
    uint8_t air_high;          // 气体浓度高位
    uint8_t air_low;           // 气体浓度低位
    uint8_t full_scale_high;    // 满量程高位
    uint8_t full_scale_low;     // 满量程低位
    uint8_t check_sum;            // 校验和
}T_HCHOCmdContent;

//温湿度数据
typedef struct
{
    uint16_t  temp;   // 一共有5位, 1位符号, 4位有效值, 如 12011 则表示: -20.11 'C, 第5位 为 1表示为负值
    uint16_t  humi;  // 一共有4位, 3位有效值, 如 3156 则表示: 31.5% RH, 第5位 无效, 小数点后第2位无意义
    uint16_t  tvoc;  // 一共有4位, 3位有效值, 如 2890, 表示: 28.9 ppm, 小数点后第2位无意义
}T_TempHumi;

extern T_TempHumi tTempHumi;   // 温湿度值
extern  uint16_t g_hcho_mass;
extern uint16_t  g_pm2p5_ug;
extern uint16_t   g_pm10_ug;

#pragma pack()

void AppVersionInfo(uint8_t display_mode);


void PM25_Init(void);
//#if (SENSOR_SELECT != HCHO_SENSOR)
void PM25_Receive(uint8_t * buf, uint16_t len);
void PM25_Display_Init(uint8_t display_mode);
//#else
void HCHO_Receive(uint8_t * buf, uint16_t len);
uint8_t HCHO_CheckSum(uint8_t * buf, uint16_t len);
void HCHO_SwitchToQueryMode(void);
void HCHO_QueryHCHOConcentration(void);
void HCHO_SwitchToActiveMode(void);
void HCHO_Indicate(uint16_t val);
void HCHO_DisplayInit(uint8_t display_mode);
void LED_IndicateColorOfHCHO(void);
void HCHO_CaliSet(void);

//#endif


void Sensor_SubStateToNext(void);

void TVOC_SetSensorExisted(uint8_t state);
void TIME_SetSensorExisted(uint8_t state);
void TIME_StopChargeDisplay(void);


#endif

