
#ifndef  __PM25SENSOR_H__
#define   __PM25SENSOR_H__

#include "GlobalDef.h"



// PM25 Sensor������

#pragma pack(1)
// HCHO Ũ��(PMS5003S)��汾��(PMS5003)
typedef union
{
   uint16_t hcho;   // ��ȩŨ��, ��ʵŨ�� = ����ֵ / 1000, unit: mg/m3
   uint8_t  ver[2]; // ver[0] Ϊ�汾��, ver[1] Ϊ�������
}U_HCHO_VERSION;

// PM25 ����������Э��涨������
typedef struct
{
   uint8_t start_char_0;
   uint8_t start_char_1;
   uint16_t len;   // ���ݳ���: 2 x 13 + 2
   
   uint16_t pm1_cf1;     // PM1.0 ug/m3
   uint16_t pm25_cf1;    // PM25   ug/m3
   uint16_t pm10_cf1;    // PM10   ug/m3
   uint16_t pm1_air;     // PM1.0, ����������
   uint16_t pm25_air;    // PM25, ����������
   uint16_t pm10_air;    // PM10, ����������
   
   uint16_t PtCnt_0p3um;      // Particle Count, 0.1 L ������0.3um���Ͽ��������
   uint16_t PtCnt_0p5um;
   uint16_t PtCnt_1p0um;
   uint16_t PtCnt_2p5um;
   uint16_t PtCnt_5p0um;
   uint16_t PtCnt_10p0um;
   
   U_HCHO_VERSION extra;  // ��������
   uint16_t sum;             // У���
}T_PM25CmdContent;

// AQI ��׼
typedef enum
{
   AQI_US = 0,
   AQI_CN,
}E_AQI_STD;

// AQI ����
typedef struct
{
   #if 0
   float C_low_us;     // Ũ�ȵ�ֵ, ������׼
   float C_high_us;    // Ũ�ȸ�ֵ, ������׼
   float C_low_cn;     // Ũ�ȵ�ֵ, �й���׼
   float C_high_cn;    // Ũ�ȸ�ֵ, �й���׼
   #else
   uint16_t C_low_us;     // Ũ�ȵ�ֵ, ������׼
   uint16_t C_high_us;    // Ũ�ȸ�ֵ, ������׼
   uint16_t C_low_cn;     // Ũ�ȵ�ֵ, �й���׼
   uint16_t C_high_cn;    // Ũ�ȸ�ֵ, �й���׼
   #endif
   
   uint16_t I_low;     // AQI ��ֵ
   uint16_t I_high;    // AQI ��ֵ
}T_AQI;

// HCHO ����
typedef struct
{
    uint8_t start_char_0;   // ��ʼͷ 0xFF
    uint8_t gas_name;       // ��������
    uint8_t unit;           // ��λ�� Ppb
    uint8_t decimal_digits;   // С��λ��
    uint8_t air_high;          // ����Ũ�ȸ�λ
    uint8_t air_low;           // ����Ũ�ȵ�λ
    uint8_t full_scale_high;    // �����̸�λ
    uint8_t full_scale_low;     // �����̵�λ
    uint8_t check_sum;            // У���
}T_HCHOCmdContent;

//��ʪ������
typedef struct
{
    uint16_t  temp;   // һ����5λ, 1λ����, 4λ��Чֵ, �� 12011 ���ʾ: -20.11 'C, ��5λ Ϊ 1��ʾΪ��ֵ
    uint16_t  humi;  // һ����4λ, 3λ��Чֵ, �� 3156 ���ʾ: 31.5% RH, ��5λ ��Ч, С������2λ������
    uint16_t  tvoc;  // һ����4λ, 3λ��Чֵ, �� 2890, ��ʾ: 28.9 ppm, С������2λ������
}T_TempHumi;

extern T_TempHumi tTempHumi;   // ��ʪ��ֵ
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

