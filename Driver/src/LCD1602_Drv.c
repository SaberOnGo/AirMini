
#include "stm32f10x.h"
#include "LCD1602_Drv.h"
#include "os_global.h"
#include "Delay.h"
#include "GlobalDef.h"
#include "board_version.h"


  



// 设置数据口为输入或输出
static void LCD_set_data_dir(E_IO_DIR dir)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	if(dir == INPUT)GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    else GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

    #ifdef USE_STD_LIB
	GPIO_InitStructure.GPIO_Pin = LCD_D7_Pin;
	GPIO_Init(LCD_D7_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = LCD_D6_Pin;
	GPIO_Init(LCD_D6_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = LCD_D5_Pin;
	GPIO_Init(LCD_D5_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = LCD_D4_Pin;
	GPIO_Init(LCD_D4_PORT, &GPIO_InitStructure);
	#else
    GPIO_InitStructure.GPIO_Pin = LCD_D7_Pin;
	STM32_GPIOInit(LCD_D7_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = LCD_D6_Pin;
	STM32_GPIOInit(LCD_D6_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = LCD_D5_Pin;
	STM32_GPIOInit(LCD_D5_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = LCD_D4_Pin;
	STM32_GPIOInit(LCD_D4_PORT, &GPIO_InitStructure);
	#endif
	
}

// Send strobe to LCD via EN line
static void LCD_enable_write(void)  //液晶使能
{
    LCD_EN_H();
	delay_us(15); // Due to datasheet E cycle time is about ~500ns
	LCD_EN_L();
	delay_us(15); // Due to datasheet E cycle time is about ~500ns
}


// Send low nibble of cmd to LCD via 4bit bus
// 发送低4位值
#define LCD_send_4bit(cmd) { LCD_DATA_4BIT_OUT(cmd << 4); LCD_enable_write(); }

// 硬件初始化
void LCD1602_HardwareInit(void)
{
#if MODULE_LCD_EN
    GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = LCD_RS_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

	#ifdef USE_STD_LIB
    GPIO_Init(LCD_RS_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LCD_RW_PIN;
	GPIO_Init(LCD_RW_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LCD_EN_PIN;
	GPIO_Init(LCD_EN_PORT, &GPIO_InitStructure);
    #else
    STM32_GPIOInit(LCD_RS_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LCD_RW_PIN;
	STM32_GPIOInit(LCD_RW_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LCD_EN_PIN;
	STM32_GPIOInit(LCD_EN_PORT, &GPIO_InitStructure);
	#endif
	
    LCD_set_data_dir(OUTPUT);
	
    // 打开 LCD 电源
	LCD_Ctrl_Set(SW_OPEN);

	LCD_EN_H();
	LCD_RW_L();
	LCD_RS_L();
	LCD_DATA_4BIT_H();
#endif
}

static void LCD_enable_wait(void)
{
#if 0
        uint32_t timeout = 0xFFFFFFFF;
		
        LCD_set_data_dir(INPUT);   //设置busy口为输入
        LCD_DATA_4BIT_H();
        LCD_RS_L();  //RS=0
        LCD_RW_H();  //RW=1
        
        delay_us(5);
        LCD_EN_H();  //E=1
        delay_us(5);
        
        while(LCD_D7_READ() && timeout--);          //等待LCD1602_DB7为0
#endif
        
        LCD_EN_L(); //重设E=0
        LCD_set_data_dir(OUTPUT);  //设置busy口为输出
}


// Send data to LCD via 4bit bus
void LCD1602_WriteData(uint8_t data)
{
#if MODULE_LCD_EN
    LCD_enable_wait();
	
	LCD_RS_H();
	LCD_RW_L();
    LCD_send_4bit(data >> 4);              // send high nibble
    LCD_send_4bit(data);                    // send low nibble
    LCD_RS_L();
    delay_us(65); //65, delay_us(45);                           // write data to RAM takes about 43us
#endif
}

// Send command to LCD via 4bit bus
void LCD1602_WriteCmd(uint8_t cmd)   // 写命令
{
#if MODULE_LCD_EN
    if(cmd != 0x28 && cmd != 0x38)LCD_enable_wait();
	
	LCD_RS_L();
	LCD_RW_L();
    LCD_send_4bit(cmd >> 4);  // send high nibble
    LCD_send_4bit(cmd);       // send low nibble
    delay_us(65); //65, delay_us(45);              // typical command takes about 39us
#endif
}

// 设置坐标, x: 行坐标, 0-1
// y: 列坐标: 0 - 15
void LCD1602_SetXY(uint8_t x, uint8_t y) 
{
#if MODULE_LCD_EN
    LCD1602_WriteCmd((y+ ( x << 6 )) | 0x80);  // Set DDRAM address with coordinates
#endif
}

void LCD1602_WriteString(uint8_t x, uint8_t y, const uint8_t *s)
{
#if MODULE_LCD_EN
    const uint8_t *p = s;
	
    LCD1602_SetXY( x, y );
    while (*p)
    {
       LCD1602_WriteData( *p++ );
    }
#endif
}

// 往指定坐标显示整数 val, 前面的0不显示
// uint8_t placeholder_size: 占位符个数, 有6个占位，则只能显示6个字符在LCD上
// 字符显示右对齐
void LCD1602_WriteInteger(uint8_t x, uint8_t y, uint32_t val, uint8_t placeholder_size)
{
#if MODULE_LCD_EN
    uint8_t num_string[10];  // 最多显示10个整数
    uint32_t div = 1000000000;
    uint8_t i, j, valid_size;  // valid_size 为有效显示的数字个数, 如 00123, 则valid_size = 3
		
	for(i = 0; i < 10; i++)
	{
	    num_string[i] = val / div + 0x30;
		val %= div;
		div /= 10;
	}

    // 跳过前面的连续 0
    for(i = 0; i < 10; i++)
    {
       if(num_string[i] != 0x30)break;
    }
	valid_size = 10 - i;
	if(valid_size == 0){ i = 9, valid_size = 1; }
	
	if(placeholder_size >= valid_size)
	{
	   uint8_t space_size = placeholder_size - valid_size;  // 空白字符个数
	   LCD1602_SetXY(x, y);
	   for(j = 0; j < space_size; j++)
	   {
	      LCD1602_WriteData(' ');
	   }
	   y += placeholder_size - valid_size;   // 右对齐
	}
	else  // 占位符不够, 只显示小值部分
	{
	   i += valid_size - placeholder_size; // 前面部分不显示, 如 123456, 占位符为4, 则显示 3456
	}

       LCD1602_SetXY(x, y);
	for(; i < 10; i++)
	{
	      LCD1602_WriteData(num_string[i]);
	}
#endif
}

void LCD1602_WriteInt(uint8_t x, uint8_t y, uint16_t val)
{
#if MODULE_LCD_EN
   LCD1602_SetXY(x, y);
   LCD1602_WriteData((val / 10) + 0x30);
   LCD1602_WriteData((val % 10) + 0x30);
#endif
}

// Clear LCD display and set cursor at first position
void LCD1602_ClearScreen(void) 
{
#if MODULE_LCD_EN
	LCD1602_WriteCmd(0x01);  // Clear display command
	delay_ms(2);               // Numb display does it at least 1.53ms
	LCD1602_WriteCmd(0x02);  // Return Home command
	delay_ms(2);               // Numb display does it at least 1.53ms
#endif
}

// Init LCD to 4bit bus mode
void LCD1602_Init(void) 
{
#if MODULE_LCD_EN
       LCD1602_HardwareInit();
	
	delay_ms(30);              // must wait >=30us after LCD Vdd rises to 4.5V

	LCD_send_4bit(0x03);      // select 4-bit bus (still 8bit)
	delay_ms(10);               // must wait more than 4.1ms
	LCD_send_4bit(0x03);      // select 4-bit bus (still 8bit)
	delay_us(150);             // must wait more than 100us
	LCD_send_4bit(0x03);      // select 4-bit bus (still 8bit)
	LCD_send_4bit(0x02);      // Function set: 4-bit bus (gotcha!)
       delay_ms(10);
	LCD1602_WriteCmd(0x28);       // LCD Function: 2 Lines, 5x8 matrix
	delay_ms(10);
	LCD1602_WriteCmd(0x01);       // LCD Function: 2 Lines, 5x8 matrix
	delay_ms(10);
	LCD1602_WriteCmd(0x0C);       // Display control: Display: on, cursor: off
	delay_ms(10);
	LCD1602_WriteCmd(0x06);       // Entry mode: increment, shift disabled
#endif
}

void LCD1602_Close(void) 
{
#if 0
    //LCD1602_HardwareInit();
	
	//delay_ms(30);              // must wait >=30us after LCD Vdd rises to 4.5V

	LCD_send_4bit(0x03);      // select 4-bit bus (still 8bit)
	delay_ms(10);               // must wait more than 4.1ms
	LCD_send_4bit(0x03);      // select 4-bit bus (still 8bit)
	delay_us(150);             // must wait more than 100us
	LCD_send_4bit(0x03);      // select 4-bit bus (still 8bit)
	LCD_send_4bit(0x02);      // Function set: 4-bit bus (gotcha!)
    delay_ms(10);
	LCD1602_WriteCmd(0x28);       // LCD Function: 2 Lines, 5x8 matrix
	delay_ms(10);
	LCD1602_WriteCmd(0x01);       // LCD Function: 2 Lines, 5x8 matrix
	delay_ms(10);
	LCD1602_WriteCmd(0x08);       // Display control: Display: on, cursor: off
	delay_ms(10);
	//LCD1602_WriteCmd(0x06);       // Entry mode: increment, shift disabled
#endif	
}


