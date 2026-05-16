/**********************************
作者：单片机俱乐部
网站：https://www.mcuclub.cn/
**********************************/


#ifndef _ESP8266_H_
#define _ESP8266_H_


/**********************************
重定义关键词
**********************************/
#define ESP8266_USART		USART2					//ESP8266串口

#define REV_OK		0											//接收完成标志
#define REV_WAIT	1											//接收未完成标志

extern unsigned char esp8266_buf[512];	//数据接受数组


/**********************************
函数声明
**********************************/
void ESP8266_Init(void);								//ESP8266初始化函数
void ESP8266_Clear(void);								//清空缓存
void ESP8266_SendData(unsigned char *data, unsigned short len);		//ESP8266发送数据
unsigned char *ESP8266_GetIPD(unsigned short timeOut);						//ESP8266获取平台返回的数据


#endif

