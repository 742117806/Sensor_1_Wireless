
//ʵ��������������Ʒ�Խ�Э��ӿ�
/*
�������ݸ�ʽ
������֤ʧ��
55 0C 31 03 01 00 00 FC E7 77 B0 9E CF 01 00 0E AA 
55 0C 31 03 01 00 00 FC E7 77 B0 9E CF 01 00 0E AA 
55 0C 31 03 01 00 00 FC E7 77 B0 9E CF 01 00 0E AA 
55 0C 31 03 01 00 00 FC E7 77 B0 9E CF 01 00 0E AA 
������֤�ɹ�
55 0C 31 03 01 00 00 FC E7 77 B0 9E CF 04 01 12 AA
*/




#include "uart.h"
#include "string.h"
#include "crc16.h"
#include "wireless_app.h"
#include "device.h"
//#include "mcu_eeprom.h"
#include "encrypt.h"
#include "wireless_app.h"
#include "wireless_drv.h"
#include "74.h"
#include "delay.h"

const uint8_t version[2]={01,00};		//�汾��



DeviceInfo_t deviceInfo=
{
	.aes = {0xA3,0xA6,0x89,0x26,0xAF,0xA7,0x13,0x29,0x33,0x0A,0xB1,0xA2,0x15,0xF8,0xFB,0xDB},
};

//���ĳ�ʼ��
void AES_Init(void)
{
	//��������ģ������aes_w�����ӽ�����
	memcpy(&aes_out[2*RsaByte_Size],deviceInfo.aes,16);
	memcpy(&aes_out[3*RsaByte_Size],deviceInfo.addr_GA,3);
	
	Rsa_Decode(aes_out);  
	key_expansion(aes_out, aes_w);  
}


/*
���MACû����¼���ȵ�MAC��¼
*/
void Device_MAC_Init(void)
{
	uint32_t delay_cnt = 0;

	DeviceInfoInit();
	while(1)
	{
		if(deviceInfo.mac_exist == 0x01)
		{
			break;
		}
		else
		{
			if(MAC_UartRec.state == UartRx_Finished)    
			{
				MAC_UartRec.state = UartRx_FrameHead;
				
				DeviceMAC_WriteProcess(MAC_UartRec.buff, MAC_UartRec.cnt);
				//DEBUT_SendBytes(lpuart1Rec.buff, lpuart1Rec.cnt);
			}
			delay_cnt ++;
			if(delay_cnt > 1500)
			{
                delay_cnt  = 0;
				UartSendData(USART2,0x0C);		//��ʾ�ַ�
			}
			delay_ms(1);
		}
	}
}



/*
���豸��Ϣ
*/
void DeviceInfoInit(void)
{
	STMFLASH_Read(DEVICE_INFO_FSADDR_START,(uint16_t*)&deviceInfo,(sizeof(deviceInfo)+1)/2);
//	deviceInfo.addr_GA[0]=0x00;
//	deviceInfo.addr_GA[1]=0x2A;
//	deviceInfo.addr_GA[2]=0x5B;
}                    

//���ݼ�ͥ���л����µĹ̶�ͨѶƵ����

void Get_WireLessChannel(uint8_t *wire_chnel)
{
    uint32_t temp_val = deviceInfo.addr_GA[0] + deviceInfo.addr_GA[1] + deviceInfo.addr_GA[2];
    if (temp_val == 0)
    {
        wire_chnel[0] = Default_Channel;
        wire_chnel[1] = Default_Channel;
    }
    else
    {
        wire_chnel[0] = (temp_val & 0x1f) << 1; //��32���ŵ��飬ÿ���ŵ����������ŵ�
        wire_chnel[1] = wire_chnel[0] + 1;
		if(wire_chnel[0] == Default_Channel)
		{
			wire_chnel[0] = wire_chnel[0] + 2;
			wire_chnel[1] = wire_chnel[0] + 1;
		} 
    }

    RF_RX_HOP_CONTROL_12[7] = Channel_Frequency_Index[wire_chnel[0]]; //����Ⱥ�����Ƶ��
    RF_RX_HOP_CONTROL_12[8] = Channel_Frequency_Index[wire_chnel[1]]; //����Ⱥ��ı���Ƶ��
}


