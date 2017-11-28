
#ifndef __IIC_DRV_H__
#define  __IIC_DRV_H__

#include "GlobalDef.h"



#define IIC_READ  0x01
#define IIC_WRITE 0x00

void IIC_Init(void);

SYS_RESULT IIC_WriteNByte(uint8_t sla_addr, uint16_t data_addr, uint8_t * pdata, uint8_t size);

SYS_RESULT IIC_ReadNByteExt(uint8_t sla_addr, uint16_t data_addr, uint8_t * pdata, uint8_t size, uint8_t restart_iic);
SYS_RESULT IIC_ReadNByte(uint8_t sla_addr, uint16_t data_addr, uint8_t * pdata, uint8_t size);
SYS_RESULT IIC_ReadNByteDirectly(uint8_t sla_addr, uint8_t * pdata, uint8_t size);


#endif

