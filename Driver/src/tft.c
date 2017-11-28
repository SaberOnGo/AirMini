#include "tft.h"
#include "stm32f10x_conf.h"
#include "MyType.h"
#include "FSMC.h"
#include "delay.h"
#include "string.h"
#include <stdio.h>





//LCD总初始化
void TFT_Init(void)
{
	FSMC_Init();//FSMC初始化
	TFT_IO_Init();//LCD_IO口初始化
	LCD9325_Init();
}


/*********************
该函数为TFT2.8英寸屏幕的IO口初始化
*********************/
void TFT_IO_Init(void)
{
	
	RCC->APB2ENR |= BIT(6)|BIT(5); //使能GPIOE,GPIOD时钟

	 /****************************************************************
	 模拟输入模式              0     	  浮空输入模式	             4
	 上拉/下拉输入模式	       8	      保留                       C
	 通用推挽输出模式（50M）   3		  通用开漏输出模式（50M）	 7
	 复用推挽输出模式（50M）   B		  复用开漏输出模式（50M）	 F
	  ****************************************************************/
	/****************************************************************
	16位数据口初始化
	DB0(PD14), DB1(PD15), DB2(PD0),  DB3(PD1)
	DB4(PE7),  DB5(PE8),  DB6(PE9),  DB7(PE10)
	DB8(PE11), DB9(PE12), DB10(PE13),DB11(PE14)
	DB12(PE15),DB13(PD8), DB14(PD9), DB15(PD10)
	******************************************************************/
	GPIOD->CRL   &= 0XFFFFFF00;//PD0,1复用推挽输出 
	GPIOD->CRL   |= 0x000000BB;

	GPIOD->CRH   &= 0X00FFF000;//PD8,9,10,14,15复用推挽输出 
	GPIOD->CRH   |= 0xBB000BBB;

	GPIOE->CRH   &= 0X00000000;//PE8,9,10,11,12,13,14,15复用推挽输出 
	GPIOE->CRH   |= 0xBBBBBBBB;

	GPIOE->CRL   &= 0X0FFFFFFF;//PE7复用推挽输出 
	GPIOE->CRL   |= 0xB0000000;

    /*****************************************************
   以上为数据口初始化
   *****************************************************/
	GPIOD->CRH   &= 0XFFFF0FFF;//PD11(LCD_RS:FSMC_A16)复用推挽输出 
	GPIOD->CRH   |= 0x0000B000;

	GPIOD->CRL   &= 0X0F00FFFF;//PD4(LCD_RD:FSMC_nOE),PD7(LCD_CS:FSMC_NE1)复用推挽输出 
	GPIOD->CRL   |= 0xB0BB0000;//PD5(LCD_WR:FSMC_nWE),复用推挽输出

    GPIOE->CRL   &= 0XFFFFFF0F;//PE1(LCD_RESET),通用推挽输出 
	GPIOE->CRL   |= 0x00000030;

	GPIOD->BSRR  |=(1<<4)|(1<<5)|(1<<7)|(1<<11);//LCD_RD,LCD_WR,LCD_CS,LCD_RS置1
	GPIOE->BRR   |=(1<<1);//LCD_RESET置0
}

//写寄存器地址函数
void LCD_WR_REG(unsigned int index)
{
	*(volatile u16 *) (Bank1_LCD_C)= index;
}

//写地址跟数据到寄存器
void LCD_WR_CMD(unsigned int index,unsigned int val)
{	
	*(volatile u16 *) (Bank1_LCD_C)= index;	
	*(volatile u16 *) (Bank1_LCD_D)= val;
}

//写数据函数
void  LCD_WR_Data(unsigned long val)
{   
	*(volatile u16 *) (Bank1_LCD_D)= val; 
}