//�Ӵ��ڽ��������������¼����ݽ���
//rec ����������
void SensorDataReadFromUart(uint8_t rec,UartRec_t *uartRecv)
{

	//if (uartRecv->rec_ok == 0)
	{
		switch ((sensor_uart_recv_state_e)uartRecv->state)
		{
		case SENSOR_FRAME_HEAD: //�ȴ�֡ͷ
			if (rec == SENSOR_FRAME_STAET)
			{
				uartRecv->state = SENSOR_FRAME_LEN;
				uartRecv->cnt = 0;
				uartRecv->buff[uartRecv->cnt++] = rec;
			}
			break;
		case SENSOR_FRAME_LEN:
			uartRecv->buff[uartRecv->cnt++] = rec;
			if(uartRecv->cnt == 8)		//�ӵ����ȵ�λ��
			{
				uartRecv->len = rec + 11; //֡���ݳ���+11=��һ֡����
				uartRecv->state = SENSOR_FRAME_DATA;
			}
			
			if(uartRecv->cnt > UART_RECV_BUFF_SIZE)			
			{
				uartRecv->state = SENSOR_FRAME_HEAD;
			}
			break;
		case SENSOR_FRAME_DATA:

			uartRecv->buff[uartRecv->cnt++] = rec;
			if(uartRecv->cnt > UART_RECV_BUFF_SIZE)	//�������ݳ��ȴ��ڻ�������С
			{
			   uartRecv->state = SENSOR_FRAME_HEAD;                    //���½�������
			}
			if (uartRecv->cnt >= uartRecv->len)
			{
				if (rec == SENSOR_FRAME_END)
				{
					//uartRecv->rec_ok = 1;
					uartRecv->state = SENSOR_FRAME_FENISH;	//�������
					
				}
				else
				{
					uartRecv->state = SENSOR_FRAME_HEAD;
				}
			}
			break;
		default:
			break;
		}
	}	
}

//�Ӵ��ڶ�ȡ����������������
//rec ����������
//void SensorDataReadFromUart(uint8_t rec)
//{

//	if(sensor_uart_rev.rev_ok == 0)
//	{
//		rec = rec;
//		if(rec == SENSOR_FRAME_STAET)
//		{
//			sensor_uart_rev.rev_cnt = 0;
//			sensor_uart_rev.buf[sensor_uart_rev.rev_cnt++] = rec;
//			sensor_uart_rev.rev_start = 1;		//��ʼ����
//		}
//		else if(sensor_uart_rev.rev_start == 1)
//		{

//			sensor_uart_rev.buf[sensor_uart_rev.rev_cnt++] = rec;
//			
//			if(sensor_uart_rev.rev_cnt == 29)//��֡���ݳ���
//			{
//				
//				if(rec == SENSOR_FRAME_END)
//				{
//					sensor_uart_rev.rev_ok = 1;
//					sensor_uart_rev.rev_len = sensor_uart_rev.rev_cnt;
//					sensor_uart_rev.rev_cnt = 0;
//					sensor_uart_rev.rev_start = 0;
//				}
//				else		// ���½���
//				{
//					sensor_uart_rev.rev_start = 0;		//��ʼ����
//					sensor_uart_rev.rev_cnt = 0;
//				}

//			}

//			
//		}
//		
//	}	
//}

