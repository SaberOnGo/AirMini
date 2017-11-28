
#include "usart.h"
#include "stm32f10x.h"



/*!< At this stage the microcontroller clock setting is already configured, 
	  this is done through SystemInit() function which is called from startup
	  file (startup_stm32f10x_xx.s) before to branch to application main.
	  To reconfigure the default setting of SystemInit() function, refer to
	  system_stm32f10x.c file
	*/	     
 /* USARTx configured as follow:
	   - BaudRate = 115200 baud  
	   - Word Length = 8 Bits
	   - One Stop Bit
	   - No parity
	   - Hardware flow control disabled (RTS and CTS signals)
	   - Receive and transmit enabled
  */
void USART_WIFI_Init(uint32_t baudrate)
{
   
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
  
    RCC_WIFI_APBPeriphClockCmd_Init();
	
     //USART1_TX   PA.9
    GPIO_InitStructure.GPIO_Pin = USART_WIFI_TxPin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(USART_WIFI_GPIO, &GPIO_InitStructure);
   
    //USART1_RX	  PA.10
    GPIO_InitStructure.GPIO_Pin = USART_WIFI_RxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(USART_WIFI_GPIO, &GPIO_InitStructure);  

    //Usart1 NVIC 配置
    /* Configure the NVIC Preemption Priority Bits */  
    NVIC_InitStructure.NVIC_IRQChannel = USART_WIFI_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		

	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	

    // 串口外设初始化
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART_WIFI, &USART_InitStructure);
	
    //USART_ITConfig(USART_WIFI, USART_IT_RXNE, ENABLE);  // 暂时不使用中断
    //USART_ITConfig(USART_WIFI, USART_IT_TXE, ENABLE);
    
    USART_Cmd(USART_WIFI, ENABLE);                    
}



#include <stdio.h>
//FILE __stdout;  

int fputc(int ch, FILE *f)
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART */
  USART_SendData(USART_WIFI, (uint8_t)ch);

  /* Loop until the end of transmission */
 // while (USART_GetFlagStatus(USART_WIFI, USART_FLAG_TC) == RESET);
  while (!(USART_WIFI->SR & USART_FLAG_TXE));


  return ch;
}

void USART_PM25_Init(uint32_t baudrate)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
	
	/* config USART2 clock */
    RCC_PM25_APBPeriphClockCmd_Init();
	
	/* USART2 GPIO config */
    /* Configure USART2 Tx (PA.02) as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = USART_PM25_TxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(USART_PM25_GPIO, &GPIO_InitStructure);
	    
    /* Configure USART2 Rx (PA.03) as input floating */
    GPIO_InitStructure.GPIO_Pin = USART_PM25_RxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(USART_PM25_GPIO, &GPIO_InitStructure);

	//Usart NVIC 配置
    /* Configure the NVIC Preemption Priority Bits */  
    NVIC_InitStructure.NVIC_IRQChannel = USART_PM25_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		

	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	
	
	/* USART2 mode config */
	USART_InitStructure.USART_BaudRate = baudrate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART_PM25, &USART_InitStructure); 
    USART_Cmd(USART_PM25, ENABLE);

	USART_ITConfig(USART_PM25, USART_IT_RXNE, ENABLE);  
	USART_ITConfig(USART_PM25, USART_IT_TXE, ENABLE); 
}

void USART_Radio_Init(uint32_t baudrate)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
	
    RCC_Radio_APBPeriphClockCmd_Init();
	
	/* USART  GPIO config */
    /* Configure USART  Tx  as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = USART_Radio_TxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(USART_Radio_GPIO, &GPIO_InitStructure);
	    
    /* Configure USART Rx  as input floating */
    GPIO_InitStructure.GPIO_Pin = USART_Radio_RxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(USART_Radio_GPIO, &GPIO_InitStructure);

	//Usart NVIC 配置
    /* Configure the NVIC Preemption Priority Bits */  
    NVIC_InitStructure.NVIC_IRQChannel = USART_PM25_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		

	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	

	
	/* USART mode config */
	USART_InitStructure.USART_BaudRate = baudrate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART_Radio, &USART_InitStructure); 
    USART_Cmd(USART_Radio, ENABLE);

	USART_ITConfig(USART_Radio, USART_IT_RXNE, ENABLE);  
	USART_ITConfig(USART_Radio, USART_IT_TXE, ENABLE); 
}

// ISR
void USART_PM25_ISR(void)
{
  if(USART_GetITStatus(USART_PM25, USART_IT_RXNE))  // 接收非空
  {
    /* Read one byte from the receive data register */
    RxBuffer1[RxCounter1++] = USART_ReceiveData(USART_PM25);

    if(RxCounter1 == NbrOfDataToRead1)
    {
       /* Disable the USARTy Receive interrupt */
       USART_ITConfig(USART_PM25, USART_IT_RXNE, DISABLE);
    }
  }
  
  if(USART_GetITStatus(USART_PM25, USART_IT_TXE))  // 发送寄存器空
  {   
    /* Write one byte to the transmit data register */
    USART_SendData(USARTy, TxBuffer1[TxCounter1++]);

    if(TxCounter1 == NbrOfDataToTransfer1)
    {
      /* Disable the USARTy Transmit interrupt */
      USART_ITConfig(USART_PM25, USART_IT_TXE, DISABLE);
    }    
  }
}


