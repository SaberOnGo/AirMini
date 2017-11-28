
#ifndef __LCD1602_DRV_H__
#define  __LCD1602_DRV_H__

#include "stm32f10x.h"

typedef  unsigned int uint;
typedef  unsigned char uchar;



/***********************º¯ÊýÉùÃ÷***********************************/

void LCD1602_HardwareInit(void);
void LCD1602_Init(void);
void LCD1602_Close(void);

void LCD1602_SetXY(uint8_t x, uint8_t y );
void LCD1602_WriteCmd(uint8_t ins );
void LCD1602_WriteData(uint8_t data );
void LCD1602_WriteString(uint8_t x, uint8_t y, const uint8_t *s );
void LCD1602_WriteInteger(uint8_t x, uint8_t y, uint32_t val, uint8_t placeholder_size);
void LCD1602_WriteInt(uint8_t x, uint8_t y, uint16_t val);

void LCD1602_ClearScreen(void);




#endif

