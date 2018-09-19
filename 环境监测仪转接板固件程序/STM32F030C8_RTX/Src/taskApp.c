//RTX����Ӧ��

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
                      �������� 
********************************************************************************************************** 
*/ 

	
static void AppTaskCreate (void); 
//__task void AppTaskLED(void); 
__task void AppTaskStart(void); 
 
/* 
********************************************************************************************************** 
                       ����
********************************************************************************************************** 
*/ 
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
//static uint64_t AppTaskLEDStk[128/8];     		/* LED����ջ */ 
static uint64_t AppTaskStartStk[128/8];   		/* ��ʼ����ջ */ 
static uint64_t AppTaskWirelessStk[960/8];     	/* ��������ջ */ 
static uint64_t AppTaskUartRxStk[256/8];     		/* ���ڽ������ݴ�������ջ */ 
//static uint64_t AppTaskUartTxStk[256/8];     		/* ���ڷ������ݴ�������ջ */ 
 
/* ������ */ 
//OS_TID HandleTaskLED = NULL; 
OS_TID HandleTaskWireless = NULL; 
OS_TID HandleTaskUartRx = NULL; 
//OS_TID HandleTaskUartTx = NULL; 
 
/* ��ʱ����� */ 
//OS_ID  HandleTimerID1; 

/* �ź��� */
//OS_SEM semaphore;
 
    

 
 
