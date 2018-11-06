
//实现与智能门锁产品对接协议接口
/*
门锁数据格式
开门验证失败
55 0C 31 03 01 00 00 FC E7 77 B0 9E CF 01 00 0E AA 
55 0C 31 03 01 00 00 FC E7 77 B0 9E CF 01 00 0E AA 
55 0C 31 03 01 00 00 FC E7 77 B0 9E CF 01 00 0E AA 
55 0C 31 03 01 00 00 FC E7 77 B0 9E CF 01 00 0E AA 
开门验证成功
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

const uint8_t version[2]={01,00};		//版本号



DeviceInfo_t deviceInfo=
{
	.aes = {0xA3,0xA6,0x89,0x26,0xAF,0xA7,0x13,0x29,0x33,0x0A,0xB1,0xA2,0x15,0xF8,0xFB,0xDB},
};

//密文初始化
void AES_Init(void)
{
	//计算出密文，存放在aes_w，供加解密用
	memcpy(&aes_out[2*RsaByte_Size],deviceInfo.aes,16);
	memcpy(&aes_out[3*RsaByte_Size],deviceInfo.addr_GA,3);
	
	Rsa_Decode(aes_out);  
	key_expansion(aes_out, aes_w);  
}


/*
如果MAC没有烧录，等等MAC烧录
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
				UartSendData(USART2,0x0C);		//提示字符
			}
			delay_ms(1);
		}
	}
}



/*
读设备信息
*/
void DeviceInfoInit(void)
{
	STMFLASH_Read(DEVICE_INFO_FSADDR_START,(uint16_t*)&deviceInfo,(sizeof(deviceInfo)+1)/2);
//	deviceInfo.addr_GA[0]=0x00;
//	deviceInfo.addr_GA[1]=0x2A;
//	deviceInfo.addr_GA[2]=0x5B;
}                    

//根据家庭组切换到新的固定通讯频道上

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
        wire_chnel[0] = (temp_val & 0x1f) << 1; //共32个信道组，每个信道组有两个信道
        wire_chnel[1] = wire_chnel[0] + 1;
		if(wire_chnel[0] == Default_Channel)
		{
			wire_chnel[0] = wire_chnel[0] + 2;
			wire_chnel[1] = wire_chnel[0] + 1;
		} 
    }

    RF_RX_HOP_CONTROL_12[7] = Channel_Frequency_Index[wire_chnel[0]]; //放入群组的主频道
    RF_RX_HOP_CONTROL_12[8] = Channel_Frequency_Index[wire_chnel[1]]; //放入群组的备用频道
}


