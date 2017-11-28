#ifndef _SPI_FLASH_H_
#define _SPI_FLASH_H_

#include "stm32f10x.h"

//º¯Êý±àÒë¿ª¹Ø
#define ON	1
#define OFF	0
#define USE_GETCHIPID			ON
#define USE_READ_STATUSREG		ON
#define USE_WRITE_STATUSREG		OFF
#define USE_WRITE_ENABLE		ON
#define USE_WRITE_DISABLE		OFF
#define USE_ERASE_CHIP			OFF
#define USE_ERASE_SECTOR		ON
#define USE_ERASE_BLOCK			ON
#define USE_WAIT_BUSY			ON
#define USE_POWERDOWN			OFF
#define USE_RELEASEPOWERDOWN	OFF
#define USE_READ_BYTES			ON
#define USE_WRITE_BYTES			ON
#define USE_READ_SECTOR			ON
#define USE_WRITE_SECTOR		ON





/****************************************
	W25X32	ÃüÁî
****************************************/
#define W25X32_CHIPID				0xEF3016

#define W25X_WriteEnable			0x06
#define W25X_WriteDisable			0x04
#define W25X_ReadStatusReg			0x05
#define W25X_WriteStatusReg			0x01
#define W25X_ReadData				0x03
#define W25X_FastReadData			0x0B
#define W25X_FastReadDual			0x3B
#define W25X_PageProgram			0x02
#define W25X_BlockErase				0xD8
#define W25X_SectorErase			0x20
#define W25X_ChipErase				0xC7
#define W25X_SetPowerDown			0xB9
#define W25X_SetReleasePowerDown	0xAB
#define W25X_DeviceID				0xAB
#define W25X_ManufactDeviceID		0x90
#define W25X_JedecDeviceID			0x9F

//CSÆ¬Ñ¡ÐÅºÅ
#define FLASH_CS_0()			(GPIOB->BRR  = GPIO_Pin_12)   // GPIO_ResetBits(GPIOB, GPIO_Pin_12)
#define FLASH_CS_1()			(GPIOB->BSRR = GPIO_Pin_12)   // GPIO_SetBits(GPIOB,   GPIO_Pin_12)

//Ð¾Æ¬¼ì²â
#define W25X_Check()			((W25X_GetChipID() == W25X32_CHIPID) ? 1 : 0)

int32_t spi2_disk_initialize(void);
uint8_t SPI_Read_Byte(void);
uint8_t SPI_Write_Byte(uint8_t data);

#if USE_GETCHIPID
	int	W25X_GetChipID(void);
#endif

#if USE_READ_STATUSREG
uint8_t		W25X_Read_StatusReg(void);
#endif

#if USE_WRITE_STATUSREG
void	W25X_Write_StatusReg(uint8_t reg);
#endif

#if USE_WRITE_ENABLE
void	W25X_Write_Enable(void);
#endif

#if USE_WRITE_DISABLE
void	W25X_Write_Disable(void);
#endif

#if USE_ERASE_CHIP
void	W25X_Erase_Chip(void);
#endif

#if USE_ERASE_SECTOR
void	SpiFlash_Erase_Sector(uint32_t nDest);
#endif

#if USE_ERASE_BLOCK
void	W25X_Erase_Block(uint32_t nDest);
#endif

#if USE_WAIT_BUSY
void	W25X_Wait_Busy(void);
#endif

#if USE_POWERDOWN
void	W25X_PowerDown(void);
#endif

#if USE_RELEASEPOWERDOWN
void	W25X_ReleasePowerDown(void);
#endif

#if USE_READ_BYTES
void SpiFlash_Read_Bytes(uint32_t nDest,  uint8_t * pBuffer, uint16_t nBytes);
#endif

#if USE_WRITE_BYTES
void SpiFlash_Write_Bytes(uint32_t nDest, uint8_t* pBuffer, uint16_t nBytes);
#endif

#if USE_READ_SECTOR
void SpiFlash_Read_Sector(uint32_t nSector, uint8_t * pBuffer);
#endif

#if USE_WRITE_SECTOR
void SpiFlash_Write_Sector(uint32_t nSector, uint8_t * pBuffer);
#endif

#endif

