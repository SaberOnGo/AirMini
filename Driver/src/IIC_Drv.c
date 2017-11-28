
#include "board_version.h"
#include "IIC_Drv.h"

#include "delay.h"
//#include "nos_api.h"

#if OS_SEM_EN
//static nos_sem_t *semIIC = NULL;
#endif

//初始化IIC
void IIC_Init(void)
{					     
	GPIO_InitTypeDef GPIO_InitStructure;
	
	IIC_APBPeriphClockCmdEnable();  
	   
	GPIO_InitStructure.GPIO_Pin   = IIC_SDA_Pin | IIC_SCL_Pin;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP ;   //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	STM32_GPIOInit(IIC_BUS_PORT, &GPIO_InitStructure);

    IIC_SCL_H();
    IIC_SDA_H();

	//if(nos_sem_init(&semIIC, "semIIC", 1, NOS_IPC_FLAG_FIFO) != NOS_OK) // 初始化信号量
	//{
	//   os_printf("create semIIC failed, %s, %d\r\n", __FILE__, __LINE__);
	//}
}

// 设置数据口为输入或输出
static void SDA_DIR(E_IO_DIR dir)
{
    GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = IIC_SDA_Pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	if(dir == INPUT)GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    else GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

	STM32_GPIOInit(IIC_SDA_PORT, &GPIO_InitStructure);
}


//产生IIC起始信号
SYS_RESULT i2c_start(void)
{
	SDA_DIR(OUTPUT);  	 
	IIC_SDA_H();
	IIC_SCL_H();
	delay_us(1);  // 4
	if(! IIC_SDA_READ())return SYS_FAILED;	//SDA线为低电平则总线忙,退出
	
 	IIC_SDA_L();  // START:when CLK is high,DATA change form high to low 
	delay_us(1);  // 4

	IIC_SCL_L();  //钳住I2C总线，准备发送或接收数据 
    
    return SYS_SUCCESS;
}	  


//产生IIC停止信号
void i2c_stop(void)
{
	SDA_DIR(OUTPUT);   // sda线输出
	IIC_SCL_L();
	IIC_SDA_L();    // STOP:when CLK is high DATA change form low to high
 	delay_us(1);    //  4 
	IIC_SCL_H();
	IIC_SDA_H();    // 发送I2C总线结束信号
	delay_us(1);	 //  4						   	
}

//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
SYS_RESULT i2c_wait_ack(void)
{
	uint16_t errCount = 0;     

	IIC_SCL_L();
	SDA_DIR(INPUT);  //SDA设置为输入  
	IIC_SDA_H(); delay_us(1);	   // 5 
	IIC_SCL_H(); delay_us(1);	   // 5
	while(IIC_SDA_READ())
	{
		errCount++;
		if(errCount > 250)  // 250
		{
			i2c_stop();
			return SYS_FAILED;
		}
	}
	IIC_SCL_L();   // 时钟输出0 	   
	return SYS_SUCCESS;  
}

//产生ACK应答
void i2c_ack(void)
{
	IIC_SCL_L();
	SDA_DIR(OUTPUT);
	IIC_SDA_L();
	delay_us(1);  // 2
	IIC_SCL_H();
	delay_us(1);  // 2
	IIC_SCL_L();
}

//不产生ACK应答		    
void i2c_nack(void)
{
	IIC_SCL_L();
	SDA_DIR(OUTPUT);
	IIC_SDA_H();
	delay_us(1);  // 2
	IIC_SCL_H();
	delay_us(1);  // 2
	IIC_SCL_L();
}			

//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答			  
void i2c_send_byte(uint8_t txd)
{                        
    uint8_t i;    	   
	
	SDA_DIR(OUTPUT);
    IIC_SCL_L(); //拉低时钟开始数据传输
    for(i = 0; i < 8; i++)
    {              
		if(txd & 0x80)IIC_SDA_H();
		else { IIC_SDA_L(); }
        txd <<= 1; 	  
		delay_us(1);   // 1
		IIC_SCL_H();
		delay_us(1);  //  1
		IIC_SCL_L();
		delay_us(1);  // 1
    }	 
} 	    

//读1个字节，ack = 1时，发送ACK，ack = 0，发送nACK   
uint8_t i2c_read_byte(uint8_t ack)
{
	uint8_t  i, receive = 0;
	
	SDA_DIR(INPUT); //SDA设置为输入
    for(i = 0; i < 8; i++ )
	{
		IIC_SCL_L();
        delay_us(1);  // 2
		IIC_SCL_H();
        receive <<= 1;
        if(IIC_SDA_READ())receive++;   
		delay_us(1); 
    }	
	IIC_SCL_L();
    if (! ack)
        i2c_nack(); //发送nACK
    else
        i2c_ack();  //发送ACK   
    return receive;
}

