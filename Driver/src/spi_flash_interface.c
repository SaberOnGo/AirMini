
#include "spi_flash_interface.h"
#include "GlobalDef.h"
#include <sfud.h>
#include "os_global.h"
#include "spi_flash.h"
#include "Ffconf.h"


uint8_t spi_flash_buf[4096];

int32_t flash_initialize(void)
{
	if (sfud_init() == SFUD_SUCCESS) 
	{
       os_printf("init sfud success\r\n");
	   return 0;
	}
	else
	{
	   os_printf("init sfud failed\r\n");
	}
	return 1;
}

DRESULT flash_ioctl(BYTE lun, BYTE cmd, void *buff)
{
  DRESULT res = RES_ERROR;
  
  switch (cmd)
  {
  /* Make sure that no pending write process */
  case CTRL_SYNC :
    res = RES_OK;
    break;
  
  /* Get number of sectors on the disk (DWORD) */
  case GET_SECTOR_COUNT :
    *(DWORD*)buff = SPI_FLASH_SECTOR_COUNT * ( SPI_FLASH_SECTOR_SIZE / _MAX_SS);
    res = RES_OK;
    break;
  
  /* Get R/W sector size (WORD) */
  case GET_SECTOR_SIZE :
    *(WORD*)buff = _MAX_SS;
    res = RES_OK;
    break;
  
  /* Get erase block size in unit of sector (DWORD) */
  // �����block size �Ķ�����nor flash��block���岻ͬ, ������ָ��������С��λ, ����Ϊ4096 B
  case GET_BLOCK_SIZE :   
    *(DWORD*)buff = SPI_FLASH_BLOCK_SIZE;
    break;
  
  default:
    res = RES_PARERR;
  }
  
  return res;
}

/**
  * @brief  Reads Sector(s)
  * @param  lun : not used
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA), ������: 0-511(W25X16, 2MB SPI FLASH)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT flash_disk_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
  uint32_t i;
  const sfud_flash *flash = sfud_get_device_table() + 0;
  BYTE * pbuf = buff;

  for(i = 0; i < count; i++)
  {
     sfud_read(flash, SPI_FLASH_START_ADDR + (sector + i) * _MAX_SS, _MAX_SS, pbuf);  
	 pbuf += _MAX_SS;
  }
  return RES_OK;
}

/**
  * @brief  Writes Sector(s)
  * @param  lun : not used
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT flash_disk_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
    /*
    #if (SPI_FLASH_SECTOR_SIZE == 4096L)
    uint32_t i;
    const sfud_flash *flash = sfud_get_device_table() + 0;
	BYTE * pbuf = (BYTE *)buff;
	
	for(i = 0; i < count; i++)
	{
	   sfud_erase(flash, SPI_FLASH_START_ADDR + (sector + i) * _MAX_SS, _MAX_SS);
	   sfud_write(flash, SPI_FLASH_START_ADDR + (sector + i) * _MAX_SS, _MAX_SS, buff);
	   pbuf += _MAX_SS;
    }
	#elif (SPI_FLASH_SECTOR_SIZE == 512L)
	*/
	
    uint32_t addr  = sector * SPI_FLASH_SECTOR_SIZE;   // ��ʼ��ַ
	uint16_t bytes = count  * SPI_FLASH_SECTOR_SIZE;  // ��д����ֽ���
    
    os_printf("fw: ad = 0x%x, bytes = %ld B\r\n", addr, bytes);

	flash_mal_write(lun, addr, (uint32_t * )buff, bytes);	
	
    //#else
	//#error "SPI_FLASH_SECTOR_SIZE define error\n"
	//#endif
	
	return RES_OK;
}


/******************************************
����: SPI FLASH д�ֽ�����
����:  uint64_t addr: ��ʼд��ĵ�ַ, 24bit
              uint32_t * buffer: ����ָ��
              uint16_t: Ҫд����ֽ���, ��� 65535
**********************************************/
uint16_t flash_mal_write(uint8_t lun, uint32_t addr, uint32_t * buffer, uint16_t bytes)
{
    const sfud_flash *flash = sfud_get_device_table() + 0;
	uint32_t secpos;
	uint16_t secoff;
	uint16_t secremain;	     
    uint8_t * pbuf = (uint8_t *)buffer;
	
 	secpos = addr / 4096;   //������ַ  
	secoff = addr % 4096;   //�������ڵ�ƫ��
	secremain = 4096 - secoff;    //����ʣ��ռ��С   
	
 	os_printf("0 w: ad = 0x%x, len = %d\r\n", addr, bytes); //������
 	if(bytes <= secremain) secremain = bytes; //������4096���ֽ�
	while(1) 
	{	
	    if(secremain < 4096)  // ��д������ݲ���һ������, ���ȶ���֮ǰ������
		{
		    sfud_read (flash, SPI_FLASH_START_ADDR + (secpos << 12), 4096, spi_flash_buf);   //������������������
	    }
		
		sfud_erase(flash, SPI_FLASH_START_ADDR + (secpos << 12), 4096);     //�����������
		os_memcpy(&spi_flash_buf[secoff], pbuf, secremain);             // ��������, ע��һ���������ݴ�С������ 0-4096
		sfud_write(flash, SPI_FLASH_START_ADDR + (secpos << 12), 4096, spi_flash_buf);
		
		if(bytes == secremain)  //д�������
			break;  
		else   //д��δ����
		{
			secpos++;  //������ַ��1
			secoff = 0;  //ƫ��λ��Ϊ0 	 
		   	pbuf += secremain;  				 //ָ��ƫ��
			//addr   += secremain;				     //д��ַƫ��	   
		   	bytes  -= secremain;			         //�ֽ����ݼ�
			if(bytes > 4096)secremain = 4096;    //��һ����������д����
			else secremain = bytes;		         //��һ����������д����
		}	 
	} 
	return 0;
}

uint16_t flash_mal_read(uint8_t lun, uint32_t addr, uint32_t * buffer, uint16_t bytes)
{
    const sfud_flash *flash = sfud_get_device_table() + 0;

    os_printf("0 r: ad = 0x%x, len = %d, tick = %ld\r\n", addr, bytes, os_get_tick()); //������
	return sfud_read(flash, SPI_FLASH_START_ADDR + addr, bytes, (uint8_t *)buffer);
}

uint32_t spi_flash_capacity = FL_SIZE_KB(4096);   // W25Q16

void flash_set_total_capacity(uint32_t capacity)
{
      spi_flash_capacity = capacity;
      os_printf("flash capacity = %ld B\r\n", capacity);
}
uint32_t flash_get_total_capacity(void)
{
     return spi_flash_capacity;
}