/*
ͨ�����߷��������豸�¼�
*/
void DeviceEventSend(FRAME_CMD_t*frame,uint8_t* eventDat,uint8_t eventDatLen,uint8_t frameNum)
{
	uint16_t crc;
	uint8_t out_len;
    uint8_t frameLen; 

	 
	frame->FameHead = HKFreamHeader;
	frame->addr_DA = deviceInfo.addr_DA;
	memcpy(frame->addr_GA,deviceInfo.addr_GA,3);
	frame->Ctrl.dir = 1;				//��վ����
	frame->Ctrl.followUpFlag=0;		//�޺���֡
	frame->Ctrl.recAckFlag = 0;	//Ӧ����
	frame->Ctrl.relayFlag = 1;		//���м�
	frame->Ctrl.eventFlag = 1;		//��ͨ֡
	frame->Ctrl.c_AFN = 0;			//��74����
	frame->FSQ.frameNum = frameNum&0x0f;	//֡��ţ�0-15��
	frame->FSQ.encryptType = 1;		//����
	frame->FSQ.routeFlag = 0;		//����·�ɹ���
	frame->FSQ.ctrlField=0;			//������

	frame->DataLen = eventDatLen+4;	//���ݳ�������1�����ݹ��ܺ�3�����ݱ�ʶ
	frame->userData.AFN = 0x80;				//���ݹ�����
	frame->userData.Index[0] = 0x04;
	frame->userData.Index[1] = 0x00;
	frame->userData.Index[2] = 0x01;
	memcpy(frame->userData.content,eventDat,eventDatLen);
	crc = CRC16_2((uint8_t*)frame,frame->DataLen+11-3);//һ֡�����ܳ��ȼ�ȥ2���ֽڵ�CRC��һ��������
	frame->userData.content[eventDatLen] = (uint8_t)(crc>>8);
	frame->userData.content[eventDatLen+1] = (uint8_t)crc;
	frame->userData.content[eventDatLen+2] = HKFreamEnd;
	
	frameLen =  frame->DataLen+11;
	Encrypt_Convert((uint8_t*)frame, frameLen, &out_len, 1);		//����
	frameLen = out_len;											//�Ѽ��ܺ�����ݳ��ȸ�ֵ��ԭ�����ݵĳ���
	FrameData_74Convert((FRAME_CMD_t*)frame,frameLen,&out_len,1); //����
	frameLen = out_len;		//����󳤶�		
	
	Si4438_Transmit_Start(&Wireless_Buf, Wireless_Channel[0],(uint8_t*)frame,frameLen);
}




/*
����Ӧ��
*/
void WirelessRespoint(FRAME_CMD_t*frame,FRAME_CMD_t* respoint,uint8_t result,uint8_t *datPath,uint8_t datPath_len)
{

    static uint8_t frameNum = 0;
	uint16_t crc;
	uint8_t out_len;
	uint8_t frameLen;

	frameNum ++;
	respoint->FameHead = HKFreamHeader;
	respoint->addr_DA = deviceInfo.addr_DA;
	memcpy(respoint->addr_GA,deviceInfo.addr_GA,3);
	respoint->Ctrl.dir = 1;				//��վ����
	respoint->Ctrl.followUpFlag=0;		//�޺���֡
	respoint->Ctrl.recAckFlag = result;	//Ӧ����
	respoint->Ctrl.relayFlag = 1;		//���м�
	respoint->Ctrl.eventFlag = 0;		//��ͨ֡
	respoint->Ctrl.c_AFN = 0;			//��74����
	respoint->DataLen = datPath_len+4;	//���ݳ�������1�����ݹ��ܺ�3�����ݱ�ʶ
	respoint->userData.AFN = frame->userData.AFN;
	respoint->FSQ.frameNum = frame->FSQ.frameNum;
	respoint->FSQ.encryptType = 1;
	respoint->FSQ.routeFlag = 0;
	respoint->FSQ.ctrlField=frame->FSQ.ctrlField;
	memcpy(respoint->userData.Index,frame->userData.Index,3);
	memcpy(respoint->userData.content,datPath,respoint->DataLen);
	crc = CRC16_2((uint8_t*)respoint,respoint->DataLen+11-3);//һ֡�����ܳ��ȼ�ȥ2���ֽڵ�CRC��һ��������
	respoint->userData.content[datPath_len] = (uint8_t)(crc>>8);
	respoint->userData.content[datPath_len+1] = (uint8_t)crc;
	respoint->userData.content[datPath_len+2]=HKFreamEnd;
	
	frameLen =  respoint->DataLen+11;
	

	
	Encrypt_Convert((uint8_t*)respoint, frameLen, &out_len, 1);	//����
	frameLen = out_len;											//�Ѽ��ܺ�����ݳ��ȸ�ֵ��ԭ�����ݵĳ���
	
	FrameData_74Convert((FRAME_CMD_t*)respoint,frameLen,&out_len,1); //����
	frameLen = out_len;		//����󳤶�
	Si4438_Transmit_Start(&Wireless_Buf,Default_Channel, (uint8_t*)respoint, frameLen);	
    	
	
	
	
}


