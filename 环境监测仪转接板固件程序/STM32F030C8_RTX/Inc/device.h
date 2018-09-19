#ifndef __DEVICE_H
#define __DEVICE_H
#include "stm32f0xx.h"
#include "protocol.h"
#include "stmflash.h"


#define SENSOR_FRAME_STAET	0xAC
#define SENSOR_FRAME_END	0x53

/*
设备信息存储区地址定义
*/
#define DEVICE_INFO_FSADDR_START	(STM32_FLASH_END-2048)
#define DEVICE_MAC_EXSIT_FSADDR		(DEVICE_INFO_FSADDR_START)		//从后面2K开始做存储区
#define DEVICE_MAC_FSADDR			(DEVICE_MAC_EXSIT_FSADDR+1)
#define DEVICE_AES_FSADDR			(DEVICE_MAC_FSADDR+8)
#define DEVICE_ADDRDA_FSADDR		(DEVICE_AES_FSADDR+16)
#define DEVICE_ADDRGA_FSADDR		(DEVICE_ADDRDA_FSADDR+1)
#define DEVICE_INFO_FSADDR_END		    (DEVICE_ADDRGA_FSADDR+3)

/*
设备信息
*/
typedef struct DeviceInfo_
{
	uint8_t mac_exist;		//标识设备MAC是否已经烧录
	uint8_t mac[8];			//设备MAC地址
	uint8_t aes[16];		//密钥
	uint8_t addr_DA;		//逻辑地址
	uint8_t addr_GA[3];		//群众地址
}DeviceInfo_t;



//门锁串口接收状态枚举
typedef enum Sensor_uart_recv_state_
{
    SENSOR_FRAME_HEAD,		//帧头
	SENSOR_FRAME_LEN,     	//数据长度
	SENSOR_FRAME_DATA,		//数据
	SENSOR_FRAME_FENISH,	//接收完成
	
}sensor_uart_recv_state_e;


//门锁数据区结构
typedef struct DOOR_DATA_
{
	uint8_t ctrlType;		//操作类别：01 增加，02 删除 03 验证 04 保留
	uint8_t userType;       //用户类别：01 指纹 02 密码 03 卡 04 手机 05 保留
    uint8_t door_ID[8];     //锁 ID：0000000000000000~9999999999999999
    uint8_t userNum;        //用户编号：00~99 和锁 ID 相关。HEX 码
    uint8_t state;          //状态：00 失败，01 成功
}DOOR_DATA_t;
//门锁协议帧结构
typedef struct DOOR_CMD_
{
	uint8_t FrameHead;
	uint8_t DataLen;		//数据区的字节数
	uint8_t CmdFunc;		//功能命令(目前是固定0x31)
	DOOR_DATA_t dataPath;		//数据区
	uint8_t crc_sum;			//crc 是起始到crc前面的校验和
	uint8_t FrameEnd;           
}DOOR_CMD_t;


//门锁注册入网时数据区结构(数据标识后面的内容)
typedef struct LOCK_JOINNET_DATA_
{
	uint8_t version[2];		//版本号
	
}LOCK_JOINNET_DATA__t;



//传感器发送回来的帧命令数据结构
typedef struct SENSOR_DATA_CMD_
{
    uint8_t frame_start;		//数据头
	uint8_t NC_6[6];			//6个无关数据
	uint8_t len;				//长度
	uint8_t pm2_5_index[3];		//pm2_5标识
	uint8_t pm2_5_data[3];		//pm2_5数据
	uint8_t temp_index[3];		//温度
	uint8_t temp_data[3];
	uint8_t	hum_index[3];		//相对湿度
	uint8_t hum_data[3];
	uint8_t crc16[2];
	uint8_t frame_end;			//数据结束		
}SENSOR_DATA_CMD_t;


//接收串口传感器数据结构体
typedef struct SENSOR_UART_REV_
{
	uint8_t rev_len;			//接收长度
	uint8_t rev_cnt:6;			//接收计算
	uint8_t rev_start:1;		//开始接收标志
	uint8_t rev_ok:1;			//接收完成标志
	uint8_t buf[32];			//接收缓存
}SENSOR_UART_REV_t;

//传感器上报数据区数据结构
typedef struct SENSOR_UPLOAD_DATA_
{
	uint8_t num;			//传感器个数
	uint8_t pm2_5_index[3];		//pm2_5标识
	uint8_t pm2_5_data[3];		//pm2_5数据
	uint8_t	hum_index[3];		//相对湿度
	uint8_t hum_data[3];
	uint8_t temp_index[3];		//温度
	uint8_t temp_data[3];
}SENSOR_UPLOAD_DATA_t;


//传感器注册入网时数据区结构(数据标识后面的内容)
typedef struct SENSOR_JOINNET_DATA_
{
	uint8_t version[2];		//版本号
	uint8_t no_2;
	uint8_t infrared:1;		//红外感应	
	uint8_t no_1:7;			//预留
	uint8_t PM2_5:1;		//PM2.5
	uint8_t HCHO:1;			//甲醛
	uint8_t CO2:1;			//二氧化碳
	uint8_t TVOC:1;			//总挥发有机物
	uint8_t humidity:1;		//湿度
	uint8_t temperature:1;	//温度
	uint8_t wind_power:1; 	//风力
	uint8_t rainfall:1;		//雨量
	
}SENSOR_JOINNET_DATA_t;


extern DeviceInfo_t deviceInfo;


void DoorLockDataTask(void);
void SensorDataReadFromUart(uint8_t rec,UartRec_t *uartRecv);
void DeviceEventSend(FRAME_CMD_t*frame,uint8_t* eventDat,uint8_t eventDatLen,uint8_t frameNum);
void DeviceInfoInit(void);
void DeviceJoinNet(FRAME_CMD_t *frame_cmd);
void SensorDataReadCmdSend(void);
void SensorProcess(uint8_t *recvData);
void Device_MAC_Init(void);


#endif