// ++++++++++++++++TFT 复位操作
void lcd_rst(void)
{
    GPIOE->BRR  |=(1<<1);//LCD_RESET置0
    delay_ms(10);					   
    GPIOE->BSRR |=(1<<1);//LCD_RESET置1		 	 
    delay_ms(10);	
}

//初始化函数
void LCD9325_Init(void)
{
	lcd_rst(); 

	LCD_WR_CMD(0x00E5, 0x8000); 	 // Set the internal vcore voltage
	LCD_WR_CMD(0x0000, 0x0001); 	 // Start internal OSC.
	delay_ms(10);
	LCD_WR_CMD(0x0001, 0x0100);	 // set SS and SM bit
	LCD_WR_CMD(0x0002, 0x0700);	 // set 1 line inveLCD_RSion
	LCD_WR_CMD(0x0003, 0x1030);	 // set GRAM write direction and BGR=1.
	LCD_WR_CMD(0x0004, 0x0000);	 // Resize register

	LCD_WR_CMD(0x0008, 0x0202);	 // set the back porch and front porch
	LCD_WR_CMD(0x0009, 0x0000);	 // set non-display area refresh cycle ISC[3:0]
	LCD_WR_CMD(0x000A, 0x0000);	 // FMARK function
	LCD_WR_CMD(0x000C, 0x0000);		 // RGB interface setting
	LCD_WR_CMD(0x000D, 0x0000);	 // Frame marker Position
	LCD_WR_CMD(0x000F, 0x0000);		 // RGB interface polarity
	LCD_WR_CMD(0x002b, 0x0020);   //frame rate and color control(0x0000)

	//*************Power On sequence ****************
	LCD_WR_CMD(0x0010, 0x0000);		 // SAP, BT[3:0], AP, DSTB, SLP, STB
	LCD_WR_CMD(0x0011, 0x0004);		 // DC1[2:0], DC0[2:0], VC[2:0]
	LCD_WR_CMD(0x0012, 0x0000);		 // VREG1OUT voltage
	LCD_WR_CMD(0x0013, 0x0000);		 // VDV[4:0] for VCOM amplitude
	delay_ms(200);;				// Dis-charge capacitor power voltage

	LCD_WR_CMD(0x0010, 0x17B0);		 // SAP, BT[3:0], AP, DSTB, SLP, STB
	LCD_WR_CMD(0x0011, 0x0001);		 // DC1[2:0], DC0[2:0], VC[2:0]
	delay_ms(50);					 // Delay 50ms
	LCD_WR_CMD(0x0012, 0x013e);		 // VREG1OUT voltage
	delay_ms(50);					 // Delay 50ms
	LCD_WR_CMD(0x0013, 0x1c00);		 // VDV[4:0] for VCOM amplitude
	LCD_WR_CMD(0x0029, 0x001e);		 // VCM[4:0] for VCOMH
	delay_ms(50);

	LCD_WR_CMD(0x0020, 0x0000);		 // GRAM horizontal Address
	LCD_WR_CMD(0x0021, 0x0000);		 // GRAM Vertical Address

	// ----------- Adjust the Gamma	Curve ----------//
	LCD_WR_CMD(0x0030, 0x0002);
	LCD_WR_CMD(0x0031, 0x0606);
	LCD_WR_CMD(0x0032, 0x0501);


	LCD_WR_CMD(0x0035, 0x0206);
	LCD_WR_CMD(0x0036, 0x0504);
	LCD_WR_CMD(0x0037, 0x0707);
	LCD_WR_CMD(0x0038, 0x0306);
	LCD_WR_CMD(0x0039, 0x0007);

	LCD_WR_CMD(0x003C, 0x0700);
	LCD_WR_CMD(0x003D, 0x0700);

	//------------------ Set GRAM area ---------------//
	LCD_WR_CMD(0x0050, 0x0000);		// Horizontal GRAM Start Address
	LCD_WR_CMD(0x0051, 0x00EF);		// Horizontal GRAM End Address
	LCD_WR_CMD(0x0052, 0x0000);		// Vertical GRAM Start Address
	LCD_WR_CMD(0x0053, 0x013F);		// Vertical GRAM Start Address


	//LCD_WR_CMD(0x0060, 0xA700);		// Gate Scan Line
	LCD_WR_CMD(0x0060, 0x2700);		// Gate Scan Line
	LCD_WR_CMD(0x0061, 0x0001);		// NDL,VLE, REV
	LCD_WR_CMD(0x006A, 0x0000);		// set scrolling line

	//-------------- Partial Display Control ---------//
	LCD_WR_CMD(0x0080, 0x0000);
	LCD_WR_CMD(0x0081, 0x0000);
	LCD_WR_CMD(0x0082, 0x0000);
	LCD_WR_CMD(0x0083, 0x0000);
	LCD_WR_CMD(0x0084, 0x0000);
	LCD_WR_CMD(0x0085, 0x0000);

	//-------------- Panel Control -------------------//
	LCD_WR_CMD(0x0090, 0x0010);
	LCD_WR_CMD(0x0092, 0x0000);
	LCD_WR_CMD(0x0093, 0x0003);
	LCD_WR_CMD(0x0095, 0x0110);
	LCD_WR_CMD(0x0097, 0x0000);
	LCD_WR_CMD(0x0098, 0x0000);
	ini();
	LCD_WR_CMD(0x0007, 0x0173);		// 262K color and display ON
	LCD_WR_CMD(3, 0x1018);
	LCD_WR_REG(0x0022);
}

