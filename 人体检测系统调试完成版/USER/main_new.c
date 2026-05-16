#include "sys.h"
#include "stdio.h"
#include "string.h"
#include "delay.h"
#include "gpio.h"
#include "key.h"
#include "oled.h"
#include "usart.h"
#include "timer.h"
#include "ds18b20.h"
#include "max30102.h"
#include "algorithm.h"  // 官方算法库

/*******************************************************************
 * 全局变量说明
 * system_state: 系统状态
 *      0 = 正常运行模式
 *      1 = 参数设置模式
 *      2 = 睡眠模式
 * work_mode: 工作模式
 *      0 = 本地模式
 *      1 = 遥控模式
 *      2 = 自动模式(默认)
 ******************************************************************/

// ===== 旧的全局变量 =====
uint8_t key_num = 0;				// 按键值存储
uint8_t flag_display = 0;			// 显示菜单页面标志
uint32_t time_num = 0;				// 时计数器,用于周期采集/上传
char display_buf[32] = {0};			// OLED 显示缓冲

uint8_t system_state = 0;			// 系统状态:0正常 1设置 2睡眠
uint8_t work_mode = 2;				// 工作模式:0本地 1遥控 2自动(默认)

short temp_value = 0;				// 温度原始数据
uint8_t heart_rate = 75;			// 心率值
uint8_t spo2 = 98;					// 血氧值

// 报警阈值(用户通过按键/蓝牙设置)
float temp_max   = 38.0f;
float temp_min   = 35.0f;
uint8_t heart_max= 120;
uint8_t heart_min= 80;
uint8_t spo2_max = 100;
uint8_t spo2_min = 60;

// 报警标志位
_Bool alarm_temp  = 0;
_Bool alarm_heart = 0;
_Bool alarm_spo2  = 0;
_Bool last_alarm_state = 0;

uint32_t max30102_data[2];
extern uint8_t usart1_buf[256];

// ===== 新增: 官方算法用缓冲区 =====
#define BUFFER_SIZE 100
uint32_t ir_buffer[BUFFER_SIZE];      // IR采样缓冲区
uint32_t red_buffer[BUFFER_SIZE];     // Red采样缓冲区
uint8_t buffer_ptr = 0;               // 缓冲区指针

// 算法输出结果
int32_t n_heart_rate = 0;
float f_spo2 = 0;
int8_t ch_spo2_valid = 0;
int8_t ch_hr_valid = 0;

// 函数声明
void Key_function(void);
void Monitor_function(void);
void Display_function(void);
void Alarm_function(void);
void Bluetooth_Run(uint8_t cmd);
void Maxim_Algorithm_Process(void);    // 新增: 算法处理函数

/*******************************************************************
 * 主程序: 系统初始化 + 主循环
 ******************************************************************/
int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	Delay_Init();
	Gpio_Init();
	Key_Init();
	Oled_Init();
	Oled_Clear_All();
	Usart1_Init(9600);
	TIM2_Init(99,7199);  // 100Hz中断
	while(DS18B20_Init() == 1);  // 等待温度传感器初始化成功
	max30102_init();     // 初始化心率血氧传感器

	while(1)
	{
		// 采集温度 (每100ms采集一次)
		temp_value = DS18B20_Get_Temp();

		// 采集Red和IR数据 (25Hz = 40ms采集一次, 正好10个时间单位)
		if(time_num % 10 == 0)
		{
			uint32_t red, ir;
			max30102_read_red_ir(&red, &ir);

			// 填充缓冲区
			if(buffer_ptr < BUFFER_SIZE)
			{
				red_buffer[buffer_ptr] = red;
				ir_buffer[buffer_ptr] = ir;
				buffer_ptr++;
			}

			// 当缓冲区满时,调用官方算法计算心率和血氧
			if(buffer_ptr >= BUFFER_SIZE)
			{
				Maxim_Algorithm_Process();

				// 复位缓冲区指针,继续采集
				buffer_ptr = 0;
			}
		}

		// 按键处理 + 数据监控 + 报警检查 + OLED显示
		Key_function();
		Monitor_function();
		Alarm_function();
		Display_function();

		if(time_num >= 5000) time_num = 0;
		else time_num++;
	}
}

/*******************************************************************
 * Maxim官方算法处理
 * 输入: ir_buffer[100], red_buffer[100]
 * 输出: 真实的心率和血氧值
 ******************************************************************/
