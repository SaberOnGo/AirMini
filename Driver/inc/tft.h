#ifndef   _TFT_H
#define   _TFT_H

#include "stm32f10x_conf.h"


//ÑÕÉ«¶¨Òå
#define RED	    0XF800
#define GREEN   0X07E0
#define BLUE    0X001F  
#define BRED    0XF81F
#define GRED    0XFFE0
#define GBLUE   0X07FF
#define BLACK   0 

#define Bank1_LCD_D    ((u32)0x60020000)    //disp Data ADDR
#define Bank1_LCD_C    ((u32)0x60000000)	 //disp Reg ADDR


void TFT_IO_Init(void);
void TFT_Init(void);
void LCD_WR_REG(unsigned int index);
void LCD_WR_CMD(unsigned int index,unsigned int val);    
void LCD_WR_Data(unsigned long val);
void LCD9325_Init(void);
void lcd_rst(void);
void DispOneColor(unsigned int Color);
void SetWindows(unsigned char X0,unsigned char X,unsigned int Y0,unsigned int Y);
void ini(void);
u16  LCD_ReadPoint(u16 x,u16 y);
u16 ili9320_BGR2RGB(u16 c);
unsigned int ili9320_ReadData(void);
unsigned int LCD_RD_data(void);
void LCD_DrawPoint(u16 x,u16 y,u16 color);
void Swap( u16 * a , u16 * b );
int BresenhamLine( u16 x1 , u16 y1 , u16 x2 , u16 y2 , u16 c);



#endif