/**************************************************************************************
* 名    称: DispOneColor
* 功    能: 全屏显示某种颜色
* 参    数: Color:颜色值
**************************************************************************************/
void DispOneColor(unsigned int Color)
{
    unsigned char j;
	unsigned int i;
    LCD_WR_CMD(0x0021,0);        //行首址0,行最大值为320
    LCD_WR_CMD(0x0020,0);        //列首址0，列最大值为240
    LCD_WR_REG(0x22);//写数据到GRAM命令
    for(j=0;j<240;j++)
	{
        for(i=0;i<320;i++)
		{
			LCD_WR_Data(Color);        //写颜色
		}
	}
}

void SetWindows(unsigned char X0,unsigned char X,unsigned int Y0,unsigned int Y)
{
	LCD_WR_CMD(3, 0x1030);//显示模式
    LCD_WR_CMD(80, X0); /* X初始坐标 */
    LCD_WR_CMD(81, X); //X结束坐标   
    LCD_WR_CMD(82, Y0); /* Y初始坐标 */
    LCD_WR_CMD(83, Y); //Y结束坐标 
       
    LCD_WR_CMD(0x0020,X0); //X起始点
    LCD_WR_CMD(0x0021,Y0); //Y起始点
    LCD_WR_REG(0x22);
}

