#ifndef STM32F0_PERIPHERAL_H
#define STM32F0_PERIPHERAL_H

#include "stm32f0xx.h"





/************************* IO口引脚属性宏定义 ***************************/
#define INPUT_MODE                  0X00
#define OUTPUT_MODE                 0X01
#define AF_MODE                     0X02
#define ANALOG_MODE                 0X03

#define PUSH_PULL                   0X00
#define OPEN_DRAIN                  0X01

#define LOW_SPEED                   0X00
#define MEDIUM_SPEED                0X01
#define HIGH_SPEED                  0X03

#define NO_PULL                     0X00
#define PULL_UP                     0X01
#define PULL_DOWN                   0X02

/************************* GPIO口时钟使能宏定义 ***************************/
#define GPIO_CLKREG                 AHBENR
#define GPIOA_CLOCK_EN              (1<<17)
#define GPIOB_CLOCK_EN              (1<<18)
#define GPIOC_CLOCK_EN              (1<<19)
#define GPIOD_CLOCK_EN              (1<<20)
#define GPIOF_CLOCK_EN              (1<<22)



/***************************** 中断向量初始化结构体 *******************************/
typedef struct
{
  uint8_t NVIC_IRQChannel;             /*!< Specifies the IRQ channel to be enabled or disabled.
                                            This parameter can be a value of @ref IRQn_Type 
                                            (For the complete STM32 Devices IRQ Channels list, 
                                            please refer to stm32f0xx.h file) */

  uint8_t NVIC_IRQChannelPriority;     /*!< Specifies the priority level for the IRQ channel specified
                                            in NVIC_IRQChannel. This parameter can be a value
                                            between 0 and 3.  */

  FunctionalState NVIC_IRQChannelCmd;  /*!< Specifies whether the IRQ channel defined in NVIC_IRQChannel
                                            will be enabled or disabled. 
                                            This parameter can be set either to ENABLE or DISABLE */   
} NVIC_InitTypeDef;



void NVIC_Init(NVIC_InitTypeDef* NVIC_InitStruct);



/*
#include "led.h"
#include "delay.h"
#include "stm32f0_usart.h"
*/
















#endif



