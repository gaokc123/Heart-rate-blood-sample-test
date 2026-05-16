/**********************************
作者：单片机俱乐部
网站：https://www.mcuclub.cn/
**********************************/


#ifndef _ALIYUN_H_
#define _ALIYUN_H_


/**********************************
函数声明
**********************************/
_Bool Aliyun_DevLink(void);																								//与onenet创建连接
void Aliyun_Subscribe(const char *topics[], unsigned char topic_cnt);			//订阅
void Aliyun_Publish(const char *topic, const char *msg);									//发布消息
void Aliyun_RevPro(unsigned char *cmd);																		//平台返回数据检测


#endif