void ini(void)
{
  LCD_WR_CMD(229,0x8000); /* Set the internal vcore voltage */
  LCD_WR_CMD(0,  0x0001); /* Start internal OSC. */
  LCD_WR_CMD(1,  0x0000); /* set SS and SM bit */
  LCD_WR_CMD(2,  0x0700); /* set 1 line inversion */
  LCD_WR_CMD(3,  0x1030); /* set GRAM write direction and BGR=1. */
  LCD_WR_CMD(4,  0x0000); /* Resize register */
  LCD_WR_CMD(8,  0x0202); /* set the back porch and front porch */
  LCD_WR_CMD(9,  0x0000); /* set non-display area refresh cycle ISC[3:0] */
  LCD_WR_CMD(10, 0x0000); /* FMARK function */
  LCD_WR_CMD(12, 0x0000); /* RGB interface setting */
  LCD_WR_CMD(13, 0x0000); /* Frame marker Position */
  LCD_WR_CMD(15, 0x0000); /* RGB interface polarity */

/* Power On sequence ---------------------------------------------------------*/
  LCD_WR_CMD(16, 0x0000); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
  LCD_WR_CMD(17, 0x0000); /* DC1[2:0], DC0[2:0], VC[2:0] */
  LCD_WR_CMD(18, 0x0000); /* VREG1OUT voltage */
  LCD_WR_CMD(19, 0x0000); /* VDV[4:0] for VCOM amplitude */
  delay_ms(200);                 /* Dis-charge capacitor power voltage (200ms) */
  LCD_WR_CMD(16, 0x17B0); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
  LCD_WR_CMD(17, 0x0137); /* DC1[2:0], DC0[2:0], VC[2:0] */
  delay_ms(5);                  /* Delay 50 ms */
  LCD_WR_CMD(18, 0x0139); /* VREG1OUT voltage */
  delay_ms(5);                  /* Delay 50 ms */
  LCD_WR_CMD(19, 0x1d00); /* VDV[4:0] for VCOM amplitude */
  LCD_WR_CMD(41, 0x0013); /* VCM[4:0] for VCOMH */
  delay_ms(5);                  /* Delay 50 ms */
  LCD_WR_CMD(32, 0x0000); /* GRAM horizontal Address */
  LCD_WR_CMD(33, 0x0000); /* GRAM Vertical Address */

/* Adjust the Gamma Curve ----------------------------------------------------*/
  LCD_WR_CMD(48, 0x0006);
  LCD_WR_CMD(49, 0x0101);
  LCD_WR_CMD(50, 0x0003);
  LCD_WR_CMD(53, 0x0106);
  LCD_WR_CMD(54, 0x0b02);
  LCD_WR_CMD(55, 0x0302);
  LCD_WR_CMD(56, 0x0707);
  LCD_WR_CMD(57, 0x0007);
  LCD_WR_CMD(60, 0x0600);
  LCD_WR_CMD(61, 0x020b);
  
/* Set GRAM area -------------------------------------------------------------*/
  LCD_WR_CMD(80, 0x0000); /* Horizontal GRAM Start Address */
  LCD_WR_CMD(81, 0x00EF); /* Horizontal GRAM End Address */
  LCD_WR_CMD(82, 0x0000); /* Vertical GRAM Start Address */
  LCD_WR_CMD(83, 0x013F); /* Vertical GRAM End Address */

  LCD_WR_CMD(96,  0x2700); /* Gate Scan Line */
  LCD_WR_CMD(97,  0x0001); /* NDL,VLE, REV */
  LCD_WR_CMD(106, 0x0000); /* set scrolling line */

/* Partial Display Control ---------------------------------------------------*/
  LCD_WR_CMD(128, 0x0000);
  LCD_WR_CMD(129, 0x0000);
  LCD_WR_CMD(130, 0x0000);
  LCD_WR_CMD(131, 0x0000);
  LCD_WR_CMD(132, 0x0000);
  LCD_WR_CMD(133, 0x0000);

/* Panel Control -------------------------------------------------------------*/
  LCD_WR_CMD(144, 0x0010);
  LCD_WR_CMD(146, 0x0000);
  LCD_WR_CMD(147, 0x0003);
  LCD_WR_CMD(149, 0x0110);
  LCD_WR_CMD(151, 0x0000);
  LCD_WR_CMD(152, 0x0000);

  LCD_WR_CMD(3, 0x1018);

  LCD_WR_CMD(7, 0x0173); /* 262K color and display ON */  

}



/****************************************************************************
* 名    称：u16 LCD_ReadPoint(u16 x,u16 y)
* 功    能：获取指定座标的颜色值
* 入口参数：x      行座标
*           y      列座标
* 出口参数：当前座标颜色值
* 说    明：
* 调用方法：i=u16 LCD_ReadPoint(10,10);
****************************************************************************/
u16  LCD_ReadPoint(u16 x,u16 y)
{ 
   u16 temp;
   LCD_WR_CMD(0x20, x);
   LCD_WR_CMD(0x21, y);//设置光标位置
   LCD_WR_REG(34);
   temp = ili9320_BGR2RGB(ili9320_ReadData());
   return temp;
}