/*
�豸����
dat:mac+aes
*/
void DeviceJoinNet(FRAME_CMD_t *frame_cmd)
{
	uint8_t falg = 0;	//�ж��߼���ַ��AES����һ����һ������1
	FRAME_CMD_t repoint;
	uint16_t i=0;
	uint8_t ret;
	JOINE_NET_CMD_t *joine_net_cmd;
	SENSOR_JOINNET_DATA_t sensor_joinnet_repoint_content;

	joine_net_cmd = (JOINE_NET_CMD_t*)frame_cmd->userData.content;
	ret = memcmp(joine_net_cmd->mac,deviceInfo.mac,8);			//�Ƚ�����MAC�ڽ��յ���MAC�Ƿ�һ��
	if(ret == 0)		//���
	{
		ret = memcmp(deviceInfo.aes,joine_net_cmd->aes,16);
		if(ret != 0)		//�豸��aes����İ巢������aes�����
		{
			memcpy(deviceInfo.aes,joine_net_cmd->aes,16);			//��ȡ�豸��aes
			DEBUG_Printf("\r\n***********deviceInfo.aes******\r\n");
			for (i = 0; i < sizeof(deviceInfo.aes); i++)
			{
				DEBUG_Printf("%02X ", deviceInfo.aes[i]);
			}
			falg  = 1;
			//STMFLASH_Write(DEVICE_INFO_FSADDR_START,(uint16_t*)&deviceInfo,(sizeof(deviceInfo)+1)/2);		//�����豸��Ϣ aes����
		}
		else
		{
			DEBUG_Printf("\r\nDevice AES Not Cheng");
		}

		if((deviceInfo.addr_DA != frame_cmd->addr_DA)||
		(deviceInfo.addr_GA[0]!= frame_cmd->addr_GA[0])||
		(deviceInfo.addr_GA[1]!= frame_cmd->addr_GA[1])||
		(deviceInfo.addr_GA[2]!= frame_cmd->addr_GA[2]))
		{
			DEBUG_Printf("\r\nGroup Is Cheng");
			falg  = 1;
			deviceInfo.addr_DA=frame_cmd->addr_DA;					//�豸�߼���ַ
			memcpy(deviceInfo.addr_GA,frame_cmd->addr_GA,3);		//�豸Ⱥ���ַ

		}
		else
		{
			DEBUG_Printf("\r\nGroup Not Cheng");
		}
		
		if(falg  == 1)
		{

			STMFLASH_Write(DEVICE_INFO_FSADDR_START,(uint16_t*)&deviceInfo,(sizeof(deviceInfo)+1)/2);		//�����豸��Ϣ aes����
			Get_WireLessChannel(Wireless_Channel);	//��ȡ����ͨ����
			//��������ģ������aes_w�����ӽ�����
			AES_Init(); 
			Wireless_Init();
		}

		

		

		sensor_joinnet_repoint_content.version[0]= version[0];
		sensor_joinnet_repoint_content.version[1]= version[1];
		//����豸�������Ĺ���(���¶ȼ�⣬ʪ�ȼ�⣬PM2.5���)�����û����Щ���ܣ���ô�ⲿ������û��
		sensor_joinnet_repoint_content.PM2_5 = 1;
		sensor_joinnet_repoint_content.humidity = 1;
		sensor_joinnet_repoint_content.temperature = 1;
		WirelessRespoint(frame_cmd,&repoint,0x00,(uint8_t*)&sensor_joinnet_repoint_content,sizeof(sensor_joinnet_repoint_content));
		
		

	}
	else
	{
		DEBUG_Printf("\r\nMAC Error");
		DEBUG_Printf("\r\n*********Recv MAC********\r\n");
		for (i = 0; i < 8; i++)
		{
		    DEBUG_Printf("%02X ", joine_net_cmd->mac[i]);
		}
		
		
		DEBUG_Printf("\r\n*********Device MAC********\r\n");
		for (i = 0; i <8; i++)
		{
		    DEBUG_Printf("%02X ", deviceInfo.mac[i]);
		}
		
		
	}
}


