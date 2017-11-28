
#include "uart_queue.h"
#include "GlobalDef.h"
#include "os_global.h"

static QueueMemType RxBuf[UART_Q_RX_BUF_SIZE] = {0};
static QueueMemType TxBuf[UART_Q_TX_BUF_SIZE] = {0};

static volatile T_UartQueue Tx_Queue, Rx_Queue;

static volatile uint16_t  is_rx_flag = 0;    // receive data or not

static volatile uint8_t TxSemaLock = UART_IDLE; //信号量

 //发送
#if 0
#define Uart_EnableTxInterrupt() USART_ITConfig(QUEUE_UART, USART_IT_TXE, ENABLE)
#else
#define Uart_EnableTxInterrupt()   SET_REG_32_BIT(QUEUE_UART->CR1, USART_CR1_TXEIE)
#endif

static void Uart_TxQueueInit(void)
{
    Tx_Queue.Base = TxBuf;
	Tx_Queue.QHead = 0;
	Tx_Queue.QTail = 0;
}
static void Uart_RxQueueInit(void)
{
    Rx_Queue.Base = RxBuf;
    Rx_Queue.QHead = 0;
	Rx_Queue.QTail = 0;
}
static void Uart_QueueInit(void)
{
    Uart_TxQueueInit();
    Uart_RxQueueInit();
}

void Uart_Q_Init(void)
{

    #if   (QUEUE_SEL == 1)
	USART1_Init(FREQ_48MHz, 115200);
	#elif (QUEUE_SEL == 2)
    USART2_Init(FREQ_24MHz, 115200);
	#elif (QUEUE_SEL == 3)
	USART3_Init(FREQ_24MHz, 115200);
    #endif

	
	Uart_QueueInit();
	
	//GLOBAL_ENABLE_IRQ();
}

/*****************************************************************************
 * @\fn  : Uart_DisableTxInterrupt
 * @\author : pi
 * @\date : 2016 - 6 - 23
 * @\brief : 禁止UART TX中断
 * @\param[in] : void  
 * @\param[out] : none
 * @\return : 
 * @\attention : 
 * @\note [others] : 清除SR寄存器的TC,TXE bit, 禁止TIEN,TCIEN,TEN

*****************************************************************************/
static void Uart_DisableTxInterrupt(void)
{
     #if 0
     CLEAR_REGISTER_BIT(HARD_USART->CR2, UART2_CR2_TIEN);  //发送中断禁止
	 CLEAR_REGISTER_BIT(HARD_USART->CR2, UART2_CR2_TCIEN);  //发送完成中断禁止
	 
     
	 CLEAR_REGISTER_BIT(HARD_USART->SR, UART2_SR_TC);  //清TC位
	 //UART1->DR = 0;   //清SR寄存器中的TXE位: 写DR寄存器, 不能加这句, 否则发送的数据前,都会发一个字节的0x00

	 //CLEAR_REGISTER_BIT(UART1->CR2, UART1_CR2_TEN); //发送禁止, 不能加这句, 否则中间的数据可能会错误
     #endif

	 #if 0
     USART_ITConfig(QUEUE_UART, USART_IT_TXE, DISABLE);
	 USART_ITConfig(QUEUE_UART, USART_IT_TC, DISABLE);
	 USART_ClearFlag(QUEUE_UART, USART_FLAG_TC);
	 #else
	 CLEAR_REG_32_BIT(QUEUE_UART->CR1, USART_CR1_TXEIE);
	 CLEAR_REG_32_BIT(QUEUE_UART->CR1, USART_CR1_TCIE);
	 CLEAR_REG_32_BIT(QUEUE_UART->SR, USART_SR_TC);
	 #endif
}


#ifndef  UART_CIRCLE_TX_QUEUE_EN
void Uart_SendByte(uint8_t data)
{
   USART_SendData(QUEUE_UART, data);
   while (!(QUEUE_UART->SR & USART_FLAG_TXE));
}

void Uart_SendDatas(uint8_t *pData, uint16_t len)
{
   uint16_t uIndex = 0;
   uint8_t *pStr = pData;

   if(NULL == pStr)
   {
      return;
   }
   for(; uIndex < len; uIndex++)
   {
       Uart_SendByte(pStr[uIndex]);
   }
}

void Uart_SendString(uint8_t *pString)
{
   uint8_t *pStr = pString;
   while(0 != *pStr)
   {
      Uart_SendByte(*pStr++);
   }
}

