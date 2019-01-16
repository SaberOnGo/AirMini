
#include "uart_queue.h"
#include "GlobalDef.h"
#include "os_global.h"

static QueueMemType RxBuf[UART_Q_RX_BUF_SIZE] = {0};
static QueueMemType TxBuf[UART_Q_TX_BUF_SIZE] = {0};

static volatile T_UartQueue Tx_Queue, Rx_Queue;

static volatile uint16_t  is_rx_flag = 0;    // receive data or not

static volatile uint8_t TxSemaLock = UART_IDLE; //�ź���

 //����
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
 * @\brief : ��ֹUART TX�ж�
 * @\param[in] : void  
 * @\param[out] : none
 * @\return : 
 * @\attention : 
 * @\note [others] : ���SR�Ĵ�����TC,TXE bit, ��ֹTIEN,TCIEN,TEN

*****************************************************************************/
static void Uart_DisableTxInterrupt(void)
{
     #if 0
     CLEAR_REGISTER_BIT(HARD_USART->CR2, UART2_CR2_TIEN);  //�����жϽ�ֹ
	 CLEAR_REGISTER_BIT(HARD_USART->CR2, UART2_CR2_TCIEN);  //��������жϽ�ֹ
	 
     
	 CLEAR_REGISTER_BIT(HARD_USART->SR, UART2_SR_TC);  //��TCλ
	 //UART1->DR = 0;   //��SR�Ĵ����е�TXEλ: дDR�Ĵ���, ���ܼ����, �����͵�����ǰ,���ᷢһ���ֽڵ�0x00

	 //CLEAR_REGISTER_BIT(UART1->CR2, UART1_CR2_TEN); //���ͽ�ֹ, ���ܼ����, �����м�����ݿ��ܻ����
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
 * @\brief : �����Ͷ�����д��һ������
 * @\param[in] : ��д�������
 * @\param[out] : none
 * @\return : ERROR, SUCCESS
 * @\attention : 
 * @\note [others] : �ڲ�����

*****************************************************************************/
static UartStatus Uart_TxQueueIn(QueueMemType data)
{
   QueuePointerType QTail = Tx_Queue.QTail;
   
   if(((Tx_Queue.QHead + 1) % UART_Q_TX_BUF_SIZE) == QTail) // ����������
   {
      return ERROR;
   }
   Tx_Queue.Base[Tx_Queue.QHead] = data;
   
   //����ͷָ�룬������˻�������ĩ�ˣ����Զ����ص�����������ʼ��
   Tx_Queue.QHead = (Tx_Queue.QHead + 1) % UART_Q_TX_BUF_SIZE; //����������
   
   return SUCCESS;
}

/*****************************************************************************
 * @\fn  : Uart_TxQueueOut
 * @\author : pi
 * @\date : 2016 - 6 - 22
 * @\brief : �ӷ��ͻ�����ȡһ������, �Ӵ������
 * @\param[in] : none
 * @\param[out] : none
 * @\return : ERROR, SUCCESS
 * @\attention : 
 * @\note [others] : �ڲ�����

*****************************************************************************/
#define  Uart_OutByte(Data)   QUEUE_UART->DR = ((Data) & (uint16_t)0x01FF)
static UartStatus Uart_TxQueueOut(void)
{
    if(Tx_Queue.QHead == Tx_Queue.QTail)  //������Ϊ��, �޷�������
    {
       TxSemaLock = UART_IDLE; //���ڿ���
       return ERROR;
    }
	TxSemaLock = UART_BUSY; //����æ
    //�ӷ��ͻ�����ȡһ������
    Uart_OutByte(Tx_Queue.Base[Tx_Queue.QTail]);
	
    // ����βָ���λ�ã�������˻�������ĩ�ˣ����Զ����ص�����������ʼ��
    Tx_Queue.QTail = (Tx_Queue.QTail + 1) % UART_Q_TX_BUF_SIZE; //βָ��+1, ����������	

    return SUCCESS;
}

/*****************************************************************************
 * @\fn  : Uart_IsTxDataBuffEmpty
 * @\author : pi
 * @\date : 2016 - 6 - 23
 * @\brief : ѭ�����Ͷ����Ƿ�Ϊ��
 * @\param[in] : void  
 * @\param[out] : none
 * @\return : ����Ϊ��: 1;  ����������: 0
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

	if(!TxSemaLock) //���ڿ���
	{
	   Uart_DisableTxInterrupt();
	}
    status = Uart_TxQueueIn(data);
	if(!TxSemaLock) //���ڿ���
	{
       Uart_TxQueueOut();  //���ڷ���
       Uart_EnableTxInterrupt();
	}
	
	return status;
}
/*****************************************************************************
 * @\fn  : Uart_SendDatas
 * @\author : pi
 * @\date : 2016 - 6 - 22
 * @\brief : ������д�뷢�Ͷ���, ����������
 * @\param[in] : uint8_t *pString  �������ݵ�ָ��
               uint16_t len      ���ݳ���
 * @\param[out] : none
 * @\return : ����״̬
 * @\attention : 
 * @\note [others] : �ⲿ����

*****************************************************************************/
UartStatus Uart_SendDatas(uint8_t *pString, uint16_t len)
{
    register uint16_t uIndex = 0;
	UartStatus status = SUCCESS;
	uint8_t ReSendTimes = 100; //ÿ�����ݵ��ط�����
	
	if(NULL == pString)
	{
	   return ERROR;
	}
	
	if(! TxSemaLock) //���ڿ���,���Ƚ�ֹ�ж�
	{
	   Uart_DisableTxInterrupt();
	}
	for(; uIndex < len; uIndex++)
	{
	    do

	    {
	       status = Uart_TxQueueIn(pString[uIndex]);  //���ݽ��뷢�ͻ�����
	       NOP();
		   ReSendTimes--;
	    }while(ERROR == status && ReSendTimes);  //һ�η��ʹ���, ���ط�
	    if(ERROR == status)
	    {
	       //return ERROR; //���ʹ����ҳ�ʱ
	       break;
	    }
	}
	if(!TxSemaLock) //���ڿ���
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
 * @\note [others] : �ⲿ����
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
 * @\brief : �жϽ��ջ����������ݳ��ȱ�compare_size ��, С, �����
 * @\param[in] : uint16_t compare_size  Ҫ�ȽϵĴ�Сֵ
 * @\param[out] :  
 * @\return : �ȽϽ��: ���������ݳ���������ıȽϽ��: More, Less, Equal
 * @\attention : 
 * @\note [others] : 

*****************************************************************************/
Compare_State Uart_IsRxBuffSizeEqualThan(uint16_t compare_size)
{
    uint16_t BuffDataLen = 0; //��������ݳ���
    QueuePointerType QHead = Rx_Queue.QHead;

	if(QHead == Rx_Queue.QTail) //������Ϊ��
	{
	   return LESS_STATE;
	}
    else if(QHead > Rx_Queue.QTail) //0  -> QTail -> QHead -> End -> 0, ͷָ�����βָ��
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
 * @\brief : ����ն��д���һ������
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
    if(((Rx_Queue.QHead + 1) % UART_Q_RX_BUF_SIZE) == Rx_Queue.QTail) //����������
	{
	   if(! is_rx_flag)
	   {
	      is_rx_flag = 1;
	   }
	   return ERROR;
	}
	Rx_Queue.Base[Rx_Queue.QHead] = data; //����һ������
    // ����ͷָ�룬������˻�������ĩ�ˣ����Զ����ص�����������ʼ��
    Rx_Queue.QHead = (Rx_Queue.QHead + 1) % UART_Q_RX_BUF_SIZE; //ͷָ�� + 1, ����������
	
    return SUCCESS;
}

/*****************************************************************************
 * @\fn  : Uart_RxQueueOut
 * @\author : pi
 * @\date : 2016 - 6 - 22
 * @\brief : �ӽ��ն����ж�ȡһ������
 * @\param[in] : QueueMemType *data  pointer
 * @\param[out] : �ӽ��ն����ж�ȡ��һ�����ݵ�ָ��
 * @\return : ERROR, SUCCESS
 * @\attention : 
 * @\note [others] : 

*****************************************************************************/
static UartStatus Uart_RxQueueOut(QueueMemType *pdata)
{
    register QueuePointerType QHead = Rx_Queue.QHead;
    if(QHead == Rx_Queue.QTail) //���ջ�����Ϊ��,������
    {
       return ERROR;
    }
	*pdata = Rx_Queue.Base[Rx_Queue.QTail]; //�Ӷ�����ȡ������
	Rx_Queue.QTail = (Rx_Queue.QTail + 1) % UART_Q_RX_BUF_SIZE; //βָ��+1, ����������
	
	return SUCCESS;
}

/*****************************************************************************
 * @\fn  : Uart_ReadDequeueByte
 * @\author : pi
 * @\date : 2016 - 6 - 24
 * @\brief : ��ȡѭ�������е�һ���ֽ�, ���Ҹ������ѳ���
 * @\param[in] :  
 * @\param[out] : uint8_t *pdata : �ѳ��ӵ��ֽڵ�ָ��
 * @\return : 
 * @\attention : 
 * @\note [others] : �ⲿ����

*****************************************************************************/
UartStatus Uart_ReadDequeueByte(uint8_t *pdata)
{
   return Uart_RxQueueOut((QueueMemType *)pdata);
}


/*****************************************************************************
 * @\fn  : Uart_ReadDequeueBuff
 * @\author : pi
 * @\date : 2016 - 6 - 22
 * @\brief : �Ӵ��ڽ��ջ�������ȡȫ������, ���������ѳ���
 * @\param[in] : QueueMemType *ReadOutBuff  o
 * @\param[out] : ReadOutBuff: ����Ļ���������ָ��
 * @\return : 
 * @\attention : 
 * @\note [others] : �ⲿ����

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
 * @\brief : �����ڽ��յ������ݼ��̷��ͳ�ȥ
 * @\param[in] :  uint16_t MaxSize: ���鳤�� 
 * @\param[out] : uint8_t * ReadOutBuff �� ������ݵ�ָ��
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

#if (! GIZWITS_TYPE)
void Queue_UART_IRQHandler(void)
{

#if (PRINTF_OUT_SEL != UART_BLOCK)
  uint8_t data;

  #if 0
  if(USART_GetITStatus(QUEUE_UART, USART_IT_RXNE))  // ���շǿ�
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
   if(USART_GetITStatus(QUEUE_UART, USART_IT_TXE))  // �������ݿ�
   #else
   if(READ_REG_32_BIT(QUEUE_UART->SR, USART_SR_TXE))
   #endif
   {   
      Uart_DisableTxInterrupt();
      /* Write one byte to the transmit data register */
      if(Uart_TxQueueOut() == SUCCESS)  //�������ڷ��Ͷ����е�ʣ������
      {
           Uart_EnableTxInterrupt();
	  }
   }
#endif
}
#endif