void Maxim_Algorithm_Process(void)
{
	// 调用Maxim官方算法
	maxim_heart_rate_and_oxygen_saturation(
		ir_buffer,              // IR采样缓冲区
		BUFFER_SIZE,            // 缓冲区长度(100)
		red_buffer,             // Red采样缓冲区
		&f_spo2,                // 输出: 血氧值(float)
		&ch_spo2_valid,         // 输出: 血氧有效标志
		&n_heart_rate,          // 输出: 心率值
		&ch_hr_valid            // 输出: 心率有效标志
	);

	// 更新全局变量 (只有有效时才更新)
	if(ch_hr_valid)
	{
		heart_rate = (uint8_t)(n_heart_rate & 0xFF);
	}
	else
	{
		heart_rate = 0;
	}

	if(ch_spo2_valid)
	{
		spo2 = (uint8_t)(f_spo2);
	}
	else
	{
		spo2 = 0;
	}
}

/*******************************************************************
 * 按键处理函数
 * 功能: 模式切换、参数加减、从睡眠唤醒
 ******************************************************************/
void Key_function(void)
{
	key_num = Chiclet_Keyboard_Scan(0);
	if(key_num == 0) return;  // 无按键直接返回

	// 睡眠状态下,只KEY3唤醒
	if(system_state == 2)
	{
		if(key_num == 3)
		{
			system_state = 0;
			Oled_Clear_All();
		}
		return;
	}

	// 正常模式下,KEY2切换工作模式(本地/遥控/自动)
	if(system_state == 0 && key_num == 2)
	{
		work_mode++;
		if(work_mode > 2) work_mode = 0;
		Oled_Clear_All();
		return;
	}

	// 遥控模式下,禁止按键操作
	if(work_mode == 1) return;

	// 设置模式下:KEY1翻页(6页参数),KEY2加,KEY3减
	if(system_state == 1)
	{
		switch(key_num)
		{
			case 1:  // 切换参数页面,6页参数+1页退出菜单
				flag_display++;
				if(flag_display > 6)
				{
					system_state = 0;
					flag_display = 0;
				}
				Oled_Clear_All();
				break;

			case 2:  // 参数 +
				switch(flag_display)
				{
					case 1: temp_max  += 1.0f; break;
					case 2: temp_min  += 1.0f; break;
					case 3: heart_max += 1;    break;
					case 4: heart_min += 1;    break;
					case 5: spo2_max  += 1;    break;
					case 6: spo2_min  += 1;    break;
				}
				break;

			case 3:  // 参数 -
				switch(flag_display)
				{
					case 1: temp_max  -= 1.0f; break;
					case 2: temp_min  -= 1.0f; break;
					case 3: heart_max -= 1;    break;
					case 4: heart_min -= 1;    break;
					case 5: spo2_max  -= 1;    break;
					case 6: spo2_min  -= 1;    break;
				}
				break;
		}
		return;
	}

	// 正常模式下按键操作
	switch(key_num)
	{
		case 1:  // 进入设置模式
			system_state = 1;
			flag_display = 1;
			Oled_Clear_All();
			break;

		case 3:  // 进入睡眠模式
			system_state = 2;
			Oled_Clear_All();
			break;
	}
}

/*******************************************************************
 * 蓝牙遥控指令执行
 * 仅在遥控模式(work_mode==1)时有效
 ******************************************************************/
void Bluetooth_Run(uint8_t cmd)
{
	if(work_mode != 1) return;

	switch(cmd)
	{
		case 'A':  // 模式切换
			work_mode = !work_mode;
			system_state = 0;
			flag_display = 0;
			break;

		case 'B':  // 进入/切换参数设置
			if(system_state == 0)
			{
				system_state = 1;
				flag_display = 1;
				Oled_Clear_All();
			}
			else
			{
				flag_display++;
				if(flag_display > 6)
				{
					system_state = 0;
					flag_display = 0;
				}
				Oled_Clear_All();
			}
			break;

		case 'C':  // 参数 +
			if(system_state == 1)
			{
				switch(flag_display)
				{
					case 1: temp_max  += 1.0f; break;
					case 2: temp_min  += 1.0f; break;
					case 3: heart_max += 1;    break;
					case 4: heart_min += 1;    break;
					case 5: spo2_max  += 1;    break;
					case 6: spo2_min  += 1;    break;
				}
			}
			break;

		case 'D':  // 参数 -
			if(system_state == 1)
			{
				switch(flag_display)
				{
					case 1: temp_max  -= 1.0f; break;
					case 2: temp_min  -= 1.0f; break;
					case 3: heart_max -= 1;    break;
					case 4: heart_min -= 1;    break;
					case 5: spo2_max  -= 1;    break;
					case 6: spo2_min  -= 1;    break;
				}
			}
			break;
	}
}

/*******************************************************************
 * 数据监测函数
 * 功能: 定时采集、数据判断、报警中断、数据上传
 ******************************************************************/
