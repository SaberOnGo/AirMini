#include "stm32f10x.h"
#include "spi_flash.h"


static void SPI2_Init(void);

int32_t spi2_disk_initialize(void)
{
	SPI2_Init();
	return 0;
}

static void SPI2_Init(void)
{
	SPI_InitTypeDef	SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable SPI1 GPIOA and GPIOB clocks */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2 | RCC_APB2Periph_GPIOA |RCC_APB2Periph_GPIOB, ENABLE);
	    
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_12 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;   //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Deselect the FLASH: Chip Select high */
	FLASH_CS_1();

	/* Configure SPI1 pins: NSS, SCK, MISO and MOSI */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* SPI1 configuration */ 
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI2, &SPI_InitStructure);

	//SPI_I2S_DMACmd(SPI1,SPI_I2S_DMAReq_Rx | SPI_I2S_DMAReq_Tx,ENABLE);

	/* Enable SPI2 Rx buffer DMA transfer request */
	//SPI_DMACmd(SPI1, SPI_DMAReq_Rx | SPI_DMAReq_Tx, ENABLE);


	/* Enable SPI1	*/
	SPI_Cmd(SPI2, ENABLE);	
}

uint8_t SPI_Read_Byte(void)
{
	return (SPI_Write_Byte(0xFF));
}

uint8_t SPI_Write_Byte(uint8_t data)
{
	/* Loop while DR register in not emplty */
	while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);

	/* Send u8 through the SPI1 peripheral */
	SPI_I2S_SendData(SPI2, data);

	/* Wait to receive a u8 */
	while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
 
	/* Return the u8 read from the SPI bus */
	return SPI_I2S_ReceiveData(SPI2);
}

#if USE_GETCHIPID
int W25X_GetChipID(void)
{
	int nID = 0;
	
	FLASH_CS_0();

	SPI_Write_Byte(W25X_JedecDeviceID);
	nID = SPI_Read_Byte();
	nID <<= 8;
	nID |= SPI_Read_Byte();
	nID <<= 8;
	nID |= SPI_Read_Byte();
	
	FLASH_CS_1();

	return nID;
}
#endif

#if USE_READ_STATUSREG
uint8_t W25X_Read_StatusReg(void)
{	uint8_t ret = 0;
	FLASH_CS_0();
	SPI_Write_Byte(W25X_ReadStatusReg);
	ret = SPI_Read_Byte();
	FLASH_CS_1();	
	return ret;
}
#endif

#if USE_WRITE_STATUSREG
void W25X_Write_StatusReg(uint8_t reg)
{	FLASH_CS_0();
	SPI_Write_Byte(W25X_WriteStatusReg);
	SPI_Write_Byte(reg);
	FLASH_CS_1();
}
#endif

#if USE_WRITE_ENABLE
void W25X_Write_Enable(void)
{	FLASH_CS_0();
	SPI_Write_Byte(W25X_WriteEnable);
	FLASH_CS_1();
}
#endif

#if USE_WRITE_DISABLE
void W25X_Write_Disable(void)
{	FLASH_CS_0();
	SPI_Write_Byte(W25X_WriteDisable);
	FLASH_CS_1();
}
#endif

#if USE_WAIT_BUSY
void W25X_Wait_Busy(void)
{	
	while(W25X_Read_StatusReg() == 0x03)
		W25X_Read_StatusReg();
}
#endif

#if USE_ERASE_SECTOR
void SpiFlash_Erase_Sector(uint32_t nDest)
{
//	nDest *= FLASH_SECTOR_SIZE;
	
	FLASH_CS_0();			
	W25X_Write_Enable();
	FLASH_CS_0();
	SPI_Write_Byte(W25X_SectorErase);
	SPI_Write_Byte((uint8_t)((nDest & 0xFFFFFF) >> 16));
	SPI_Write_Byte((uint8_t)((nDest & 0xFFFF) >> 8));
	SPI_Write_Byte((uint8_t)nDest & 0xFF);
	FLASH_CS_1();
	W25X_Wait_Busy();
}
#endif

