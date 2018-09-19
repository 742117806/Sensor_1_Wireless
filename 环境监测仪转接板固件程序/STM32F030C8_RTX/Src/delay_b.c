//利用定时器1实现延迟函数，定时器1个计数为1微秒

#include "delay.h"
#include "RTL.h"

extern TIM_HandleTypeDef htim1;


//最大延迟64999us
void delay_us(uint32_t nus)
{
	htim1.Instance->CNT = 0;	//清空计数器
	while((htim1.Instance->CNT < nus)? 1:0);	
}

////////////////////////////////////////
//延时nms
void delay_ms(uint32_t nms)
{
	uint32_t i=0;
	for(i=0;i<nms;i++)
	{
       delay_us(1000);	//1ms
   }
}