/***********************************************
说明: I2C 启动写寄存器命令
参数: uint8_t sla_addr     I2C 丛机地址
             uint8_t data_addr   寄存器地址
返回值: 启动成功: SYS_SUCCESS;   失败: SYS_FAILED
************************************************/
SYS_RESULT i2c_start_write(uint8_t sla_addr, uint16_t data_addr)
{
    if(i2c_start())return SYS_FAILED;
		
    i2c_send_byte(sla_addr | IIC_WRITE);
	if(i2c_wait_ack()){ INSERT_ERROR_INFO(0); }

    // 写寄存器地址
    #if 0
	i2c_send_byte((data_addr >> 8)); 
	if(i2c_wait_ack()){ INSERT_ERROR_INFO(0); return SYS_FAILED; }
	#endif
	
	i2c_send_byte((uint8_t)data_addr);
	if(i2c_wait_ack()){ INSERT_ERROR_INFO(0); return SYS_FAILED; }
    
	
    return SYS_SUCCESS;
}

SYS_RESULT IIC_WriteNByte_Raw(uint8_t sla_addr, uint16_t data_addr, uint8_t * pdata, uint8_t size)
{  	
    uint8_t *p = (uint8_t *)pdata;
    register uint8_t i;
	
    if(i2c_start_write(sla_addr, data_addr)){ INSERT_ERROR_INFO(0); return SYS_FAILED; }  	

	for(i = 0; i<size; i++, p++)
    {
        i2c_send_byte(*p);			//写数据
        if(i2c_wait_ack()){ INSERT_ERROR_INFO(0); return SYS_FAILED; }
    }
    i2c_stop();						//发送STOP 信号
    return SYS_SUCCESS;
}

SYS_RESULT IIC_WriteNByte(uint8_t sla_addr, uint16_t data_addr, uint8_t * pdata, uint8_t size)
{
   SYS_RESULT res;

  // nos_sem_take_wait_forever(semIIC);
   //OSSchedLock();
   res = IIC_WriteNByte_Raw(sla_addr, data_addr, pdata, size);
   //OSSchedUnlock();
   //nos_sem_release(semIIC);
    
   return res;
}

/*******************************
功能说明: 读取N个字节
参数: uint8_t sla_addr: I2C设备从机地址
             uint16_t data_addr: 寄存器地址
             uint8_t * pdata: 读取的数据起始指针
             uint8_t   size: 读取的数据长度, 输入时是要读取的数据长度. 
********************************/
SYS_RESULT IIC_ReadNByte_Raw(uint8_t sla_addr, uint16_t data_addr, uint8_t * pdata, uint8_t size, uint8_t restart_iic)
{
    uint8_t *p = pdata;

    if(i2c_start_write(sla_addr, data_addr)){ INSERT_ERROR_INFO(0); return SYS_FAILED; }

    if(restart_iic){ i2c_start(); }
	
    i2c_send_byte(sla_addr  | IIC_READ); //读模式
    if(i2c_wait_ack()){ INSERT_ERROR_INFO(0); return SYS_FAILED; }

    for(; size > 1; size--, p++)	//注意这里必须是size > 1,因为下面还会再读取一个
    {
        *p = i2c_read_byte(1);  //读取数据
    }
    *p = i2c_read_byte(0);
    i2c_stop();
	return SYS_SUCCESS;
}

SYS_RESULT IIC_ReadNByteExt(uint8_t sla_addr, uint16_t data_addr, uint8_t * pdata, uint8_t size, uint8_t restart_iic)
{
   SYS_RESULT res;
   
   //nos_sem_take_wait_forever(semIIC);
   //OSSchedLock();
   res = IIC_ReadNByte_Raw(sla_addr, data_addr, pdata, size, restart_iic);
   //OSSchedUnlock();
  // nos_sem_release(semIIC);

   return res;
}

// 标准IIC
SYS_RESULT IIC_ReadNByte(uint8_t sla_addr, uint16_t data_addr, uint8_t * pdata, uint8_t size)
{
    return IIC_ReadNByteExt(sla_addr, data_addr, pdata, size, 0);
}

/************************************************
功能: 直接读N个字节, 不需要指定寄存器地址
参数: 
*************************************************/
SYS_RESULT IIC_ReadNByteDirectly_Raw(uint8_t sla_addr, uint8_t * pdata, uint8_t size)
{
    uint8_t *p = pdata;

    if(i2c_start()){ INSERT_ERROR_INFO(0); return SYS_FAILED; }
	
    i2c_send_byte(sla_addr | IIC_READ); //读模式
    if(i2c_wait_ack()){ INSERT_ERROR_INFO(0); return SYS_FAILED; }

    for(; size > 1; size--, p++)	//注意这里必须是size > 1,因为下面还会再读取一个
    {
        *p = i2c_read_byte(1);  //读取数据
    }
    *p = i2c_read_byte(0);
    i2c_stop();

	return SYS_SUCCESS;
}

SYS_RESULT IIC_ReadNByteDirectly(uint8_t sla_addr, uint8_t * pdata, uint8_t size)
{
   SYS_RESULT res;

   //nos_sem_take_wait_forever(semIIC);
   //OSSchedLock();
   res = IIC_ReadNByteDirectly_Raw(sla_addr, pdata, size);
   //OSSchedUnlock();
   //nos_sem_release(semIIC);

   return res;
}