#if USE_ERASE_BLOCK
void W25X_Erase_Block(uint32_t nDest)
{
	nDest *= FLASH_BLOCK_SIZE;
	
	FLASH_CS_0();			
	W25X_Write_Enable();
	FLASH_CS_0();
	SPI_Write_Byte(W25X_SectorErase);
	SPI_Write_Byte((uint8_t)((nDest & 0xFFFFFF) >> 16));
	SPI_Write_Byte((uint8_t)((nDest & 0xFFFF) >> 8));
	SPI_Write_Byte((uint8_t)nDest & 0xFF);
	FLASH_CS_1();
	W25X_Wait_Busy();
}
#endif

#if USE_ERASE_CHIP
void W25X_Erase_Chip(void)
{
	FLASH_CS_0();
	W25X_Write_Enable();
	FLASH_CS_0();
	W25X_Wait_Busy();
	FLASH_CS_0();
	SPI_Write_Byte(W25X_ChipErase);
	FLASH_CS_1();
	W25X_Wait_Busy();
}
#endif

#if USE_READ_BYTES
void SpiFlash_Read_Bytes(uint32_t nDest, uint8_t * pBuffer, uint16_t nBytes)
{	
	uint16_t i;
	
	FLASH_CS_0();
	SPI_Write_Byte(W25X_ReadData);
	SPI_Write_Byte((uint8_t)(nDest >> 16));
	SPI_Write_Byte((uint8_t)(nDest >> 8));
	SPI_Write_Byte((uint8_t)nDest);
	for(i=0;i<nBytes;i++)
	{
		pBuffer[i] = SPI_Read_Byte();
	}
	
	FLASH_CS_1();
	W25X_Wait_Busy();
}
#endif

#if USE_READ_SECTOR
void SpiFlash_Read_Sector(uint32_t nSector, uint8_t * pBuffer)
{	
    uint16_t i;
	
	//扇区号转为地址
	nSector *= FLASH_SECTOR_SIZE;
	
	FLASH_CS_0();
	SPI_Write_Byte(W25X_ReadData);
	SPI_Write_Byte((uint8_t)(nSector >> 16));
	SPI_Write_Byte((uint8_t)(nSector>> 8));
	SPI_Write_Byte((uint8_t) nSector);
	
	for(i=0;i<FLASH_SECTOR_SIZE;i++)
	{	pBuffer[i] = SPI_Read_Byte();
	}
	FLASH_CS_1();
	W25X_Wait_Busy();
}
#endif

#if USE_WRITE_BYTES
void SpiFlash_Write_Bytes(uint32_t nSector, uint8_t * pBuffer, uint16_t nBytes)
{
	uint16_t i;

	for(i=0;i<nBytes;i++)	
	{	
		W25X_Write_Enable(); 
		
		FLASH_CS_0();		
		SPI_Write_Byte(W25X_PageProgram);
		SPI_Write_Byte((uint8_t)(((nSector+i) & 0xFFFFFF) >> 16));  /* 发送3个字节的地址信息 */
		SPI_Write_Byte((uint8_t)(((nSector+i) & 0xFFFF) >> 8));
		SPI_Write_Byte((uint8_t)(nSector+i) & 0xFF);

	//	for(i=0;i<nBytes;i++)
		SPI_Write_Byte(pBuffer[i]);
		
		FLASH_CS_1();
	
		W25X_Wait_Busy();
	}


}
#endif

#if USE_WRITE_SECTOR
void SpiFlash_Write_Sector(uint32_t nSector, uint8_t * pBuffer)
{	
	int i,j;
	
	//扇区号转为地址
	nSector *= FLASH_SECTOR_SIZE;
	
	//一个扇区需要几个页
	for(j=0;j<FLASH_PAGES_PER_SECTOR;j++)
	{
		FLASH_CS_0();
		W25X_Write_Enable();
		FLASH_CS_0();
		
		SPI_Write_Byte(W25X_PageProgram);
		SPI_Write_Byte((uint8_t)(nSector >> 16));
		SPI_Write_Byte((uint8_t)(nSector >> 8));	
		SPI_Write_Byte((uint8_t) nSector);
		
		for(i=0;i<FLASH_PAGE_SIZE;i++)								
			SPI_Write_Byte(pBuffer[i]);
		
		pBuffer += FLASH_PAGE_SIZE;
		nSector += FLASH_PAGE_SIZE;

		FLASH_CS_1();
		W25X_Wait_Busy();
	}
}
#endif
