#ifndef __DELAY_H
#define __DELAY_H 			   
//#include "sys.h"  
#include "stm32f0xx.h"





//#define delay_ms(n) SysDelay_Xms(n)


extern volatile uint32_t time1_delay_cnt;

void delay_ms(uint32_t nms);
void delay_us(uint32_t nus);

#endif