/* 
********************************************************************************************************* 
*  �� �� ��: AppTaskLED 
*  ����˵��: LED��˸  
*  ��    ��: �� 
*  �� �� ֵ: �� 
*    �� �� ��: 1  (��ֵԽС���ȼ�Խ�ͣ������ uCOS�෴) 
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
*  �� �� ��: AppTaskWireless 
*  ����˵��: ���߽������ݴ���
*  ��    ��: @dat:���߽��յ����� @len:�������ݵĳ���
*  �� �� ֵ: �� 
********************************************************************************************************* 
**/ 
uint8_t  WirelessRxProcess(uint8_t *dat,uint8_t len)
{
	uint16_t index;		//���ݱ�ʶ
	FRAME_CMD_t *frame_cmd =(FRAME_CMD_t*)dat;
	uint16_t crc=0;		//���ݽ������ݼ��������CRC16
	uint16_t crc_1=0;	//���յ���CRC16
	uint8_t out_len;		//���ܺ�ĳ���
	
    if(frame_cmd->Ctrl.c_AFN == 0)
	{
		DEBUG_Printf("\r\n74Code");
		FrameData_74Convert((FRAME_CMD_t*)dat,len,&out_len,0); //����
		len = out_len;		//����󳤶�
	}
	
	if((frame_cmd->FSQ.encryptType == 2)||(frame_cmd->FSQ.encryptType == 1))		//�Ǽ��ܵ�
	{
		DEBUG_Printf("\r\nencryptType");
		Encrypt_Convert((uint8_t*)frame_cmd, len, &out_len, 0);         //����
	}
	else		//������
	{
		DEBUG_Printf("\r\nno encryptType");
	}
	
	crc = CRC16_2(dat,frame_cmd->DataLen+11-3);		//һ֡�����ܳ��ȼ�ȥ2���ֽڵ�CRC��һ��������
	
	crc_1 = (dat[frame_cmd->DataLen+11-3]<<8)+dat[frame_cmd->DataLen+11-2];
	
	if(crc != crc_1)		//����CRC
	{
		DEBUG_Printf("\r\nCRC16_ERROR!!");
		return 0;		//CRC16����
	}
	
	index = (uint16_t)(frame_cmd->userData.Index[1]<<8)+frame_cmd->userData.Index[2];
	switch(frame_cmd->userData.Index[0])
	{
		case 0xFF:	//��ʼ�����ݱ�ʶ
			switch(index)
			{
				case 0xFFFF:			//�豸����
					DeviceJoinNet(frame_cmd);
					
					break;
				case 0xFFFE:
					DEBUG_Printf("\r\n0xFFFE");
					break;
				default:
					break;
			}
			break;
		case 0x00:	//״̬�����ݱ�ʶ
			break;
		case 0x01:	//���������ݱ�ʶ
			break;
		case 0x02:	//�������ܿ��Ʊ�ʶ
			break;
		case 0x03:	//�����������ݱ�ʶ
			switch(index)
			{
				case 0xFFFF:			//��ȡ����������
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
*  �� �� ��: AppTaskWireless 
*  ����˵��: �������ݴ���
*  ��    ��: �� 
*  �� �� ֵ: �� 
*    �� �� ��: 1  (��ֵԽС���ȼ�Խ�ͣ������ uCOS�෴) 
********************************************************************************************************* 
*/ 
void AppTaskWireless(void)
{
	Get_WireLessChannel(Wireless_Channel);
	Wireless_Init();
	Si4438_Receive_Start(Wireless_Channel[0]); //��ʼ������������	
	while(1) 
    { 
		if (WIRELESS_STATUS == Wireless_RX_Finish)
		{
//			#if _74CODE_EN
//			p = (FRAME_CMD_t*)Wireless_Buf.Wireless_RxData;
//			if(p->Ctrl.c_AFN == 0)
//			{
//				FrameData_74Convert((FRAME_CMD_t*)Wireless_Buf.Wireless_RxData,Wireless_Buf.Wireless_PacketLength,&out_len,0); //����
//				Wireless_Buf.Wireless_PacketLength = out_len;		//����󳤶�
//			}
//			#endif
//			#if _ENCRYPT_EN
//			if(p->FSQ.encryptType != 0)
//			{
//			
//				Encrypt_Convert(Wireless_Buf.Wireless_RxData,Wireless_Buf.Wireless_PacketLength,&out_len,0);		//����
//				Wireless_Buf.Wireless_PacketLength = out_len;
//			}
//			#endif 			
			LEDG_TOGGLE();
			DEBUG_SendBytes(Wireless_Buf.Wireless_RxData,Wireless_Buf.Wireless_PacketLength);
			WirelessRxProcess(Wireless_Buf.Wireless_RxData,Wireless_Buf.Wireless_PacketLength);
			Si4438_Receive_Start(Wireless_Channel[0]); //��ʼ������������

		}
		if (WIRELESS_STATUS == Wireless_TX_Finish)
		{
			DEBUG_Printf("Wireless_TX_Finish\r\n");
			Si4438_Receive_Start(Wireless_Channel[0]); //��ʼ������������
		}
		else if (WIRELESS_STATUS == Wireless_RX_Failure)
		{
			WirelessRx_Timeout_Cnt = 0;
			DEBUG_Printf("Wireless_RX_Failure\r\n");
			os_dly_wait(3);
			Set_Property(Interrupt_Close);
			os_dly_wait(20);
			Si4438_Receive_Start(Wireless_Channel[0]); //��ʼ������������
		}
//		else if ((WIRELESS_STATUS == Wireless_RX_Sync) && (WirelessRx_Timeout_Cnt > 500)) //500ms��ʱ
//		{

//			DEBUG_Printf("Wireless_RX_Sync\r\n");
//			os_dly_wait(3);
//			Set_Property(Interrupt_Close);
//			os_dly_wait(20);
//			Si4438_Receive_Start(Wireless_Channel[0]); //��ʼ������������
//			WirelessRx_Timeout_Cnt = 0;
//		}
//		os_sem_send(&semaphore);
		DEBUG_Printf("\r\nAppTaskWireless"); 
		os_dly_wait(300); 
    } 
}

/* 
********************************************************************************************************* 
*  �� �� ��: AppTaskUartRx 
*  ����˵��: �������ݴ���
*  ��    ��: �� 
*  �� �� ֵ: �� 
*    �� �� ��: 2  (��ֵԽС���ȼ�Խ�ͣ������ uCOS�෴) 
********************************************************************************************************* 
*/ 
void AppTaskUartRx(void)
{
   

    while(1)
	{
        /*
		���ڳ�ʱ�������ݴ���
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
		������¼�豸MACЭ�����ݴ���
		*/
		if(MAC_UartRec.state == UartRx_Finished)
		{
			DEBUG_SendBytes(MAC_UartRec.buff,MAC_UartRec.len);
			MAC_UartRec.state = UartRx_FrameHead;
		}
		
		/*
		���ڽ���Э������
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
*  �� �� ��: AppTaskUartRx 
*  ����˵��: �������ݴ���
*  ��    ��: �� 
*  �� �� ֵ: �� 
*    �� �� ��: 2  (��ֵԽС���ȼ�Խ�ͣ������ uCOS�෴) 
********************************************************************************************************* 
*/ 
void AppTaskUartTx(void)
{
//	OS_RESULT xResult;
	os_dly_wait(200);		//2��
	while(1)
	{
//		xResult = os_sem_wait (&semaphore, 100); 
//        switch (xResult) 
//        { 
//            /* ����ȴ����ܵ��ź���ͬ���ź� */ 
//            case OS_R_OK: 
//                DEBUG_Printf("����ȴ����ܵ��ź���ͬ���ź�\r\n"); 
//                break;  
//            /* �ź��������ã�usMaxBlockTime �ȴ�ʱ�����յ��ź���ͬ���ź� */ 
//            case OS_R_SEM: 
//                DEBUG_Printf("�ź��������ã�usMaxBlockTime �ȴ�ʱ�����յ��ź���ͬ���ź�\r\n"); 
//                break; 
//            /* ��ʱ */ 
//            case OS_R_TMO: 

//                break; 
//            /* ����ֵ������ */ 
//            default:                      
//                break; 
//		}
		DEBUG_Printf("\r\nAppTaskUartTx");
		//SensorDataReadCmdSend();
	    os_dly_wait(200);		//20��
	}
}

/* 
********************************************************************************************************* 
*  �� �� ��: AppTaskStart 
*  ����˵��: ��������Ҳ����������ȼ����� 
*  ��    ��: �� 
*  �� �� ֵ: �� 
*    �� �� ��: 2   
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
*  �� �� ��: RTX_OS_Init 
*  ����˵��: ����RTXϵͳ
*  ��    ��: �� 
*  �� �� ֵ: �� 
********************************************************************************************************* 
*/ 

 void RTX_OS_Init(void)
 {
 /* ������������ */ 
    os_sys_init_user (AppTaskStart,             /* ������ */ 
                    4,                        /* �������ȼ� */ 
                    &AppTaskStartStk,         /* ����ջ */ 
                    sizeof(AppTaskStartStk)); /* ����ջ��С����λ�ֽ��� */ 

 }
 
 
/* 
********************************************************************************************************* 
*  �� �� ��: AppTaskCreate 
*  ����˵��: ����Ӧ������ 
*  ��    ��: �� 
*  �� �� ֵ: �� 
********************************************************************************************************* 
*/ 
static void AppTaskCreate (void) 
{ 
//	HandleTaskLED = os_tsk_create_user(AppTaskLED,              /* ������ */  
//                                     4,                       /* �������ȼ� */  
//                                     &AppTaskLEDStk,          /* ����ջ */ 
//                                     sizeof(AppTaskLEDStk));  /* ����ջ��С����λ�ֽ��� */ 
									 
	HandleTaskWireless = os_tsk_create_user(AppTaskWireless,              /* ������ */  
                                     3,                       /* �������ȼ� */  
                                     &AppTaskWirelessStk,          /* ����ջ */ 
                                     sizeof(AppTaskWirelessStk));  /* ����ջ��С����λ�ֽ��� */ 
									 
	HandleTaskUartRx = os_tsk_create_user(AppTaskUartRx,              /* ������ */  
                                     2,                       /* �������ȼ� */  
                                     &AppTaskUartRxStk,          /* ����ջ */ 
                                     sizeof(AppTaskUartRxStk));  /* ����ջ��С����λ�ֽ��� */
									 
//	HandleTaskUartTx = os_tsk_create_user(AppTaskUartTx,              /* ������ */  
//                                     1,                       /* �������ȼ� */  
//                                     &AppTaskUartTxStk,          /* ����ջ */ 
//                                     sizeof(AppTaskUartTxStk));  /* ����ջ��С����λ�ֽ��� */								 

	/* �����ź�������ֵ�� 0, ��������ͬ�� */ 
	//os_sem_init (&semaphore, 0);  
	
									 
}

