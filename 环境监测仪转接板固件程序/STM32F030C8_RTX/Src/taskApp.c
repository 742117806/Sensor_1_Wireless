//RTX任务应用

#include "taskApp.h"
#include "wireless_drv.h"
#include "uart.h"
#include "protocol.h"
#include "device.h"
#include "74.h"
#include "CRC16.h"
#include "encrypt.h"



/* 
********************************************************************************************************** 
                      函数声明 
********************************************************************************************************** 
*/ 

	
static void AppTaskCreate (void); 
//__task void AppTaskLED(void); 
__task void AppTaskStart(void); 
 
/* 
********************************************************************************************************** 
                       变量
********************************************************************************************************** 
*/ 
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
//static uint64_t AppTaskLEDStk[128/8];     		/* LED任务栈 */ 
static uint64_t AppTaskStartStk[128/8];   		/* 开始任务栈 */ 
static uint64_t AppTaskWirelessStk[960/8];     	/* 无线任务栈 */ 
static uint64_t AppTaskUartRxStk[256/8];     		/* 串口接收数据处理任务栈 */ 
//static uint64_t AppTaskUartTxStk[256/8];     		/* 串口发送数据处理任务栈 */ 
 
/* 任务句柄 */ 
//OS_TID HandleTaskLED = NULL; 
OS_TID HandleTaskWireless = NULL; 
OS_TID HandleTaskUartRx = NULL; 
//OS_TID HandleTaskUartTx = NULL; 
 
/* 定时器句柄 */ 
//OS_ID  HandleTimerID1; 

/* 信号量 */
//OS_SEM semaphore;
 
    

 
 
/* 
********************************************************************************************************* 
*  函 数 名: AppTaskLED 
*  功能说明: LED闪烁  
*  形    参: 无 
*  返 回 值: 无 
*    优 先 级: 1  (数值越小优先级越低，这个跟 uCOS相反) 
********************************************************************************************************* 
*/ 
//__task void AppTaskLED(void) 
//{ 

//	
//    while(1) 
//    { 

//		LEDR_TOGGLE();

//		
//		//HAL_UART_Transmit(&huart2,(uint8_t*)"uart2 Init\r\n",12,1000);
//		os_dly_wait(30); 
//    } 
//} 

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
*    优 先 级: 1  (数值越小优先级越低，这个跟 uCOS相反) 
********************************************************************************************************* 
*/ 
void AppTaskWireless(void)
{
	Get_WireLessChannel(Wireless_Channel);
	Wireless_Init();
	Si4438_Receive_Start(Wireless_Channel[0]); //开始接收无线数据	
	while(1) 
    { 
		if (WIRELESS_STATUS == Wireless_RX_Finish)
		{
//			#if _74CODE_EN
//			p = (FRAME_CMD_t*)Wireless_Buf.Wireless_RxData;
//			if(p->Ctrl.c_AFN == 0)
//			{
//				FrameData_74Convert((FRAME_CMD_t*)Wireless_Buf.Wireless_RxData,Wireless_Buf.Wireless_PacketLength,&out_len,0); //解码
//				Wireless_Buf.Wireless_PacketLength = out_len;		//解码后长度
//			}
//			#endif
//			#if _ENCRYPT_EN
//			if(p->FSQ.encryptType != 0)
//			{
//			
//				Encrypt_Convert(Wireless_Buf.Wireless_RxData,Wireless_Buf.Wireless_PacketLength,&out_len,0);		//解密
//				Wireless_Buf.Wireless_PacketLength = out_len;
//			}
//			#endif 			
			LEDG_TOGGLE();
			DEBUG_SendBytes(Wireless_Buf.Wireless_RxData,Wireless_Buf.Wireless_PacketLength);
			WirelessRxProcess(Wireless_Buf.Wireless_RxData,Wireless_Buf.Wireless_PacketLength);
			Si4438_Receive_Start(Wireless_Channel[0]); //开始接收无线数据

		}
		if (WIRELESS_STATUS == Wireless_TX_Finish)
		{
			DEBUG_Printf("Wireless_TX_Finish\r\n");
			Si4438_Receive_Start(Wireless_Channel[0]); //开始接收无线数据
		}
		else if (WIRELESS_STATUS == Wireless_RX_Failure)
		{
			WirelessRx_Timeout_Cnt = 0;
			DEBUG_Printf("Wireless_RX_Failure\r\n");
			os_dly_wait(3);
			Set_Property(Interrupt_Close);
			os_dly_wait(20);
			Si4438_Receive_Start(Wireless_Channel[0]); //开始接收无线数据
		}
//		else if ((WIRELESS_STATUS == Wireless_RX_Sync) && (WirelessRx_Timeout_Cnt > 500)) //500ms超时
//		{

//			DEBUG_Printf("Wireless_RX_Sync\r\n");
//			os_dly_wait(3);
//			Set_Property(Interrupt_Close);
//			os_dly_wait(20);
//			Si4438_Receive_Start(Wireless_Channel[0]); //开始接收无线数据
//			WirelessRx_Timeout_Cnt = 0;
//		}
//		os_sem_send(&semaphore);
		DEBUG_Printf("\r\nAppTaskWireless"); 
		os_dly_wait(300); 
    } 
}