//�����������ϱ�����
//frame���ϱ�����֡����ָ��
//eventDat���ϱ���������ָ��
//eventDatLen���ϱ����ݳ���
//frameNum������֡��
void SensorDataUpLoad(FRAME_CMD_t*frame,uint8_t* eventDat,uint8_t eventDatLen,uint8_t frameNum)
{
	
	uint16_t crc;
	uint8_t out_len,frame_len;		//���ܺ�����ĳ���
	
	
	frame->FameHead = HKFreamHeader;
	frame->addr_DA = deviceInfo.addr_DA;
	memcpy(frame->addr_GA,deviceInfo.addr_GA,3);

	
	frame->Ctrl.c_AFN = 0;		//���ƹ����룬0��վӦ��ʹ�ã�1������֡���� 7��74����
	frame->Ctrl.eventFlag = 1;	//�¼���־��0��ͨ֡��1�¼�֡
	frame->Ctrl.relayFlag = 1;	//�м̱�־��0����֡��1ת��֡
	frame->Ctrl.followUpFlag =0;		//����֡��־��0�޺���֡��1�к���֡
	frame->Ctrl.recAckFlag = 0;	//����վ����Ӧ���־��0��ȷӦ��1�쳣Ӧ��
	frame->Ctrl.dir = 1;		//���ͷ���0��վ������1��վ����
	
	
	frame->FSQ.frameNum = frameNum&0x0F;	//֡��ţ�0-15��
	//frame->FSQ.encryptType = 0;		//������
	frame->FSQ.encryptType = 1;		//����
	frame->FSQ.routeFlag = 0;		//����·�ɹ���
	frame->FSQ.ctrlField=0;			//������
	frame->DataLen = eventDatLen+4;	//���ݳ�������1�����ݹ��ܺ�3�����ݱ�ʶ
	frame->userData.AFN = 0;				//���ݹ�����
	frame->userData.Index[0] = 0x03;		//���ݱ�ʶ��03 FF FF�����������ݿ��ʶ
	frame->userData.Index[1] = 0xFF;
	frame->userData.Index[2] = 0xFF;
	memcpy(frame->userData.content,eventDat,eventDatLen);
	crc = CRC16_2((uint8_t*)frame,frame->DataLen+11-3);//һ֡�����ܳ��ȼ�ȥ2���ֽڵ�CRC��һ��������
	frame->userData.content[eventDatLen] = (uint8_t)(crc>>8);
	frame->userData.content[eventDatLen+1] = (uint8_t)crc;
	frame->userData.content[eventDatLen+2] = HKFreamEnd;
	
	frame_len = frame->DataLen+11;
	
	Encrypt_Convert((uint8_t*)frame, frame_len, &out_len, 1);
	frame_len = out_len;
	FrameData_74Convert((FRAME_CMD_t *)frame,frame_len,&out_len,1);
	frame_len = out_len;
	
	Si4438_Transmit_Start(&Wireless_Buf, Wireless_Channel[0],(uint8_t*)frame, frame_len);		//����
	DEBUG_SendBytes((uint8_t*)frame,out_len);
}


/***********************************************************
���ܣ����������ݴ���
������@recvData ���ڽ��յ�����
***********************************************************/
void SensorProcess(uint8_t *recvData)
{
	static uint8_t frameNum;
	
	uint8_t frame_cmd[256];
	
	SENSOR_DATA_CMD_t  *sensor_cmd;
	
	SENSOR_UPLOAD_DATA_t uploadData;		//�ϱ�������
	
	sensor_cmd = (SENSOR_DATA_CMD_t*)recvData;
	
	/*
	�Ѵ��������ݴ�����ϲ���Ҫ�ĸ�ʽ
	*/
	uploadData.num = 3;					//һ����3������������
	memcpy(uploadData.pm2_5_index,sensor_cmd->pm2_5_index,6);		//6�����ݰ�������������ʶ������
	memcpy(uploadData.hum_index,sensor_cmd->hum_index,6);			//6�����ݰ�������������ʶ������
	memcpy(uploadData.temp_index,sensor_cmd->temp_index,6);		//6�����ݰ�������������ʶ������
		
	SensorDataUpLoad((FRAME_CMD_t*)frame_cmd,(uint8_t*)&uploadData,sizeof(uploadData),frameNum++);

}


//���Ͷ�ȡ���������ݴ�������
void SensorDataReadCmdSend(void)
{
	uint8_t cmd_buf[14]={0xAC,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x03,0xFF,0xFF,0x42,0xCA,0x53};
	UartSendBytes(USART2,cmd_buf,14);
	
}


//��ʱ��ѯ����������
void SensorDataReadTimer(void)
{
	static uint32_t new_t,old_t;		//��ǰʱ�䣬�ϴ�ʱ��
	uint32_t v_t;						//ʱ����			
	new_t = HAL_GetTick();
	
	if(new_t>old_t)
	{
		v_t = new_t - old_t;
	}
	else
	{
		v_t = old_t - new_t;
	}
	if(v_t > 10000)			//10 ��
	{
		v_t = 0;
		old_t = new_t;
		SensorDataReadCmdSend();
	}
}

