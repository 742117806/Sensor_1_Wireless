#ifndef __DEVICE_H
#define __DEVICE_H
#include "stm32f0xx.h"
#include "protocol.h"
#include "stmflash.h"


#define SENSOR_FRAME_STAET	0xAC
#define SENSOR_FRAME_END	0x53

/*
�豸��Ϣ�洢����ַ����
*/
#define DEVICE_INFO_FSADDR_START	(STM32_FLASH_END-2048)
#define DEVICE_MAC_EXSIT_FSADDR		(DEVICE_INFO_FSADDR_START)		//�Ӻ���2K��ʼ���洢��
#define DEVICE_MAC_FSADDR			(DEVICE_MAC_EXSIT_FSADDR+1)
#define DEVICE_AES_FSADDR			(DEVICE_MAC_FSADDR+8)
#define DEVICE_ADDRDA_FSADDR		(DEVICE_AES_FSADDR+16)
#define DEVICE_ADDRGA_FSADDR		(DEVICE_ADDRDA_FSADDR+1)
#define DEVICE_INFO_FSADDR_END		    (DEVICE_ADDRGA_FSADDR+3)

/*
�豸��Ϣ
*/
typedef struct DeviceInfo_
{
	uint8_t mac_exist;		//��ʶ�豸MAC�Ƿ��Ѿ���¼
	uint8_t mac[8];			//�豸MAC��ַ
	uint8_t aes[16];		//��Կ
	uint8_t addr_DA;		//�߼���ַ
	uint8_t addr_GA[3];		//Ⱥ�ڵ�ַ
}DeviceInfo_t;



//�������ڽ���״̬ö��
typedef enum Sensor_uart_recv_state_
{
    SENSOR_FRAME_HEAD,		//֡ͷ
	SENSOR_FRAME_LEN,     	//���ݳ���
	SENSOR_FRAME_DATA,		//����
	SENSOR_FRAME_FENISH,	//�������
	
}sensor_uart_recv_state_e;


//�����������ṹ
typedef struct DOOR_DATA_
{
	uint8_t ctrlType;		//�������01 ���ӣ�02 ɾ�� 03 ��֤ 04 ����
	uint8_t userType;       //�û����01 ָ�� 02 ���� 03 �� 04 �ֻ� 05 ����
    uint8_t door_ID[8];     //�� ID��0000000000000000~9999999999999999
    uint8_t userNum;        //�û���ţ�00~99 ���� ID ��ء�HEX ��
    uint8_t state;          //״̬��00 ʧ�ܣ�01 �ɹ�
}DOOR_DATA_t;
//����Э��֡�ṹ
typedef struct DOOR_CMD_
{
	uint8_t FrameHead;
	uint8_t DataLen;		//���������ֽ���
	uint8_t CmdFunc;		//��������(Ŀǰ�ǹ̶�0x31)
	DOOR_DATA_t dataPath;		//������
	uint8_t crc_sum;			//crc ����ʼ��crcǰ���У���
	uint8_t FrameEnd;           
}DOOR_CMD_t;


//����ע������ʱ�������ṹ(���ݱ�ʶ���������)
typedef struct LOCK_JOINNET_DATA_
{
	uint8_t version[2];		//�汾��
	
}LOCK_JOINNET_DATA__t;



//���������ͻ�����֡�������ݽṹ
typedef struct SENSOR_DATA_CMD_
{
    uint8_t frame_start;		//����ͷ
	uint8_t NC_6[6];			//6���޹�����
	uint8_t len;				//����
	uint8_t pm2_5_index[3];		//pm2_5��ʶ
	uint8_t pm2_5_data[3];		//pm2_5����
	uint8_t temp_index[3];		//�¶�
	uint8_t temp_data[3];
	uint8_t	hum_index[3];		//���ʪ��
	uint8_t hum_data[3];
	uint8_t crc16[2];
	uint8_t frame_end;			//���ݽ���		
}SENSOR_DATA_CMD_t;


//���մ��ڴ��������ݽṹ��
typedef struct SENSOR_UART_REV_
{
	uint8_t rev_len;			//���ճ���
	uint8_t rev_cnt:6;			//���ռ���
	uint8_t rev_start:1;		//��ʼ���ձ�־
	uint8_t rev_ok:1;			//������ɱ�־
	uint8_t buf[32];			//���ջ���
}SENSOR_UART_REV_t;

//�������ϱ����������ݽṹ
typedef struct SENSOR_UPLOAD_DATA_
{
	uint8_t num;			//����������
	uint8_t pm2_5_index[3];		//pm2_5��ʶ
	uint8_t pm2_5_data[3];		//pm2_5����
	uint8_t	hum_index[3];		//���ʪ��
	uint8_t hum_data[3];
	uint8_t temp_index[3];		//�¶�
	uint8_t temp_data[3];
}SENSOR_UPLOAD_DATA_t;


//������ע������ʱ�������ṹ(���ݱ�ʶ���������)
typedef struct SENSOR_JOINNET_DATA_
{
	uint8_t version[2];		//�汾��
	uint8_t no_2;
	uint8_t infrared:1;		//�����Ӧ	
	uint8_t no_1:7;			//Ԥ��
	uint8_t PM2_5:1;		//PM2.5
	uint8_t HCHO:1;			//��ȩ
	uint8_t CO2:1;			//������̼
	uint8_t TVOC:1;			//�ܻӷ��л���
	uint8_t humidity:1;		//ʪ��
	uint8_t temperature:1;	//�¶�
	uint8_t wind_power:1; 	//����
	uint8_t rainfall:1;		//����
	
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