#else  //UART_CIRCLE_TX_QUEUE_EN
/*****************************************************************************
 * @\fn  : Uart_TxQueueIn
 * @\author : pi
 * @\date : 2016 - 6 - 22
 * @\brief : 往发送队列中写入一个数据
 * @\param[in] : 待写入的数据
 * @\param[out] : none
 * @\return : ERROR, SUCCESS
 * @\attention : 
 * @\note [others] : 内部调用

*****************************************************************************/
static UartStatus Uart_TxQueueIn(QueueMemType data)
{
   QueuePointerType QTail = Tx_Queue.QTail;
   
   if(((Tx_Queue.QHead + 1) % UART_Q_TX_BUF_SIZE) == QTail) // 缓冲区撑满
   {
      return ERROR;
   }
   Tx_Queue.Base[Tx_Queue.QHead] = data;
   
   //更新头指针，如果到了缓冲区的末端，就自动返回到缓冲区的起始处
   Tx_Queue.QHead = (Tx_Queue.QHead + 1) % UART_Q_TX_BUF_SIZE; //缓冲区回绕
   
   return SUCCESS;
}

/*****************************************************************************
 * @\fn  : Uart_TxQueueOut
 * @\author : pi
 * @\date : 2016 - 6 - 22
 * @\brief : 从发送缓冲区取一个数据, 从串口输出
 * @\param[in] : none
 * @\param[out] : none
 * @\return : ERROR, SUCCESS
 * @\attention : 
 * @\note [others] : 内部调用

*****************************************************************************/
#define  Uart_OutByte(Data)   QUEUE_UART->DR = ((Data) & (uint16_t)0x01FF)
static UartStatus Uart_TxQueueOut(void)
{
    if(Tx_Queue.QHead == Tx_Queue.QTail)  //缓冲区为空, 无发送数据
    {
       TxSemaLock = UART_IDLE; //串口空闲
       return ERROR;
    }
	TxSemaLock = UART_BUSY; //串口忙
    //从发送缓冲区取一个数据
    Uart_OutByte(Tx_Queue.Base[Tx_Queue.QTail]);
	
    // 更新尾指针的位置，如果到了缓冲区的末端，就自动返回到缓冲区的起始处
    Tx_Queue.QTail = (Tx_Queue.QTail + 1) % UART_Q_TX_BUF_SIZE; //尾指针+1, 缓冲区回绕	

    return SUCCESS;
}

/*****************************************************************************
 * @\fn  : Uart_IsTxDataBuffEmpty
 * @\author : pi
 * @\date : 2016 - 6 - 23
 * @\brief : 循环发送队列是否为空
 * @\param[in] : void  
 * @\param[out] : none
 * @\return : 队列为空: 1;  队列有数据: 0
 * @\attention : 
 * @\note [others] : 

*****************************************************************************/
uint8_t Uart_IsTxDataBuffEmpty(void)
{
   return (Tx_Queue.QHead == Tx_Queue.QTail);
}

/*****************************************************************************
 * @\fn  : Uart_SendByte
 * @\author : pi
 * @\date : 2016 - 6 - 23
 * @\brief : no
 * @\param[in] : none
 * @\param[out] : none
 * @\return : 
 * @\attention : 
 * @\note [others] : 

*****************************************************************************/
UartStatus Uart_SendByte(uint8_t data)
{
    UartStatus status = ERROR;

	if(!TxSemaLock) //串口空闲
	{
	   Uart_DisableTxInterrupt();
	}
    status = Uart_TxQueueIn(data);
	if(!TxSemaLock) //串口空闲
	{
       Uart_TxQueueOut();  //串口发送
       Uart_EnableTxInterrupt();
	}
	
	return status;
}
/*****************************************************************************
 * @\fn  : Uart_SendDatas
 * @\author : pi
 * @\date : 2016 - 6 - 22
 * @\brief : 将数据写入发送队列, 并启动发送
 * @\param[in] : uint8_t *pString  发送数据的指针
               uint16_t len      数据长度
 * @\param[out] : none
 * @\return : 发送状态
 * @\attention : 
 * @\note [others] : 外部调用

*****************************************************************************/
UartStatus Uart_SendDatas(uint8_t *pString, uint16_t len)
{
    register uint16_t uIndex = 0;
	UartStatus status = SUCCESS;
	uint8_t ReSendTimes = 100; //每个数据的重发次数
	
	if(NULL == pString)
	{
	   return ERROR;
	}
	
	if(! TxSemaLock) //串口空闲,则先禁止中断
	{
	   Uart_DisableTxInterrupt();
	}
	for(; uIndex < len; uIndex++)
	{
	    do

	    {
	       status = Uart_TxQueueIn(pString[uIndex]);  //数据进入发送缓冲区
	       NOP();
		   ReSendTimes--;
	    }while(ERROR == status && ReSendTimes);  //一次发送错误, 则重发
	    if(ERROR == status)
	    {
	       //return ERROR; //发送错误且超时
	       break;
	    }
	}
	if(!TxSemaLock) //串口空闲
	{
	   Uart_TxQueueOut();
	   Uart_EnableTxInterrupt();
	}
		
	return status;
	
}

