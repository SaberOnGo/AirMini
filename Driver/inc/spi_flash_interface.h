
/*********************************** SPI FLASH �洢�ӿڷ���ͷ�ļ� ********************************************/
#ifndef __SPI_FLASH_INTERFACE_H__
#define  __SPI_FLASH_INTERFACE_H__

#include <stdint.h>
#include "diskio.h"
#include "Integer.h"

#include "Ffconf.h"


//Flash��ز���
/************************************************************************************************************************************************************
 spi flash ����������СΪ4KB, ������������Ϊ512, ���� 2MB
 ���� 260KB �ռ� Ϊ Ӧ�ó����ϵͳ���������洢��
| <---sector 0 - sector 31--->  | <-----------sector 32 ~ 63-----------> | <--sector 64 ----- >  |<-sector 65 ~ sector 511-------->   |
| <-----128 KB -----------> | <--------128 KB--------------------> | <------4KB------->|---------�ļ�ϵͳ---------- -|
| APP1 ������(�³���)     |  APP2 ������(��һ���汾����)    |   ϵͳ������        |---------�ļ�ϵͳ----------- |  
|-------------------------- 260 KB (65����������)    --------------------------------|--1788 KB FatFs �ļ�ϵͳ��---|
*************************************************************************************************************************************************************/
#define FL_SIZE_KB(count)    (((uint32_t)count) * 1024L )    // ��ʾ�ж���KB

extern uint32_t flash_get_total_capacity(void);

// SPI FLASH �����Сʵ�ʶ���
#if 1
#define FL_TOTAL_SIZE      flash_get_total_capacity()
#else
#define FL_TOTAL_SIZE      FL_SIZE_KB(4096)     // W25Q32
#endif

#define FL_SECTOR_SIZE     (4096L)     // ��������ʵ�ʴ�С
#define FL_PAGE_SIZE	   256L        //  �ɱ�̵���С��λ: ҳ, �Ĵ�С, Unit: B
#define FL_PAGES_PER_SECTOR	(FL_SECTOR_SIZE / FL_PAGE_SIZE)   // ÿ������ҳ��


// FAT �߼�������ض���
#if (_MAX_SS == 4096)
#define SPI_FLASH_SECTOR_SIZE		(4096L)            //  FatFs ������С: 4KB, �� SPI FLASH ��������Сһ��(4KB)
#elif (_MAX_SS == 512)
#define SPI_FLASH_SECTOR_SIZE		(512L)             //  FatFs������С: 512B, �� SPI FLASH������(4KB)��С��ͬ
#else
#define "flash macro define error"
#endif

/************************************************************************************************************************************************************
 spi flash ����������СΪ4KB, ������������Ϊ512, ���� 2MB
 ���� 260KB �ռ� Ϊ Ӧ�ó����ϵͳ���������洢��
| <---sector 0 - sector 31--->  | <-----------sector 32 ~ 63-----------> | <--sector 64 ----- >  |<-sector 65 ~ sector 511-------->   |
| <-----128 KB -----------> | <--------128 KB--------------------> | <------4KB------->|---------�ļ�ϵͳ---------- -|
| APP1 ������(�³���)     |  APP2 ������(��һ���汾����)    |   ϵͳ������        |---------�ļ�ϵͳ----------- |  
|-------------------------- 260 KB (65����������)    --------------------------------|--1788 KB FatFs �ļ�ϵͳ��---|
*************************************************************************************************************************************************************/

#define SPI_FLASH_BLOCK_SIZE		FL_SECTOR_SIZE               // 65536    // ���С: 64 KB,  Block Size in Bytes 
#define SPI_FLASH_START_ADDR        (65L * FL_SECTOR_SIZE)        //  FATFSϵͳ��ʼ��ַ, �ӵ�65����������ʼ
#define SPI_FLASH_SECTOR_COUNT		((FL_TOTAL_SIZE  - SPI_FLASH_START_ADDR) / SPI_FLASH_SECTOR_SIZE)  //  SPI FLASH �е�FatFs������������

	

//CSƬѡ�ź�
#define SPI_FLASH_CS_0()			(GPIOB->BRR  = GPIO_Pin_12)  // GPIO_ResetBits(GPIOB, GPIO_Pin_12)
#define SPI_FLASH_CS_1()			(GPIOB->BSRR = GPIO_Pin_12)  // GPIO_SetBits(GPIOB,   GPIO_Pin_12)

extern uint8_t spi_flash_buf[4096];

void flash_reset(void);

int32_t  flash_initialize(void);
DRESULT flash_ioctl(BYTE lun, BYTE cmd, void *buff);
DRESULT flash_disk_read(BYTE lun, BYTE *buff, DWORD sector, UINT count);
DRESULT flash_disk_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count);

uint16_t flash_mal_write(uint8_t lun, uint32_t addr, uint32_t * buffer, uint16_t bytes);
uint16_t flash_mal_read(uint8_t lun, uint32_t addr, uint32_t * buffer, uint16_t bytes);

void flash_set_total_capacity(uint32_t capacity);
uint32_t flash_get_total_capacity(void);

#endif   // end of file