/* 
********************************************************************************************************* 
*  函 数 名: AppTaskUartRx 
*  功能说明: 串口数据处理
*  形    参: 无 
*  返 回 值: 无 
*    优 先 级: 2  (数值越小优先级越低，这个跟 uCOS相反) 
********************************************************************************************************* 
*/ 
void AppTaskUartRx(void)
{
   

    while(1)
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
		}
		
		/*
		串口接收协议数据
		*/
		if(uart1RecBuff.state == SENSOR_FRAME_FENISH)
		{
		    DEBUG_SendBytes(uart1RecBuff.buff,uart1RecBuff.len);
			uart1RecBuff.state = SENSOR_FRAME_HEAD;
		}
		
		DEBUG_Printf("\r\nAppTaskUartRx");  
		os_dly_wait(200);
	}

}

/* 
********************************************************************************************************* 
*  函 数 名: AppTaskUartRx 
*  功能说明: 串口数据处理
*  形    参: 无 
*  返 回 值: 无 
*    优 先 级: 2  (数值越小优先级越低，这个跟 uCOS相反) 
********************************************************************************************************* 
*/ 
void AppTaskUartTx(void)
{
//	OS_RESULT xResult;
	os_dly_wait(200);		//2秒
	while(1)
	{
//		xResult = os_sem_wait (&semaphore, 100); 
//        switch (xResult) 
//        { 
//            /* 无需等待接受到信号量同步信号 */ 
//            case OS_R_OK: 
//                DEBUG_Printf("无需等待接受到信号量同步信号\r\n"); 
//                break;  
//            /* 信号量不可用，usMaxBlockTime 等待时间内收到信号量同步信号 */ 
//            case OS_R_SEM: 
//                DEBUG_Printf("信号量不可用，usMaxBlockTime 等待时间内收到信号量同步信号\r\n"); 
//                break; 
//            /* 超时 */ 
//            case OS_R_TMO: 

//                break; 
//            /* 其他值不处理 */ 
//            default:                      
//                break; 
//		}
		DEBUG_Printf("\r\nAppTaskUartTx");
		//SensorDataReadCmdSend();
	    os_dly_wait(200);		//20秒
	}
}

/* 
********************************************************************************************************* 
*  函 数 名: AppTaskStart 
*  功能说明: 启动任务，也就是最高优先级任务。 
*  形    参: 无 
*  返 回 值: 无 
*    优 先 级: 2   
********************************************************************************************************* 
*/ 	

__task void AppTaskStart(void) 
{ 
	

	AppTaskCreate(); 
	while(1)
	{
		DEBUG_Printf("\r\nAppTaskStart");
		SensorDataReadCmdSend();
		os_dly_wait(100);
	}
	//os_tsk_delete_self();
}

/* 
********************************************************************************************************* 
*  函 数 名: RTX_OS_Init 
*  功能说明: 启动RTX系统
*  形    参: 无 
*  返 回 值: 无 
********************************************************************************************************* 
*/ 

 void RTX_OS_Init(void)
 {
 /* 创建启动任务 */ 
    os_sys_init_user (AppTaskStart,             /* 任务函数 */ 
                    4,                        /* 任务优先级 */ 
                    &AppTaskStartStk,         /* 任务栈 */ 
                    sizeof(AppTaskStartStk)); /* 任务栈大小，单位字节数 */ 

 }
 
 
/* 
********************************************************************************************************* 
*  函 数 名: AppTaskCreate 
*  功能说明: 创建应用任务 
*  形    参: 无 
*  返 回 值: 无 
********************************************************************************************************* 
*/ 
static void AppTaskCreate (void) 
{ 
//	HandleTaskLED = os_tsk_create_user(AppTaskLED,              /* 任务函数 */  
//                                     4,                       /* 任务优先级 */  
//                                     &AppTaskLEDStk,          /* 任务栈 */ 
//                                     sizeof(AppTaskLEDStk));  /* 任务栈大小，单位字节数 */ 
									 
	HandleTaskWireless = os_tsk_create_user(AppTaskWireless,              /* 任务函数 */  
                                     3,                       /* 任务优先级 */  
                                     &AppTaskWirelessStk,          /* 任务栈 */ 
                                     sizeof(AppTaskWirelessStk));  /* 任务栈大小，单位字节数 */ 
									 
	HandleTaskUartRx = os_tsk_create_user(AppTaskUartRx,              /* 任务函数 */  
                                     2,                       /* 任务优先级 */  
                                     &AppTaskUartRxStk,          /* 任务栈 */ 
                                     sizeof(AppTaskUartRxStk));  /* 任务栈大小，单位字节数 */
									 
//	HandleTaskUartTx = os_tsk_create_user(AppTaskUartTx,              /* 任务函数 */  
//                                     1,                       /* 任务优先级 */  
//                                     &AppTaskUartTxStk,          /* 任务栈 */ 
//                                     sizeof(AppTaskUartTxStk));  /* 任务栈大小，单位字节数 */								 

	/* 创建信号量计数值是 0, 用于任务同步 */ 
	//os_sem_init (&semaphore, 0);  
	
									 
}