UartStatus Uart_SendString(uint8_t *pString)
{
    uint16_t len = 0;
    len = os_strlen((void *)pString);
    return Uart_SendDatas(pString, len + 1);
}

#endif  // end UART_CIRCLE_TX_QUEUE_EN




/*****************************************************************************
 * @\fn  : Uart_CheckRxFlag
 * @\author : pi
 * @\date : 2016 - 6 - 22
 * @\brief : check uart receive data or not 
 * @\param[in] : void  void
 * @\param[out] : none
 * @\return :  1: has data;  0: no data
 * @\attention : 
 * @\note [others] : 外部调用
*****************************************************************************/
#if 1

uint16_t Uart_CheckRxFlag(void)
{
    return is_rx_flag;
}


void Uart_ClearRxFlag(void)
{
   is_rx_flag = 0;
}

/*****************************************************************************
 * @\fn  : Uart_IsRxBuffSizeEqualThan
 * @\author : pi
 * @\date : 2016 - 6 - 24
 * @\brief : 判断接收缓冲区的数据长度比compare_size 大, 小, 或等于
 * @\param[in] : uint16_t compare_size  要比较的大小值
 * @\param[out] :  
 * @\return : 比较结果: 缓冲区数据长度与参数的比较结果: More, Less, Equal
 * @\attention : 
 * @\note [others] : 

*****************************************************************************/
Compare_State Uart_IsRxBuffSizeEqualThan(uint16_t compare_size)
{
    uint16_t BuffDataLen = 0; //输出的数据长度
    QueuePointerType QHead = Rx_Queue.QHead;

	if(QHead == Rx_Queue.QTail) //缓冲区为空
	{
	   return LESS_STATE;
	}
    else if(QHead > Rx_Queue.QTail) //0  -> QTail -> QHead -> End -> 0, 头指针大于尾指针
	{
	   BuffDataLen = QHead - Rx_Queue.QTail;
	}else  //0  -> QHead -> QTail -> End -> 0
	{
	    BuffDataLen = QHead + UART_Q_RX_BUF_SIZE - Rx_Queue.QTail;
	}

	if(BuffDataLen == compare_size)
	{
	   return EQUAL_STATE;  
	}
	else if(BuffDataLen > compare_size)
	{
	   return MORE_STATE;
	}
	else
	{
	   return LESS_STATE;
	}
}

#define Uart_SetRxFlag(status) (is_rx_flag=status)

#endif	

/*****************************************************************************
 * @\fn  : Uart_RxQueueIn
 * @\author : pi
 * @\date : 2016 - 6 - 22
 * @\brief : 向接收队列存入一个数据
 * @\param[in] : QueueMemType data  no
 * @\param[out] : none
 * @\return : ERROR, SUCCESS
 * @\attention : 
 * @\note [others] : 

*****************************************************************************/
#if (PRINTF_OUT_SEL == UART_QUEUE)
static 
#endif
UartStatus Uart_RxQueueIn(QueueMemType data)
{
    if(((Rx_Queue.QHead + 1) % UART_Q_RX_BUF_SIZE) == Rx_Queue.QTail) //缓冲区撑满
	{
	   if(! is_rx_flag)
	   {
	      is_rx_flag = 1;
	   }
	   return ERROR;
	}
	Rx_Queue.Base[Rx_Queue.QHead] = data; //存入一个数据
    // 更新头指针，如果到了缓冲区的末端，就自动返回到缓冲区的起始处
    Rx_Queue.QHead = (Rx_Queue.QHead + 1) % UART_Q_RX_BUF_SIZE; //头指针 + 1, 缓冲区回绕
	
    return SUCCESS;
}

