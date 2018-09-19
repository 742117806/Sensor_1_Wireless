
#ifndef __UART_H__
#define __UART_H__

#include "stm32f0xx.h"


#define _DEBUG_ 0				//�����Ƿ��ӡ������Ϣ��0����ӡ��1��ӡ

#if _DEBUG_
#define DEBUG_Printf(...)   printf(__VA_ARGS__)
#define DEBUG_SendBytes(b,n)	UartSendBytes(USART1,b,n);
#define DEBUG_SendStr(s)  		UartSendStr(USART1,s)
#else
#define DEBUG_Printf(...) 
#define DEBUG_SendBytes(b,n)
#define DEBUG_SendStr(s)
#endif

extern uint8_t uart1Rec;        //UART1�жϽ������ݱ���
extern uint8_t uart2Rec;		 //UART2�жϽ������ݱ���

void Uart1SendData(uint8_t byte);
void Uart1SendBytes(uint8_t *buf,uint16_t len);
void UartSendData(USART_TypeDef *USARTx,uint8_t byte);
void UartSendBytes(USART_TypeDef *USARTx,uint8_t *buf,uint16_t len);
void UartSendStr(USART_TypeDef *USARTx,char *str);
#endif

