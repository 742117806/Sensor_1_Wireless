#include "stm32f0_spi.h"


uint8_t SPI_RWbyte(uint8_t sdata)
{
	extern SPI_HandleTypeDef hspi1;
	
	uint8_t rData;
	HAL_SPI_TransmitReceive(&hspi1,&sdata,&rData,1,100);
	return  rData;

}






