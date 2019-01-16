
#include "Uart_Drv.h"
#include "stm32f10x.h"
#include "GlobalDef.h"
#include "os_global.h"
#include "PM25Sensor.h"

#ifdef USE_STD_LIB
void USART1_Init(uint32_t baudrate)
{
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    //RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	
     //USART1_TX   PA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //USART1_RX	  PA.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);  
	
    //Usart1 NVIC 配置
    /* Configure the NVIC Preemption Priority Bits */  
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
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
    USART_Init(USART1, &USART_InitStructure);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);  // 接收中断使能
    //USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
    USART_Cmd(USART1, ENABLE);     
}

void USART2_Init(uint32_t baudrate)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
	
	/* config USART2 clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	/* USART2 GPIO config */
    /* Configure USART2 Tx (PA.02) as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Configure USART2 Rx (PA.03) as input floating */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	//Usart NVIC 配置
    /* Configure the NVIC Preemption Priority Bits */  
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
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
	USART_Init(USART2, &USART_InitStructure); 

	
	//USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);  
	//USART_ITConfig(USART2, USART_IT_TXE, ENABLE); 
    
	USART_Cmd(USART2, ENABLE);
}

void USART3_Init(uint32_t baudrate)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
	
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	
	/* USART  GPIO config */
    /* Configure USART3  Tx  as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
	
    /* Configure USART3 Rx  as input floating */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

	//Usart NVIC 配置
    /* Configure the NVIC Preemption Priority Bits */  
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
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
	USART_InitStructure.USART_Mode = USART_Mode_Rx; //| USART_Mode_Tx;
	USART_Init(USART3, &USART_InitStructure); 
	
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);  
	//USART_ITConfig(USART3, USART_IT_TXE, ENABLE); 

	USART_Cmd(USART3, ENABLE);
}
#else
// 默认: 一个起始位，8个数据位，n个停止位, 无校验位, 无硬件流控制
// USART2, 3, 4, 5: PCLK1, 这里为 24MHz; USART1: PCLK2: 48MHz
void USART1_Init(uint32_t freq, uint32_t baudrate)
{
       RCC->APB2RSTR |=  RCC_APB2RSTR_USART1RST;		            //串口1复位
	RCC->APB2RSTR &=~ RCC_APB2RSTR_USART1RST;		            //串口1停止复位

	SET_REG_32_BIT(RCC->APB2ENR, RCC_APB2Periph_GPIOA);		// 使能 GPIOA 时钟
	SET_REG_32_BIT(RCC->APB2ENR, RCC_APB2Periph_USART1);     // 串口1时钟使能
	GPIOA->CRH = (GPIOA->CRH & 0xFFFFF00F) + 0x000004B0;		// PA9: TX, 复用推挽输出; PA10: RX, 浮空输入					    

	//USART1->BRR  = FREQ_48MHz / baudrate;  //波特率配置为BaudRate
	USART1->BRR  = freq / baudrate;  //波特率配置为BaudRate
	
	USART1->CR1 |= (USART_CR1_UE | USART_Mode_Rx | USART_Mode_Tx);
	USART1->CR1 |= USART_CR1_RXNEIE;	    // 接收中断使能
	//USART1->CR1 |= USART_CR1_TXEIE;     // 发送空中断使能
	
	STM32_NVICInit(USART1_IRQn, 3, 5, 0);  // 第3组优先级, 3位抢占优先级, 1位响应优先级
}

void USART2_Init(uint32_t freq, uint32_t baudrate)
{
       RCC->APB1RSTR |=  RCC_APB1RSTR_USART2RST;		            // 串口2复位
	RCC->APB1RSTR &=~ RCC_APB1RSTR_USART2RST;		            // 串口2停止复位
	
	SET_REG_32_BIT(RCC->APB2ENR, RCC_APB2Periph_GPIOA);		//使能 GPIOA 时钟
	SET_REG_32_BIT(RCC->APB1ENR, RCC_APB1Periph_USART2);		//串口2时钟使能
	GPIOA->CRL = (GPIOA->CRL & 0xFFFF00FF) + 0x00004B00;		//PA2: TX, 复用推挽输出; PA3: RX, 浮空输入

	USART2->BRR  = freq / baudrate;  //波特率配置为BaudRate
	
	//USART2->CR1 |= (USART_CR1_UE | USART_Mode_Rx | USART_Mode_Tx);
	USART2->CR1 |= (USART_CR1_UE | USART_Mode_Tx);
	
	//USART2->CR1 |= USART_CR1_RXNEIE;	// 接收中断使能
	//USART2->CR1 |= USART_CR1_TXEIE;     // 发送空中断使能
	
	STM32_NVICInit(USART2_IRQn, 3, 5, 1);   // 第3组优先级, 3位抢占优先级, 1位响应优先级
}

void USART3_Init(uint32_t freq, uint32_t baudrate)
{
       RCC->APB1RSTR |=  RCC_APB1RSTR_USART3RST;		            //串口3复位
	RCC->APB1RSTR &=~ RCC_APB1RSTR_USART3RST;		            //串口3停止复位
	
	SET_REG_32_BIT(RCC->APB2ENR, RCC_APB2Periph_GPIOB);       //使能 GPIOB 时钟
	SET_REG_32_BIT(RCC->APB1ENR, RCC_APB1Periph_USART3);		//串口3时钟使能
	GPIOB->CRH = (GPIOB->CRH & 0xFFFF00FF) + 0x00004B00;		//PB10: TX, 复用推挽输出; PB11: RX, 浮空输入	
	
	//USART3->BRR  = FREQ_24MHz / baudrate;  //波特率配置为BaudRate
	USART3->BRR  = freq / baudrate;  //波特率配置为BaudRate
	
	USART3->CR1 |= (USART_CR1_UE | USART_Mode_Rx | USART_Mode_Tx);
	USART3->CR1 |= USART_CR1_RXNEIE;	   // 接收中断使能
	//USARTx->CR1 |= USART_CR1_TXEIE;     // 发送空中断使能
	
	STM32_NVICInit(USART3_IRQn, 3, 5, 1);  // 第3组优先级, 3位抢占优先级, 1位响应优先级
}
#endif


#if 1 //DEBUG_VERSION
#include <stdio.h>
#include "uart_queue.h"
//FILE __stdout;  

int fputc(int ch, FILE *f)
{

  #if 0
  
  QUEUE_UART->DR = (ch & (uint16_t)0x01FF);
  while (!(QUEUE_UART->SR & USART_FLAG_TXE));

  #endif
  
  return 0;
}

#else
void os_print(char * s)
{
     while(*s)
     {
        QUEUE_UART->DR = ((*s++) & (uint16_t)0x01FF);
        while (!(QUEUE_UART->SR & USART_FLAG_TXE));
     }
}
#endif













