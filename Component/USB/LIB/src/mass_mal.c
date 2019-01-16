/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : mass_mal.c
* Author             : MCD Application Team
* Version            : V2.2.0
* Date               : 06/13/2008
* Description        : Medium Access Layer interface
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/


#include "mass_mal.h"
#include "disk_image.h"
#include "spi_flash_interface.h"
#include "rom_flash_interface.h"
#include "sd_card_interface.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

    
	
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
long long Mass_Memory_Size[MAX_LUN + 1];
uint32_t Mass_Block_Size[MAX_LUN + 1];
uint32_t Mass_Block_Count[MAX_LUN + 1];

/*******************************************************************************
* Function Name  : MAL_Init
* Description    : Initializes the Media on the STM32
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_Init(uint8_t lun)
{
  uint16_t status = MAL_OK;

  switch (lun)
  {
    #if FLASH_DISK_EN
    case FLASH_DISK:  // spi flash
	{
		status = flash_initialize();
	}break;
    #endif
	
	#if SD_DISK_EN
    case SD_DISK:
    {
	}break;
    #endif
	
	#if ROM_DISK_EN
	case ROM_DISK:
	{   
		status = rom_flash_initialize();
    }break;
	#endif
	
    default:
      return MAL_FAIL;
  }
  return status;
}
/*******************************************************************************
* Function Name  : MAL_Write
* Description    : Write sectors
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_Write(uint8_t lun, uint64_t Memory_Offset, uint32_t *Writebuff, uint16_t Transfer_Length)
{
	switch(lun)  //这里,根据lun的值确定所要操作的磁盘
	{
	    #if FLASH_DISK_EN
	    case FLASH_DISK:  //磁盘0为 SPI FLASH盘
		{
		    flash_mal_write(lun, Memory_Offset, Writebuff, Transfer_Length);
		}break;
		#endif

		#if SD_DISK_EN
		case SD_DISK:  //磁盘1为SD卡	
		{
		}break;
		#endif

		#if ROM_DISK_EN
		case ROM_DISK:  
	    {
			 rom_flash_mal_write(lun, Memory_Offset, Writebuff, Transfer_Length);
	    }break;
		#endif
		
		default:
			return MAL_FAIL;
	}
	return MAL_OK;
}



/*******************************************************************************
* Function Name  : MAL_Read
* Description    : Read sectors
* Input          : None
* Output         : None
* Return         : Buffer pointer
*******************************************************************************/
uint16_t MAL_Read(uint8_t lun, uint64_t Memory_Offset, uint32_t *Readbuff, uint16_t Transfer_Length)
{
  switch (lun)
  {
    #if FLASH_DISK_EN
    case FLASH_DISK:
	{
		flash_mal_read(lun, Memory_Offset, Readbuff, Transfer_Length);
	}break;
	#endif

	#if SD_DISK_EN
    case SD_DISK:
	{
	}break;
	#endif

	#if ROM_DISK_EN
	case ROM_DISK:
  	{
		rom_flash_mal_read(lun, Memory_Offset, Readbuff, Transfer_Length);
    }break;
	#endif
	
    default:
      return MAL_FAIL;
  }
  return MAL_OK;
}

/*******************************************************************************
* Function Name  : MAL_GetStatus
* Description    : Get status
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_GetStatus (uint8_t lun)
{
  switch(lun)
  {
      #if FLASH_DISK_EN
      case FLASH_DISK:  // SPI FLASH
	  {
         Mass_Block_Count[FLASH_DISK] = SPI_FLASH_SECTOR_COUNT; 
         Mass_Block_Size[FLASH_DISK]  = SPI_FLASH_SECTOR_SIZE; 
         Mass_Memory_Size[FLASH_DISK] = Mass_Block_Count[FLASH_DISK] * Mass_Block_Size[FLASH_DISK];
	  }break;
	  #endif

	  #if SD_DISK_EN
	  case SD_DISK:  // SD 卡
	  {
	  }break;
	  #endif

	  #if ROM_DISK_EN
	  case ROM_DISK:   // ROM FLASH
	  {
		 Mass_Block_Count[ROM_DISK] = ROM_FLASH_SIZE / ROM_FLASH_PAGE_SIZE; 
         Mass_Block_Size[ROM_DISK] =  ROM_FLASH_PAGE_SIZE; 
         Mass_Memory_Size[ROM_DISK] = ROM_FLASH_SIZE; 
	  }break;
	  #endif
	  
	  default:
	  	return MAL_FAIL;
   }
   return MAL_OK;
}

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
