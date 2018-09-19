//实现与串口相关的功能函数

#include "uart.h"

/* 
********************************************************************************************************** 
                      函数声明 
********************************************************************************************************** 
*/ 

/* 
********************************************************************************************************** 
                       变量
********************************************************************************************************** 
*/ 

uint8_t uart1Rec;        //UART1中断接收数据
uint8_t uart2Rec;		 //UART2中断接收数据		


extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;


/* 
********************************************************************************************************* 
*  函 数 名: UartSendData 
*  功能说明: 串口发送1个字节函数
*  形    参: @USARTx串口ID(USART1,USART2),@byte 要发送的字节 
*  返 回 值: 无 
********************************************************************************************************* 
*/ 
void UartSendData(USART_TypeDef *USARTx,uint8_t byte)
{
	USARTx->TDR = byte;     

	while((USARTx->ISR&0X40)==0);
}

/* 
********************************************************************************************************* 
*  函 数 名: UartSendBytes 
*  功能说明: 串口发送多个字节函数
*  形    参: @USARTx串口ID(USART1,USART2)，@buf 发送是缓冲区地址，@len 发送字节长度  
*  返 回 值: 无 
********************************************************************************************************* 
*/ 
void UartSendBytes(USART_TypeDef *USARTx,uint8_t *buf,uint16_t len)
{
	uint8_t i = 0;
	for(i=0;i<len;i++)
	{
		UartSendData(USARTx,*buf++);
	}
}

/* 
********************************************************************************************************* 
*  函 数 名: UartSendStr
*  功能说明: 串口发送字符串
*  形    参: @USARTx串口ID(USART1,USART2)，@str字符串缓冲区地址
*  返 回 值: 无 
********************************************************************************************************* 
*/ 
void UartSendStr(USART_TypeDef *USARTx,char *str)
{ 
    while (0 != *str)    
    {    
        UartSendData(USARTx, *str);    
        str++;    
    }    
}

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
int fputc(int ch, FILE *f)
{
    /* Place your implementation of fputc here */
    /* e.g. write a character to the USART2 and Loop until the end of transmission */

    USART1->TDR = ch;
    while ((USART1->ISR & 0X40) == 0)
        ;
    return ch;
}