//从串口接收门锁发来的事件数据接收
//rec ：串口数据
void SensorDataReadFromUart(uint8_t rec,UartRec_t *uartRecv)
{

	//if (uartRecv->rec_ok == 0)
	{
		switch ((sensor_uart_recv_state_e)uartRecv->state)
		{
		case SENSOR_FRAME_HEAD: //等待帧头
			if (rec == SENSOR_FRAME_STAET)
			{
				uartRecv->state = SENSOR_FRAME_LEN;
				uartRecv->cnt = 0;
				uartRecv->buff[uartRecv->cnt++] = rec;
			}
			break;
		case SENSOR_FRAME_LEN:
			uartRecv->buff[uartRecv->cnt++] = rec;
			if(uartRecv->cnt == 8)		//接到长度的位置
			{
				uartRecv->len = rec + 11; //帧数据长度+11=整一帧长度
				uartRecv->state = SENSOR_FRAME_DATA;
			}
			
			if(uartRecv->cnt > UART_RECV_BUFF_SIZE)			
			{
				uartRecv->state = SENSOR_FRAME_HEAD;
			}
			break;
		case SENSOR_FRAME_DATA:

			uartRecv->buff[uartRecv->cnt++] = rec;
			if(uartRecv->cnt > UART_RECV_BUFF_SIZE)	//接收数据长度大于缓存区大小
			{
			   uartRecv->state = SENSOR_FRAME_HEAD;                    //重新接收数据
			}
			if (uartRecv->cnt >= uartRecv->len)
			{
				if (rec == SENSOR_FRAME_END)
				{
					//uartRecv->rec_ok = 1;
					uartRecv->state = SENSOR_FRAME_FENISH;	//接收完成
					
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

//从串口读取传感器传来的数据
//rec ：串口数据
//void SensorDataReadFromUart(uint8_t rec)
//{

//	if(sensor_uart_rev.rev_ok == 0)
//	{
//		rec = rec;
//		if(rec == SENSOR_FRAME_STAET)
//		{
//			sensor_uart_rev.rev_cnt = 0;
//			sensor_uart_rev.buf[sensor_uart_rev.rev_cnt++] = rec;
//			sensor_uart_rev.rev_start = 1;		//开始接收
//		}
//		else if(sensor_uart_rev.rev_start == 1)
//		{

//			sensor_uart_rev.buf[sensor_uart_rev.rev_cnt++] = rec;
//			
//			if(sensor_uart_rev.rev_cnt == 29)//整帧数据长度
//			{
//				
//				if(rec == SENSOR_FRAME_END)
//				{
//					sensor_uart_rev.rev_ok = 1;
//					sensor_uart_rev.rev_len = sensor_uart_rev.rev_cnt;
//					sensor_uart_rev.rev_cnt = 0;
//					sensor_uart_rev.rev_start = 0;
//				}
//				else		// 重新接收
//				{
//					sensor_uart_rev.rev_start = 0;		//开始接收
//					sensor_uart_rev.rev_cnt = 0;
//				}

//			}

//			
//		}
//		
//	}	
//}

/*
通过无线发送门锁设备事件
*/
void DeviceEventSend(FRAME_CMD_t*frame,uint8_t* eventDat,uint8_t eventDatLen,uint8_t frameNum)
{
	uint16_t crc;
	uint8_t out_len;
    uint8_t frameLen; 

	 
	frame->FameHead = HKFreamHeader;
	frame->addr_DA = deviceInfo.addr_DA;
	memcpy(frame->addr_GA,deviceInfo.addr_GA,3);
	frame->Ctrl.dir = 1;				//从站发出
	frame->Ctrl.followUpFlag=0;		//无后续帧
	frame->Ctrl.recAckFlag = 0;	//应答结果
	frame->Ctrl.relayFlag = 1;		//无中继
	frame->Ctrl.eventFlag = 1;		//普通帧
	frame->Ctrl.c_AFN = 0;			//带74编码
	frame->FSQ.frameNum = frameNum&0x0f;	//帧序号（0-15）
	frame->FSQ.encryptType = 1;		//加密
	frame->FSQ.routeFlag = 0;		//不是路由功能
	frame->FSQ.ctrlField=0;			//控制域

	frame->DataLen = eventDatLen+4;	//数据长度中有1个数据功能和3个数据标识
	frame->userData.AFN = 0x80;				//数据功能码
	frame->userData.Index[0] = 0x04;
	frame->userData.Index[1] = 0x00;
	frame->userData.Index[2] = 0x01;
	memcpy(frame->userData.content,eventDat,eventDatLen);
	crc = CRC16_2((uint8_t*)frame,frame->DataLen+11-3);//一帧数据总长度减去2个字节的CRC和一个结束符
	frame->userData.content[eventDatLen] = (uint8_t)(crc>>8);
	frame->userData.content[eventDatLen+1] = (uint8_t)crc;
	frame->userData.content[eventDatLen+2] = HKFreamEnd;
	
	frameLen =  frame->DataLen+11;
	Encrypt_Convert((uint8_t*)frame, frameLen, &out_len, 1);		//加密
	frameLen = out_len;											//把加密后的数据长度赋值给原来数据的长度
	FrameData_74Convert((FRAME_CMD_t*)frame,frameLen,&out_len,1); //编码
	frameLen = out_len;		//编码后长度		
	
	Si4438_Transmit_Start(&Wireless_Buf, Wireless_Channel[0],(uint8_t*)frame,frameLen);
}




/*
无线应答
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
	respoint->Ctrl.dir = 1;				//从站发出
	respoint->Ctrl.followUpFlag=0;		//无后续帧
	respoint->Ctrl.recAckFlag = result;	//应答结果
	respoint->Ctrl.relayFlag = 1;		//无中继
	respoint->Ctrl.eventFlag = 0;		//普通帧
	respoint->Ctrl.c_AFN = 0;			//有74编码
	respoint->DataLen = datPath_len+4;	//数据长度中有1个数据功能和3个数据标识
	respoint->userData.AFN = frame->userData.AFN;
	respoint->FSQ.frameNum = frame->FSQ.frameNum;
	respoint->FSQ.encryptType = 1;
	respoint->FSQ.routeFlag = 0;
	respoint->FSQ.ctrlField=frame->FSQ.ctrlField;
	memcpy(respoint->userData.Index,frame->userData.Index,3);
	memcpy(respoint->userData.content,datPath,respoint->DataLen);
	crc = CRC16_2((uint8_t*)respoint,respoint->DataLen+11-3);//一帧数据总长度减去2个字节的CRC和一个结束符
	respoint->userData.content[datPath_len] = (uint8_t)(crc>>8);
	respoint->userData.content[datPath_len+1] = (uint8_t)crc;
	respoint->userData.content[datPath_len+2]=HKFreamEnd;
	
	frameLen =  respoint->DataLen+11;
	

	
	Encrypt_Convert((uint8_t*)respoint, frameLen, &out_len, 1);	//加密
	frameLen = out_len;											//把加密后的数据长度赋值给原来数据的长度
	
	FrameData_74Convert((FRAME_CMD_t*)respoint,frameLen,&out_len,1); //编码
	frameLen = out_len;		//编码后长度
	Si4438_Transmit_Start(&Wireless_Buf,Default_Channel, (uint8_t*)respoint, frameLen);	
    	
	
	
	
}


/*
设备入网
dat:mac+aes
*/
void DeviceJoinNet(FRAME_CMD_t *frame_cmd)
{
	uint8_t falg = 0;	//判断逻辑地址或AES其中一个不一样都置1
	FRAME_CMD_t repoint;
	uint16_t i=0;
	uint8_t ret;
	JOINE_NET_CMD_t *joine_net_cmd;
	SENSOR_JOINNET_DATA_t sensor_joinnet_repoint_content;

	joine_net_cmd = (JOINE_NET_CMD_t*)frame_cmd->userData.content;
	ret = memcmp(joine_net_cmd->mac,deviceInfo.mac,8);			//比较自身MAC于接收到的MAC是否一样
	if(ret == 0)		//相等
	{
		ret = memcmp(deviceInfo.aes,joine_net_cmd->aes,16);
		if(ret != 0)		//设备的aes与核心板发过来的aes不相等
		{
			memcpy(deviceInfo.aes,joine_net_cmd->aes,16);			//获取设备的aes
			DEBUG_Printf("\r\n***********deviceInfo.aes******\r\n");
			for (i = 0; i < sizeof(deviceInfo.aes); i++)
			{
				DEBUG_Printf("%02X ", deviceInfo.aes[i]);
			}
			falg  = 1;
			//STMFLASH_Write(DEVICE_INFO_FSADDR_START,(uint16_t*)&deviceInfo,(sizeof(deviceInfo)+1)/2);		//保存设备信息 aes长度
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
			deviceInfo.addr_DA=frame_cmd->addr_DA;					//设备逻辑地址
			memcpy(deviceInfo.addr_GA,frame_cmd->addr_GA,3);		//设备群组地址

		}
		else
		{
			DEBUG_Printf("\r\nGroup Not Cheng");
		}
		
		if(falg  == 1)
		{

			STMFLASH_Write(DEVICE_INFO_FSADDR_START,(uint16_t*)&deviceInfo,(sizeof(deviceInfo)+1)/2);		//保存设备信息 aes长度
			Get_WireLessChannel(Wireless_Channel);	//获取无线通道号
			//计算出密文，存放在aes_w，供加解密用
			AES_Init(); 
			Wireless_Init();
		}

		

		

		sensor_joinnet_repoint_content.version[0]= version[0];
		sensor_joinnet_repoint_content.version[1]= version[1];
		//添加设备本身附带的功能(如温度检测，湿度检测，PM2.5检测)，如果没有这些功能，那么这部分数据没有
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


//传感器被动上报数据
//frame：上报命令帧数组指针
//eventDat：上报内容数组指针
//eventDatLen：上报内容长度
//frameNum：发送帧号
void SensorDataUpLoad(FRAME_CMD_t*frame,uint8_t* eventDat,uint8_t eventDatLen,uint8_t frameNum)
{
	
	uint16_t crc;
	uint8_t out_len,frame_len;		//加密后输出的长度
	
	
	frame->FameHead = HKFreamHeader;
	frame->addr_DA = deviceInfo.addr_DA;
	memcpy(frame->addr_GA,deviceInfo.addr_GA,3);

	
	frame->Ctrl.c_AFN = 0;		//控制功能码，0从站应答使用，1读后续帧数据 7无74编码
	frame->Ctrl.eventFlag = 1;	//事件标志，0普通帧，1事件帧
	frame->Ctrl.relayFlag = 1;	//中继标志，0本地帧，1转发帧
	frame->Ctrl.followUpFlag =0;		//后续帧标志，0无后续帧，1有后续帧
	frame->Ctrl.recAckFlag = 0;	//接收站接收应答标志，0正确应答，1异常应答
	frame->Ctrl.dir = 1;		//传送方向，0主站发出，1从站发出
	
	
	frame->FSQ.frameNum = frameNum&0x0F;	//帧序号（0-15）
	//frame->FSQ.encryptType = 0;		//不加密
	frame->FSQ.encryptType = 1;		//加密
	frame->FSQ.routeFlag = 0;		//不是路由功能
	frame->FSQ.ctrlField=0;			//控制域
	frame->DataLen = eventDatLen+4;	//数据长度中有1个数据功能和3个数据标识
	frame->userData.AFN = 0;				//数据功能码
	frame->userData.Index[0] = 0x03;		//数据标识（03 FF FF）传感器数据块标识
	frame->userData.Index[1] = 0xFF;
	frame->userData.Index[2] = 0xFF;
	memcpy(frame->userData.content,eventDat,eventDatLen);
	crc = CRC16_2((uint8_t*)frame,frame->DataLen+11-3);//一帧数据总长度减去2个字节的CRC和一个结束符
	frame->userData.content[eventDatLen] = (uint8_t)(crc>>8);
	frame->userData.content[eventDatLen+1] = (uint8_t)crc;
	frame->userData.content[eventDatLen+2] = HKFreamEnd;
	
	frame_len = frame->DataLen+11;
	
	Encrypt_Convert((uint8_t*)frame, frame_len, &out_len, 1);
	frame_len = out_len;
	FrameData_74Convert((FRAME_CMD_t *)frame,frame_len,&out_len,1);
	frame_len = out_len;
	
	Si4438_Transmit_Start(&Wireless_Buf, Wireless_Channel[0],(uint8_t*)frame, frame_len);		//加密
	DEBUG_SendBytes((uint8_t*)frame,out_len);
}


/***********************************************************
功能：传感器数据处理
参数：@recvData 串口接收的数据
***********************************************************/
void SensorProcess(uint8_t *recvData)
{
	static uint8_t frameNum;
	
	uint8_t frame_cmd[256];
	
	SENSOR_DATA_CMD_t  *sensor_cmd;
	
	SENSOR_UPLOAD_DATA_t uploadData;		//上报数据区
	
	sensor_cmd = (SENSOR_DATA_CMD_t*)recvData;
	
	/*
	把传感器数据打包成上层需要的格式
	*/
	uploadData.num = 3;					//一共有3个传感器数据
	memcpy(uploadData.pm2_5_index,sensor_cmd->pm2_5_index,6);		//6个数据包括传感器数标识和数据
	memcpy(uploadData.hum_index,sensor_cmd->hum_index,6);			//6个数据包括传感器数标识和数据
	memcpy(uploadData.temp_index,sensor_cmd->temp_index,6);		//6个数据包括传感器数标识和数据
		
	SensorDataUpLoad((FRAME_CMD_t*)frame_cmd,(uint8_t*)&uploadData,sizeof(uploadData),frameNum++);

}


//发送读取传感器数据串口命令
void SensorDataReadCmdSend(void)
{
	uint8_t cmd_buf[14]={0xAC,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x03,0xFF,0xFF,0x42,0xCA,0x53};
	UartSendBytes(USART2,cmd_buf,14);
	
}


//定时查询传感器数据
void SensorDataReadTimer(void)
{
	static uint32_t new_t,old_t;		//当前时间，上次时间
	uint32_t v_t;						//时间间隔			
	new_t = HAL_GetTick();
	
	if(new_t>old_t)
	{
		v_t = new_t - old_t;
	}
	else
	{
		v_t = old_t - new_t;
	}
	if(v_t > 10000)			//10 秒
	{
		v_t = 0;
		old_t = new_t;
		SensorDataReadCmdSend();
	}
}

