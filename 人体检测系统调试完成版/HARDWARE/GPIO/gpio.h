/**********************************
作者：单片机俱乐部
网站：https://www.mcuclub.cn/
**********************************/


#ifndef __GPIO_H
#define __GPIO_H


/**********************************
包含头文件
**********************************/
#include "sys.h"


/**********************************
重定义关键词
**********************************/
#define LEDS_GPIO_PORT                		  GPIOC
#define LEDS_GPIO_PIN                 		  GPIO_Pin_13
#define LED_SYS                       		  PCout(13)

#define BEEP_GPIO_CLK                       RCC_APB2Periph_GPIOB				//蜂鸣器引脚
#define BEEP_PORT                           GPIOB
#define BEEP_PIN                            GPIO_Pin_1
#define BEEP 									    		      PBout(1)

#define LED_GPIO_CLK                        RCC_APB2Periph_GPIOB				//LED灯引脚
#define LED_PORT                            GPIOB
#define LED_PIN                             GPIO_Pin_0
#define LED 									    		      PBout(0)

#define RELAY_1_PORT                        GPIOB				                //加水继电器引脚
#define RELAY_1_PIN                         GPIO_Pin_14
#define RELAY_1 														PBout(14)

#define RELAY_2_PORT                        GPIOB                       //加热继电器引脚
#define RELAY_2_PIN                         GPIO_Pin_15
#define RELAY_2 														PBout(15)

#define MOTOR_A1_PORT                       GPIOB				                //电机引脚
#define MOTOR_A1_PIN                        GPIO_Pin_13
#define MOTOR_A1 														PBout(13)


/**********************************
函数声明
**********************************/
void Gpio_Init(void);													//GPIO初始化


#endif
