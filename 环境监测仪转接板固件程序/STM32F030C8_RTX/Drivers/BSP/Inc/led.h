

#ifndef __LED_H__
#define __LED_H__

#include "stm32f0xx.h"
#include "main.h"


#define LEDR_ON()  	HAL_GPIO_WritePin(LEDR_GPIO_Port, LEDR_Pin, GPIO_PIN_SET);
#define LEDR_OFF() 	HAL_GPIO_WritePin(LEDR_GPIO_Port, LEDR_Pin, GPIO_PIN_RESET);
#define LEDR_TOGGLE() 	HAL_GPIO_TogglePin(LEDR_GPIO_Port, LEDR_Pin);
#define LEDG_ON()  	HAL_GPIO_WritePin(LEDG_GPIO_Port, LEDG_Pin, GPIO_PIN_SET);
#define LEDG_OFF() 	HAL_GPIO_WritePin(LEDG_GPIO_Port, LEDG_Pin, GPIO_PIN_RESET);
#define LEDG_TOGGLE() 	HAL_GPIO_TogglePin(LEDG_GPIO_Port, LEDG_Pin);


#endif

