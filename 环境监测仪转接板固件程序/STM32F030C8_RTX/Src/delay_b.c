//���ö�ʱ��1ʵ���ӳٺ�������ʱ��1������Ϊ1΢��

#include "delay.h"
#include "RTL.h"

extern TIM_HandleTypeDef htim1;


//����ӳ�64999us
void delay_us(uint32_t nus)
{
	htim1.Instance->CNT = 0;	//��ռ�����
	while((htim1.Instance->CNT < nus)? 1:0);	
}

////////////////////////////////////////
//��ʱnms
void delay_ms(uint32_t nms)
{
	uint32_t i=0;
	for(i=0;i<nms;i++)
	{
       delay_us(1000);	//1ms
   }
}




