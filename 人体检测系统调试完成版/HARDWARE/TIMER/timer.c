/**********************************
作者：单片机俱乐部
网站：https://www.mcuclub.cn/
**********************************/


/**********************************
包含头文件
**********************************/
#include "timer.h"
#include "gpio.h"

/*
TIM2_Init(999,7199);			//定时器初始化函数
*/

/**********************************
变量定义
**********************************/
uint8_t flag_timer_begin_2s = 1;							//计时开始标志位
uint8_t flag_2s = 0;													//定时1s完成标志位
s16 pwm_value = 0;														//pwm值
_Bool flag_countdown_begin = 0;								//计时开始标志位
_Bool flag_1s = 0;														//定时1s完成标志位


/****
*******定时器初始化函数
*****/	
void TIM2_Init(u16 arr,u16 psc)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); 					//时钟使能
	
	//定时器初始化
	TIM_TimeBaseStructure.TIM_Period = arr; 											//设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; 										//设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; 			//设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  	//TIM向上计数模式
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure); 							//根据指定的参数初始化TIMx的时间基数单位

	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE ); 										//使能指定的定时器中断,允许更新中断

	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;  							//定时器中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  		//先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  					//从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 							//IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  															//初始化NVIC寄存器

	TIM_Cmd(TIM2, ENABLE);  																			//使能定时器					 
}

/****
*******定时器中断服务函数
*****/	
void TIM2_IRQHandler(void)
{
	static uint16_t time_value_2s = 0;
	static uint16_t pwm_count = 0;
	static uint16_t time_value_1s = 0;
	
	if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)  						//检查定时器更新中断发生与否
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);  								//清除定时器更新中断标志

		if(flag_timer_begin_2s == 1)																//开始计时
		{
			time_value_2s++;
			if(time_value_2s >= 300)  										  					//定时2s
			{
				time_value_2s = 0;
				flag_2s = 1;
			}
		}
		else
		{
			time_value_2s = 0;
			flag_2s = 0;
		}

		if(flag_countdown_begin == 1)							//开始计时
		{
			time_value_1s ++;
			if(time_value_1s >= 100)  							//定时1s
			{
				time_value_1s = 0;
				flag_1s = 1;
			}
		}
		else
		{
			time_value_1s = 0;
		}

		pwm_count ++;
		if(pwm_count >= 10)
		{
			pwm_count = 0;
		}
		if(pwm_count < pwm_value)
		{
			MOTOR_A1 = 1;
		}
		else
		{
			MOTOR_A1 = 0;
		}
	}
}

