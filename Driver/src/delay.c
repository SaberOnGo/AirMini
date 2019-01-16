
#include "stm32f10x.h"
#include <stm32f10x_tim.h>
#include <stm32f10x_rcc.h>
#include <stdint.h>
#include "delay.h"
#include "GlobalDef.h"

#if 0
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//Mini STM32������
//ʹ��SysTick����ͨ����ģʽ���ӳٽ��й���
//����delay_us,delay_ms
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2010/5/27
//�汾��V1.2
//��Ȩ���У�����ؾ���
//Copyright(C) ����ԭ�� 2009-2019
//All rights reserved
//********************************************************************************
//V1.2�޸�˵��
//�������ж��е��ó�����ѭ���Ĵ���
//��ֹ��ʱ��׼ȷ,����do while�ṹ!
//////////////////////////////////////////////////////////////////////////////////	 
static u8  fac_us=0;//us��ʱ������
static u16 fac_ms=0;//ms��ʱ������
//��ʼ���ӳٺ���
//SYSTICK��ʱ�ӹ̶�ΪHCLKʱ�ӵ�1/8
//SYSCLK:ϵͳʱ��
void delay_init(u8 SYSCLK)
{
	//SysTick->CTRL&=0xfffffffb;//bit2���,ѡ���ⲿʱ��  HCLK/8
	fac_us=SYSCLK/8;		    
	fac_ms=(u16)fac_us*1000;
}								    
//��ʱnms
//ע��nms�ķ�Χ
//SysTick->LOADΪ24λ�Ĵ���,����,�����ʱΪ:
//nms<=0xffffff*8*1000/SYSCLK
//SYSCLK��λΪHz,nms��λΪms
//��72M������,nms<=1864 
void delay_ms(u16 nms)
{	 		  	  
	u32 temp;		   
	SysTick->LOAD=(u32)nms*fac_ms;//ʱ�����(SysTick->LOADΪ24bit)
	SysTick->VAL =0x00;           //��ռ�����
	SysTick->CTRL=0x01 ;          //��ʼ����  
	do
	{
		temp=SysTick->CTRL;
	}
	while(temp&0x01&&!(temp&(1<<16)));//�ȴ�ʱ�䵽��   
	SysTick->CTRL=0x00;       //�رռ�����
	SysTick->VAL =0X00;       //��ռ�����	  	    
}   
//��ʱnus
//nusΪҪ��ʱ��us��.		    								   
void delay_us(u32 nus)
{		
	u32 temp;	    	 
	SysTick->LOAD=nus*fac_us; //ʱ�����	  		 
	SysTick->VAL=0x00;        //��ռ�����
	SysTick->CTRL=0x01 ;      //��ʼ���� 	 
	do
	{
		temp=SysTick->CTRL;
	}
	while(temp&0x01&&!(temp&(1<<16)));//�ȴ�ʱ�䵽��   
	SysTick->CTRL=0x00;       //�رռ�����
	SysTick->VAL =0X00;       //��ռ�����	 
}


#else

#define DELAY_TIM_FREQUENCY_US 1000000		/* = 1MHZ -> timer runs in microseconds */
#define DELAY_TIM_FREQUENCY_MS 1000			/* = 1kHZ -> timer runs in milliseconds */

#define TIM2_TimeBaseInit(TIMx, TIM_TimeBaseInitStruct) {\
	TIMx->CR1 &= (uint16_t)(~((uint16_t)(TIM_CR1_DIR | TIM_CR1_CMS)));\
	TIMx->CR1 |= (uint32_t)TIM_TimeBaseInitStruct.TIM_CounterMode;\
	TIMx->ARR = TIM_TimeBaseInitStruct.TIM_Period;\
	TIMx->PSC = TIM_TimeBaseInitStruct.TIM_Prescaler;\
	TIMx->EGR = TIM_PSCReloadMode_Immediate;\
	}