/*****************************************************************************
 * @\fn  : Uart_RxQueueOut
 * @\author : pi
 * @\date : 2016 - 6 - 22
 * @\brief : 从接收队列中读取一个数据
 * @\param[in] : QueueMemType *data  pointer
 * @\param[out] : 从接收队列中读取的一个数据的指针
 * @\return : ERROR, SUCCESS
 * @\attention : 
 * @\note [others] : 

*****************************************************************************/
static UartStatus Uart_RxQueueOut(QueueMemType *pdata)
{
    register QueuePointerType QHead = Rx_Queue.QHead;
    if(QHead == Rx_Queue.QTail) //接收缓冲区为空,无数据
    {
       return ERROR;
    }
	*pdata = Rx_Queue.Base[Rx_Queue.QTail]; //从队列中取出数据
	Rx_Queue.QTail = (Rx_Queue.QTail + 1) % UART_Q_RX_BUF_SIZE; //尾指针+1, 缓冲区回绕
	
	return SUCCESS;
}

/*****************************************************************************
 * @\fn  : Uart_ReadDequeueByte
 * @\author : pi
 * @\date : 2016 - 6 - 24
 * @\brief : 读取循环队列中的一个字节, 并且该数据已出队
 * @\param[in] :  
 * @\param[out] : uint8_t *pdata : 已出队的字节的指针
 * @\return : 
 * @\attention : 
 * @\note [others] : 外部调用

*****************************************************************************/
UartStatus Uart_ReadDequeueByte(uint8_t *pdata)
{
   return Uart_RxQueueOut((QueueMemType *)pdata);
}


/*****************************************************************************
 * @\fn  : Uart_ReadDequeueBuff
 * @\author : pi
 * @\date : 2016 - 6 - 22
 * @\brief : 从串口接收缓冲区读取全部数据, 并且数据已出队
 * @\param[in] : QueueMemType *ReadOutBuff  o
 * @\param[out] : ReadOutBuff: 输出的缓冲区数据指针
 * @\return : 
 * @\attention : 
 * @\note [others] : 外部调用

*****************************************************************************/
uint16_t Uart_ReadDequeueBuff(QueueMemType *ReadOutBuff)
{
	register uint16_t index = 0;
	while(Uart_RxQueueOut(&ReadOutBuff[index]) == SUCCESS)
	{
	   index++;
	}

	return index;
}


/*****************************************************************************
 * @\fn  : Uart_RxToTx
 * @\author : pi
 * @\date : 2016 - 6 - 24
 * @\brief : 将串口接收到的数据即刻发送出去
 * @\param[in] :  uint16_t MaxSize: 数组长度 
 * @\param[out] : uint8_t * ReadOutBuff ： 输出数据的指针
 * @\return : 
 * @\attention : 
 * @\note [others] : 

*****************************************************************************/
void Uart_RxToTx(uint8_t * ReadOutBuff, uint16_t MaxSize)
{
    #if 1
	
    #ifdef UART_DEBUG
    uint16_t len = 0;

	len = Uart_ReadDequeueBuff((QueueMemType *) ReadOutBuff);
	
	if(len > MaxSize)
	{
	   len = MaxSize;
	}
	if(len)
	{
	   Uart_SendDatas(ReadOutBuff, len);
	}
	#endif

	#else
        
    uint8_t c = 0;
    
	if(Uart_ReadDequeueByte(&c) == SUCCESS)
	{
	   Uart_SendByte(c);
	}

	#endif
}


void Queue_UART_IRQHandler(void)
{

#if (PRINTF_OUT_SEL != UART_BLOCK)
  uint8_t data;

  #if 0
  if(USART_GetITStatus(QUEUE_UART, USART_IT_RXNE))  // 接收非空
  #else
  if(READ_REG_32_BIT(QUEUE_UART->SR, USART_SR_RXNE))
  #endif
  {
    /* Read one byte from the receive data register */
	#if 0
    data = USART_ReceiveData(QUEUE_UART);
    #else
    data    = (uint8_t)(QUEUE_UART->DR & (uint16_t)0x01FF);
    #endif
	
    if(Uart_RxQueueIn(data) == ERROR)
    {
       /* Disable the USARTy Receive interrupt */
	   #if 0
       USART_ITConfig(QUEUE_UART, USART_IT_RXNE, DISABLE);
	   #else
       CLEAR_REG_32_BIT(QUEUE_UART->CR1, USART_CR1_RXNEIE);
	   #endif
    }
  }

   #if 0
   if(USART_GetITStatus(QUEUE_UART, USART_IT_TXE))  // 发送数据空
   #else
   if(READ_REG_32_BIT(QUEUE_UART->SR, USART_SR_TXE))
   #endif
   {   
      Uart_DisableTxInterrupt();
      /* Write one byte to the transmit data register */
      if(Uart_TxQueueOut() == SUCCESS)  //继续串口发送队列中的剩余数据
      {
           Uart_EnableTxInterrupt();
	  }
   }
#endif
}