void Monitor_function(void)
{
	// 只在正常模式和遥控模式进行报警判断
	if(system_state != 2 && system_state != 1)
	{
		float temp_cur = temp_value / 10.0f;
		_Bool new_alarm_temp  = (temp_cur > temp_max || temp_cur < temp_min);
		_Bool new_alarm_heart = (heart_rate > heart_max || heart_rate < heart_min);
		_Bool new_alarm_spo2  = (spo2 > spo2_max || spo2 < spo2_min);

		// 本地/遥控模式下异常才上报(自动模式不上报)
		if(work_mode != 2)
		{
			if(new_alarm_temp)
				UsartPrintf(USART1,"温度报警: %s!\r\n", temp_cur>temp_max ? "温度过高" : "温度过低");
			if(new_alarm_heart)
				UsartPrintf(USART1,"心率报警: %s!\r\n", heart_rate>heart_max ? "心率过快" : "心率过慢");
			if(new_alarm_spo2)
				UsartPrintf(USART1,"血氧报警: %s!\r\n", spo2>spo2_max ? "血氧过高" : "血氧过低");
		}

		// 更新报警标志
		alarm_temp  = new_alarm_temp;
		alarm_heart = new_alarm_heart;
		alarm_spo2  = new_alarm_spo2;
	}

	// 定时数据上传(周期300ms)
	if(time_num % 30 == 0 && system_state != 2)
	{
		if(system_state == 1)  // 设置模式上报当前设置值
		{
			switch(flag_display)
			{
				case 1: UsartPrintf(USART1,"[设置] 温度上限: %.1f\r\n", temp_max);  break;
				case 2: UsartPrintf(USART1,"[设置] 温度下限: %.1f\r\n", temp_min);  break;
				case 3: UsartPrintf(USART1,"[设置] 心率上限: %d\r\n", heart_max); break;
				case 4: UsartPrintf(USART1,"[设置] 心率下限: %d\r\n", heart_min); break;
				case 5: UsartPrintf(USART1,"[设置] 血氧上限: %d\r\n", spo2_max);  break;
				case 6: UsartPrintf(USART1,"[设置] 血氧下限: %d\r\n", spo2_min);  break;
				default:UsartPrintf(USART1,"[设置] 请选择参数...\r\n");               break;
			}
		}
		else  // 正常模式上报实时数据
		{
			UsartPrintf(USART1,"温度:%d.%d\r\n", temp_value/10, temp_value%10);
			UsartPrintf(USART1,"心率:%d\r\n", heart_rate);
			UsartPrintf(USART1,"血氧:%d\r\n", spo2);
			if(work_mode == 0)      UsartPrintf(USART1,"模式:本地\r\n");
			else if(work_mode == 1) UsartPrintf(USART1,"模式:遥控\r\n");
			else                    UsartPrintf(USART1,"模式:自动\r\n");
		}
	}

	// 处理来自串口的指令
	if(USART1_WaitRecive() == 0)
	{
		// 指令A: 切换工作模式
		if(strstr((char*)usart1_buf,"A"))
		{
			work_mode++;
			if(work_mode > 2) work_mode = 0;
			system_state = 0;
			flag_display = 0;
			Oled_Clear_All();
		}
		// 指令E: 切换睡眠状态
		if(strstr((char*)usart1_buf,"E"))
		{
			if(system_state == 2)
			{
				system_state = 0;
				Oled_Clear_All();
				BEEP = 0;
				LED = 0;
				alarm_temp=alarm_heart=alarm_spo2=0;
			}
			else
			{
				system_state = 2;
				Oled_Clear_All();
				BEEP = 0;
				LED = 0;
				alarm_temp=alarm_heart=alarm_spo2=0;
			}
		}
		// 遥控模式下执行B/C/D指令
		if(work_mode == 1)
		{
			if(strstr((char*)usart1_buf,"B")) Bluetooth_Run('B');
			if(strstr((char*)usart1_buf,"C")) Bluetooth_Run('C');
			if(strstr((char*)usart1_buf,"D")) Bluetooth_Run('D');
		}
		USART1_Clear();  // 清空串口缓冲区
	}
}

/*******************************************************************
 * 报警控制函数
 * 功能: 控制蜂鸣器、指示灯LED
 * 本地/遥控/睡眠模式逻辑控制
 ******************************************************************/
void Alarm_function(void)
{
	if(system_state == 2)  // 睡眠模式全部静音
	{
		BEEP = 0;
		LED  = 0;
		return;
	}

	if(system_state != 0)  // 设置模式不报警
	{
		BEEP = 0;
		LED  = 0;
		return;
	}

	if(work_mode == 2)     // 自动模式只点亮LED
	{
		BEEP = 0;
		LED  = 1;
		return;
	}

	// 本地/遥控模式: 异常则报警
	if(alarm_temp || alarm_heart || alarm_spo2)
	{
		BEEP = 1;
		LED  = 1;
	}
	else
	{
		BEEP = 0;
		LED  = 0;
	}
}

/*******************************************************************
 * OLED显示函数
 * 根据不同模式、状态自动切换显示页面
 ******************************************************************/