// Init timer for Microseconds delays
#if 0
void _init_us(void) 
{
	TIM_TimeBaseInitTypeDef TIM;

	// Enable clock for TIM2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);

	// Time base configuration
	TIM_TimeBaseStructInit(&TIM);
	TIM.TIM_Prescaler = (SystemCoreClock/DELAY_TIM_FREQUENCY_US)-1;
	TIM.TIM_Period = UINT16_MAX;
	TIM.TIM_ClockDivision = 0;
	TIM.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2,&TIM);

	// Enable counter for TIM2
	TIM_Cmd(TIM2,ENABLE);
}
#else

#if 1
#define _init_us() {\
	SET_REG_32_BIT(RCC->APB1ENR, RCC_APB1Periph_TIM2);\
	TIM2->PSC  = (SystemCoreClock / DELAY_TIM_FREQUENCY_US) - 1;\
	TIM2->ARR  = UINT16_MAX;\
	TIM2->EGR  = TIM_PSCReloadMode_Immediate;\
	TIM2->CR1 |= TIM_CR1_CEN;\
	}
#else
#define _init_us() {\
	TIM_TimeBaseInitTypeDef TIM;\
	SET_REG_32_BIT(RCC->APB1ENR, RCC_APB1Periph_TIM2);\
	TIM.TIM_Prescaler = (SystemCoreClock / DELAY_TIM_FREQUENCY_US) - 1;\
	TIM.TIM_Period = UINT16_MAX;\
	TIM.TIM_ClockDivision = 0;\
	TIM.TIM_CounterMode = TIM_CounterMode_Up;\
	TIM2_TimeBaseInit(TIM2, TIM);\
	SET_REG_32_BIT(TIM2->CR1, TIM_CR1_CEN);\
	}
#endif

#endif

// Init and start timer for Milliseconds delays
#if 0
void _init_ms(void) 
{

    TIM_TimeBaseInitTypeDef TIM;
	
	// Enable clock for TIM2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);

	// Time base configuration
	
	TIM_TimeBaseStructInit(&TIM);
	TIM.TIM_Prescaler = (SystemCoreClock/DELAY_TIM_FREQUENCY_MS)-1;
	TIM.TIM_Period = UINT16_MAX;
	TIM.TIM_ClockDivision = 0;
	TIM.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2,&TIM);

	// Enable counter for TIM2
	TIM_Cmd(TIM2,ENABLE);
}
#else
#define _init_ms() {\
	SET_REG_32_BIT(RCC->APB1ENR, RCC_APB1Periph_TIM2);\
	TIM2->PSC  = (SystemCoreClock / DELAY_TIM_FREQUENCY_MS) - 1;\
	TIM2->ARR  = UINT16_MAX;\
	TIM2->EGR  = TIM_PSCReloadMode_Immediate;\
	TIM2->CR1 |= TIM_CR1_CEN;\
	}
#endif

// Stop timer
#if 0
void _stop_timer(void) {
	TIM_Cmd(TIM2,DISABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,DISABLE); // Powersavings?
}
#else
#define _stop_timer() {\
	CLEAR_REG_32_BIT(TIM2->CR1, TIM_CR1_CEN);\
	CLEAR_REG_32_BIT(RCC->APB1ENR, RCC_APB1Periph_TIM2);\
	}
#endif


// Do delay for nTime milliseconds
void delay_ms(uint32_t mSecs) 
{
   volatile uint32_t start;
   
	// Init and start timer
	_init_ms();

	// Dummy loop with 16 bit count wrap around
	start = TIM2->CNT;
	while((TIM2->CNT-start) <= mSecs);

	// Stop timer
	_stop_timer();
}

// Do delay for nTime microseconds
WEAK_ATTR void delay_us(uint32_t uSecs) 
{
    volatile uint32_t start;
	
	// Init and start timer
	_init_us();

	// Dummy loop with 16 bit count wrap around
	TIM2->CNT = 0;
	start = TIM2->CNT;
	while((TIM2->CNT-start) <= uSecs);

	// Stop timer
	_stop_timer();
}

#endif































