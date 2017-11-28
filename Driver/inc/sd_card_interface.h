
/***************************** SD CARD 存储接口访问头文件 ****************************************************/
#ifndef __SD_CARD_INTERFACE_H__
#define  __SD_CARD_INTERFACE_H__

#include <stdint.h>

#include "diskio.h"
#include "Integer.h"

#define SD_CARD_DISK_SIZE            ((uint64_t)((uint64_t)2 * 1024 * 1024 * 1024))   // 2GB SD 卡
#define SD_CARD_SECTOR_SIZE          512                           // 扇区大小
#define SD_CARD_BLOCK_SIZE           512           // 块大小

int32_t SD_initialize(void);
DRESULT SD_ioctl(BYTE lun, BYTE cmd, void *buff);

#endif