void Display_function(void)
{
	uint8_t current_alarm = (alarm_temp || alarm_heart || alarm_spo2);
	static uint8_t normal_first = 1;
	static uint8_t last_set_page = 0xFF;
	static uint8_t last_sys_state= 0xFF;

	if(system_state == 2)  // 睡眠模式显示
	{
		if(last_sys_state != 2)
		{
			Oled_Clear_All();
			last_sys_state = 2;
		}
		Oled_ShowCHinese(2,3,"睡眠");
		last_alarm_state = 0;
		last_set_page = 0xFF;
		return;
	}
	last_sys_state = system_state;

	if(system_state == 1)  // 设置页面显示
	{
		if(flag_display != last_set_page)
		{
			Oled_Clear_All();
			last_set_page = flag_display;
		}

		switch(flag_display)
		{
			case 1:
				Oled_ShowCHinese(1,0,"温度上限");
				sprintf(display_buf,"%.1f",temp_max);
				Oled_ShowString(2,8,display_buf);
				break;
			case 2:
				Oled_ShowCHinese(1,0,"温度下限");
				sprintf(display_buf,"%.1f",temp_min);
				Oled_ShowString(2,8,display_buf);
				break;
			case 3:
				Oled_ShowCHinese(1,0,"心率上限");
				sprintf(display_buf,"%03d",heart_max);
				Oled_ShowString(2,8,display_buf);
				break;
			case 4:
				Oled_ShowCHinese(1,0,"心率下限");
				sprintf(display_buf,"%03d",heart_min);
				Oled_ShowString(2,8,display_buf);
				break;
			case 5:
				Oled_ShowCHinese(1,0,"血氧上限");
				sprintf(display_buf,"%03d",spo2_max);
				Oled_ShowString(2,8,display_buf);
				break;
			case 6:
				Oled_ShowCHinese(1,0,"血氧下限");
				sprintf(display_buf,"%03d",spo2_min);
				Oled_ShowString(2,8,display_buf);
				break;
		}
		last_alarm_state = 0;
		return;
	}
	last_set_page = 0xFF;

	// 自动模式: 仅显示数据,不显示报警
	if(work_mode == 2)
	{
		if(normal_first)
		{
			Oled_Clear_All();
			normal_first = 0;
		}
		sprintf(display_buf,"%d.%dC   ",temp_value/10,temp_value%10);
		Oled_ShowString(1,6,display_buf);
		Oled_ShowCHinese(1,0,"温度: ");

		sprintf(display_buf,"%d    ",heart_rate);
		Oled_ShowString(2,6,display_buf);
		Oled_ShowCHinese(2,0,"心率: ");

		sprintf(display_buf,"%d    ",spo2);
		Oled_ShowString(3,6,display_buf);
		Oled_ShowCHinese(3,0,"血氧: ");

		Oled_ShowCHinese(4,0,"模式:自动");
		return;
	}

	// 本地/遥控: 异常时显示异常信息
	if(current_alarm != last_alarm_state)
	{
		Oled_Clear_All();
		last_alarm_state = current_alarm;
	}

	if(current_alarm)
	{
		if(temp_value/10.0f > temp_max)      Oled_ShowCHinese(1,0,"温度过高");
		else if(temp_value/10.0f < temp_min) Oled_ShowCHinese(1,0,"温度过低");
		else Oled_ShowCHinese(1,0,"        ");

		if(heart_rate > heart_max)           Oled_ShowCHinese(2,0,"心率过快");
		else if(heart_rate < heart_min)      Oled_ShowCHinese(2,0,"心率过慢");
		else Oled_ShowCHinese(2,0,"        ");

		if(spo2 > spo2_max)                 Oled_ShowCHinese(3,0,"血氧过高");
		else if(spo2 < spo2_min)             Oled_ShowCHinese(3,0,"血氧过低");
		else Oled_ShowCHinese(3,0,"        ");
		return;
	}

	// 正常数据显示页面
	if(normal_first)
	{
		Oled_Clear_All();
		normal_first = 0;
	}

	sprintf(display_buf,"%d.%dC   ",temp_value/10,temp_value%10);
	Oled_ShowString(1,6,display_buf);
	Oled_ShowCHinese(1,0,"温度: ");

	sprintf(display_buf,"%d    ",heart_rate);
	Oled_ShowString(2,6,display_buf);
	Oled_ShowCHinese(2,0,"心率: ");

	sprintf(display_buf,"%d    ",spo2);
	Oled_ShowString(3,6,display_buf);
	Oled_ShowCHinese(3,0,"血氧: ");

	if(work_mode == 0)
		Oled_ShowCHinese(4,0,"模式:本地");
	else if(work_mode == 1)
		Oled_ShowCHinese(4,0,"模式:遥控");
}
