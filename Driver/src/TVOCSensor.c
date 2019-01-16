
#include "TVOCSensor.h"

#include "IIC_Drv.h"
#include "os_timer.h"
#include "PM25Sensor.h"
#include "board_version.h"




#if TVOC_DEBUG_EN
#define TVOC_DEBUG(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#else
#define TVOC_DEBUG(...)
#endif



void TVOC_ReadNByte(uint16_t data_addr, uint8_t * pdata, uint8_t size);


#define TVOC_SESOR_IIC_ADDR  0x5E

static uint8_t rx_buf[4];

unsigned long sumErrCount = 0; // 校验和错误计数
unsigned long totalCount = 0;   // 总数

static os_timer_t tTVOCSensorTimer;
//static 
void TVOCSensorTimer_CallBack(void * arg)
{
    uint8_t new_sum = 0;
	uint16_t tvoc_ppm = 0;
	uint8_t i;
	
      TVOC_IIC_ReadNByteDirectly(TVOC_SESOR_IIC_ADDR, rx_buf, 4);
	new_sum = rx_buf[0] + rx_buf[1] + rx_buf[2];
	tvoc_ppm = ((uint16_t)rx_buf[1] << 8) + rx_buf[2];

	if(rx_buf[0] == 0)
	{
	    os_timer_arm(&tTVOCSensorTimer, 50, 0);
		return;
	}

	totalCount++;
	if(new_sum != rx_buf[3])
	{
	    sumErrCount++;
	    os_printf("\r\nTVOC sum err: new_sum = 0x%x, rx_buf[3] = 0x%x, tick = %ld\n", new_sum, rx_buf[3], os_get_tick());
	}
	else
	{
		//tvoc_ppm *= 0.1;
		os_printf("\r\nget TVOC success: %d.%d ppm, tick = %ld\r\n", tvoc_ppm / 10, tvoc_ppm % 10, os_get_tick());
	}
	os_printf("\r\n");
	for(i = 0; i < 4; i++)
	{
		os_printf("buf[%d] = 0x%x\n", i, rx_buf[i]);
    }
	os_printf("\r\n");
	TVOC_DEBUG("TVOC: %d.%d ppm, tick = %ld\r\n", tvoc_ppm / 10, tvoc_ppm % 10, os_get_tick());
    TVOC_DEBUG("total = %ld, errCount = %ld, errRate = %d%%\r\n", totalCount, sumErrCount, (uint16_t)((double)sumErrCount / totalCount) * 100 );
    if(tvoc_ppm == 0xFFFF)tTempHumi.tvoc = 0xFFFF;
	else 
	{
	   #if USE_TVOC_CAL
       tTempHumi.tvoc = tvoc_ppm * 100; // 单位: ppb
	   #else
	   tTempHumi.tvoc = tvoc_ppm * 10; 
	   #endif
	}
	TVOC_SetSensorExisted(E_TRUE);
	
	os_memset(rx_buf, 0, sizeof(rx_buf));
	
    os_timer_arm(&tTVOCSensorTimer, 100, 0);
	
}

void TVOC_Init(void)
{
       IIC_Init();
	os_timer_setfn(&tTVOCSensorTimer, TVOCSensorTimer_CallBack, NULL);
	os_timer_arm(&tTVOCSensorTimer, 201, 0);
}

void TVOC_ClosePower(void)
{
       IIC_ClosePower();
	os_timer_disarm(&tTVOCSensorTimer);
}





