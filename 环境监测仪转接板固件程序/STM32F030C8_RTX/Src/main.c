
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f0xx_hal.h"

/* USER CODE BEGIN Includes */
//#include "taskApp.h"
#include "delay.h"
#include "led.h"
#include "stm32f0_spi.h"   
#include "wireless_drv.h" 

#include "uart.h"
#include "protocol.h"
#include "device.h"
#include "74.h"
#include "CRC16.h"
#include "encrypt.h"

              
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM1_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/**
********************************************************************************************************* 
*  函 数 名: AppTaskWireless 
*  功能说明: 无线接收数据处理
*  形    参: @dat:无线接收的数据 @len:接收数据的长度
*  返 回 值: 无 
********************************************************************************************************* 
**/ 
uint8_t  WirelessRxProcess(uint8_t *dat,uint8_t len)
{
	uint16_t index;		//数据标识
	FRAME_CMD_t *frame_cmd =(FRAME_CMD_t*)dat;
	uint16_t crc=0;		//根据接收数据计算出来的CRC16
	uint16_t crc_1=0;	//接收到的CRC16
	uint8_t out_len;		//解密后的长度
	
    if(frame_cmd->Ctrl.c_AFN == 0)
	{
		DEBUG_Printf("\r\n74Code");
		FrameData_74Convert((FRAME_CMD_t*)dat,len,&out_len,0); //解码
		len = out_len;		//解码后长度
	}
	
	if((frame_cmd->FSQ.encryptType == 2)||(frame_cmd->FSQ.encryptType == 1))		//是加密的
	{
		DEBUG_Printf("\r\nencryptType");
		Encrypt_Convert((uint8_t*)frame_cmd, len, &out_len, 0);         //解密
	}
	else		//不加密
	{
		DEBUG_Printf("\r\nno encryptType");
	}
	
	crc = CRC16_2(dat,frame_cmd->DataLen+11-3);		//一帧数据总长度减去2个字节的CRC和一个结束符
	
	crc_1 = (dat[frame_cmd->DataLen+11-3]<<8)+dat[frame_cmd->DataLen+11-2];
	
	if(crc != crc_1)		//检验CRC
	{
		DEBUG_Printf("\r\nCRC16_ERROR!!");
		return 0;		//CRC16错误
	}
	
	index = (uint16_t)(frame_cmd->userData.Index[1]<<8)+frame_cmd->userData.Index[2];
	switch(frame_cmd->userData.Index[0])
	{
		case 0xFF:	//初始化数据标识
			switch(index)
			{
				case 0xFFFF:			//设备入网
					DeviceJoinNet(frame_cmd);
					
					break;
				case 0xFFFE:
					DEBUG_Printf("\r\n0xFFFE");
					break;
				default:
					break;
			}
			break;
		case 0x00:	//状态量数据标识
			break;
		case 0x01:	//控制量数据标识
			break;
		case 0x02:	//电器功能控制标识
			break;
		case 0x03:	//传感器类数据标识
			switch(index)
			{
				case 0xFFFF:			//读取传感器数据
					//SensorDataReadCmdSend();
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}

	return 1;
}

/* 
********************************************************************************************************* 
*  函 数 名: AppTaskWireless 
*  功能说明: 无线数据处理
*  形    参: 无 
*  返 回 值: 无 
********************************************************************************************************* 
*/ 
void AppTaskWireless(void)
{
//	FRAME_CMD_t *p;
//	uint8_t out_len;
	
	if (WIRELESS_STATUS == Wireless_RX_Finish)
	{
		
		if(Wireless_Buf.Wireless_RxData[0]==0xAC)
        {
			DEBUG_SendBytes(Wireless_Buf.Wireless_RxData,Wireless_Buf.Wireless_PacketLength);
			WirelessRxProcess(Wireless_Buf.Wireless_RxData,Wireless_Buf.Wireless_PacketLength);

		}	
		if (WIRELESS_STATUS == Wireless_RX_Finish)
		{
			Si4438_Receive_Start(Wireless_Channel[0]); //开始接收无线数据
		}
		
	}
	if (WIRELESS_STATUS == Wireless_TX_Finish)
	{
		DEBUG_Printf("Wireless_TX_Finish\r\n");
		Si4438_Receive_Start(Wireless_Channel[0]); //开始接收无线数据
	}
	else if (WIRELESS_STATUS == Wireless_RX_Failure)
	{
		WirelessRx_Timeout_Cnt = 0;
		Si4438_Receive_Start(Wireless_Channel[0]); //开始接收无线数据
	}
}

/* 
********************************************************************************************************* 
*  函 数 名: AppTaskUartRx 
*  功能说明: 串口数据处理
*  形    参: 无 
*  返 回 值: 无 
********************************************************************************************************* 
*/ 
void AppTaskUartRx(void)
{
   

	/*
	串口超时接收数据处理
	*/
//		if((uart1RecBuff.timeOut > 20)&&(uart1RecBuff.cnt>0))   
//		{  
//			
//			DEBUG_SendBytes(uart1RecBuff.buff,uart1RecBuff.cnt);
//			Si4438_Transmit_Start(&Wireless_Buf,Wireless_Channel[0],uart1RecBuff.buff,uart1RecBuff.cnt);
//			uart1RecBuff.cnt = 0;
//			uart1RecBuff.timeOut = 0;
//		}
	
	
	/*
	串口烧录设备MAC协议数据处理
	*/
	if(MAC_UartRec.state == UartRx_Finished)
	{
		DEBUG_SendBytes(MAC_UartRec.buff,MAC_UartRec.len);
		MAC_UartRec.state = UartRx_FrameHead;
		DeviceMAC_WriteProcess(MAC_UartRec.buff, MAC_UartRec.len);
	}
	
	/*
	串口接收协议数据
	*/
	if(uart1RecBuff.state == SENSOR_FRAME_FENISH)
	{
		DEBUG_SendBytes(uart1RecBuff.buff,uart1RecBuff.cnt);
		
		SensorProcess(uart1RecBuff.buff);
		uart1RecBuff.state = SENSOR_FRAME_HEAD;
	}
	
	//DEBUG_Printf("\r\nAppTaskUartRx");  

}

/* 
********************************************************************************************************* 
*  函 数 名: AppTaskUartRx 
*  功能说明: 串口数据处理
*  形    参: 无 
*  返 回 值: 无 
********************************************************************************************************* 
*/ 
uint32_t SensorDataReadCnt = 0;
extern uint8_t last_PM25_value[3];          //上次PM2_5的值
void AppTaskUartTx(void)
{

	
	if(SensorDataReadCnt > 10000)		//10秒
	{
		SensorDataReadCnt = 0;
		SensorDataReadCmdSend();
	}
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
 // uint8_t mcuFlagBuf[10];

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  delay_init(48);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
	HAL_TIM_Base_Start_IT(&htim1);
	HAL_UART_Receive_IT(&huart1, &uart1Rec, 1);
	HAL_UART_Receive_IT(&huart2, &uart2Rec, 1);
//	_74Code_Test();
	Device_MAC_Init();
	
	Get_WireLessChannel(Wireless_Channel);
	//计算出密文，存放在aes_w，供加解密用
	AES_Init(); 
	Wireless_Init();
	Si4438_Receive_Start(Wireless_Channel[0]); //开始接收无线数据	
	
	//RTX_OS_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  //Test_Write(123,123);
//  STMFLASH_Write(FLASH_USER_START_ADDR,(uint16_t*)"12345678",4);
//  STMFLASH_Read(FLASH_USER_START_ADDR,(uint16_t*)mcuFlagBuf,8);
//  DEBUG_SendBytes(mcuFlagBuf,8);

  while (1)
  {

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
		
	  AppTaskWireless();
	 AppTaskUartRx();
	  AppTaskUartTx();
  }
  /* USER CODE END 3 */

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* SPI1 init function */
static void MX_SPI1_Init(void)
{

  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* TIM1 init function */
static void MX_TIM1_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 48-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1000;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USART1 init function */
static void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 38400;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USART2 init function */
static void MX_USART2_UART_Init(void)
{

  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LEDG_Pin|LEDR_Pin|SI4438_NSS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SI4438_SDN_GPIO_Port, SI4438_SDN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LEDG_Pin LEDR_Pin */
  GPIO_InitStruct.Pin = LEDG_Pin|LEDR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : SI4438_NSS_Pin */
  GPIO_InitStruct.Pin = SI4438_NSS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SI4438_NSS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SI4438_nIRQ_Pin */
  GPIO_InitStruct.Pin = SI4438_nIRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SI4438_nIRQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SI4438_SDN_Pin */
  GPIO_InitStruct.Pin = SI4438_SDN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SI4438_SDN_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