/****************************************************************************
* 名    称：u16 ili9320_BGR2RGB(u16 c)
* 功    能：RRRRRGGGGGGBBBBB 改为 BBBBBGGGGGGRRRRR 格式
* 入口参数：c      BRG 颜色值
* 出口参数：RGB 颜色值
* 说    明：内部函数调用
* 调用方法：
****************************************************************************/
u16 ili9320_BGR2RGB(u16 c)
{
  u16  r, g, b;
  u16 temp;
  b = (c>>0)  & 0x1f;
  g = (c>>5)  & 0x3f;
  r = (c>>11) & 0x1f;
  temp = (b<<11) + (g<<5) + (r<<0);
  return temp;
}

/****************************************************************************
* 名    称：u16 ili9320_ReadData(void)
* 功    能：读取控制器数据
* 入口参数：无
* 出口参数：返回读取到的数据
* 说    明：内部函数
* 调用方法：i=ili9320_ReadData();
****************************************************************************/
unsigned int ili9320_ReadData(void)
{
  u16 val=0;
  val=LCD_RD_data();
  return val;
}

unsigned int LCD_RD_data(void)
{
	unsigned int a=0;
	
	a=*(volatile u16 *) (Bank1_LCD_D); //L

	return(a);	
}

//画点
//x:0~239
//y:0~319
//COLOR:此点的颜色
void LCD_DrawPoint(u16 x,u16 y,u16 color)
{
    LCD_WR_CMD(0x20, x);
    LCD_WR_CMD(0x21, y);//设置光标位置
	 
	LCD_WR_REG(0x22);//开始写入GRAM
	LCD_WR_Data(color); 	    
} 

/**********************************************************
Bresenham画直线
x1，y1为起始坐标
x2，y3为终点坐标
c为颜色
**********************************************************/
int BresenhamLine( u16 x1 , u16 y1 , u16 x2 , u16 y2 , u16 c)
{
  u16 dx , dy ;
  int tx , ty ;
  int inc1 , inc2 ;
  int d , iTag ;
  int x , y ;

  LCD_DrawPoint(x1,y1,c);
  
  if ( x1 == x2 && y1 == y2 )  /*如果两点重合，结束后面的动作。*/
    return 1 ;
  
  iTag = 0 ;
  
   if(x2>=x1)
     dx = x2 - x1;
   else
     dx = x1 - x2;
  
   if(y2>=x1)
     dy = y2 - y1;
   else
     dy = y1 - y2;
   
  if ( dx < dy )   /*如果dy为计长方向，则交换纵横坐标。*/
  {
    iTag = 1 ;
    Swap ( & x1 , & y1 );
    Swap ( & x2 , & y2 );
    Swap ( & dx , & dy );
  }
  
   tx = ( x2 - x1 ) > 0 ? 1 : -1 ;    /*确定是增1还是减1*/
   ty = ( y2 - y1 ) > 0 ? 1 : -1 ;
   x = x1 ;
   y = y1 ;
   inc1 = dy<<1 ;
   inc2 = ( dy - dx )<<1;
   d = inc1 - dx ;
   
  while ( x != x2 )     /*循环画点*/
  {
    if ( d < 0 )
         d += inc1 ;
    else
    {
      y += ty ;
      d += inc2 ;
    }
    
    if ( iTag )
       LCD_DrawPoint( y , x , c ) ;
    else
       LCD_DrawPoint( x , y , c ) ;
   
   x += tx ;
  }
  
   return 0;
}

void Swap( u16 * a , u16 * b )   /*交换*/
{
  u16 tmp ;
  tmp = * a ;
  * a = * b ;
  * b = tmp ;
}



