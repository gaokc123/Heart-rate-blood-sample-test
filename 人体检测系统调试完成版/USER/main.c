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

/*******************************************************************
 * 全局变量说明
 * system_state：系统状态
 *      0 = 正常运行模式
 *      1 = 参数设置模式
 *      2 = 关机模式
 * work_mode：工作模式
 *      0 = 本地模式
 *      1 = 遥控模式
 *      2 = 待机模式（开机默认）
 ******************************************************************/
uint8_t key_num = 0;				// 按键值存储
uint8_t flag_display = 0;			// 设置菜单页码标记
uint32_t time_num = 0;				// 计时计数器，用于定时采集/上传
char display_buf[32] = {0};			// OLED 显示缓存

uint8_t system_state = 0;			// 系统状态：0运行 1设置 2关机
uint8_t work_mode = 2;				// 工作模式：0本地 1遥控 2待机（开机默认）

short temp_value = 0;				// 体温原始数据
uint8_t heart_rate = 75;			// 心率值
uint8_t spo2 = 98;					// 血氧值

// 报警阈值参数（可通过按键/蓝牙设置）
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

// 函数声明
void Key_function(void);
void Monitor_function(void);
void Display_function(void);
void Alarm_function(void);
void Bluetooth_Run(uint8_t cmd);

/*******************************************************************
 * 主函数：系统初始化 + 主循环
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
	TIM2_Init(99,7199);
	while(DS18B20_Init() == 1);	// 等待体温传感器初始化成功
	max30102_init();				// 心率血氧传感器初始化

	while(1)
	{
		// 读取传感器数据
		temp_value = DS18B20_Get_Temp();
		heart_rate = get_real_heart_rate();
		spo2       = get_real_spo2();

		// 按流程执行：按键 → 监测 → 报警 → 显示
		Key_function();
		Monitor_function();
		Alarm_function();
		Display_function();

		if(time_num >= 5000) time_num = 0;
		else time_num++;
	}
}

/*******************************************************************
 * 按键处理函数
 * 功能：模式切换、参数加减、进入/退出设置
 ******************************************************************/
void Key_function(void)
{
	key_num = Chiclet_Keyboard_Scan(0);
	if(key_num == 0) return;	// 无按键直接返回

	// 关机状态下，按KEY3开机
	if(system_state == 2)
	{
		if(key_num == 3)
		{
			system_state = 0;
			Oled_Clear_All();
		}
		return;
	}

	// 运行模式下，按KEY2切换工作模式：待机→本地→遥控→待机
	if(system_state == 0 && key_num == 2)
	{
		work_mode++;
		if(work_mode > 2) work_mode = 0;
		Oled_Clear_All();
		return;
	}

	// 遥控模式下，禁止本地按键操作
	if(work_mode == 1) return;

	// 设置模式下：按键1切页，按键2加，按键3减
	if(system_state == 1)
	{
		switch(key_num)
		{
			case 1:	// 切换设置项，超过6项退出设置
				flag_display++;
				if(flag_display > 6)
				{
					system_state = 0;
					flag_display = 0;
				}
				Oled_Clear_All();
				break;

			case 2:	// 参数 +
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

			case 3:	// 参数 -
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

	// 运行模式下按键功能
	switch(key_num)
	{
		case 1:	// 进入设置模式
			system_state = 1;
			flag_display = 1;
			Oled_Clear_All();
			break;

		case 3:	// 进入关机模式
			system_state = 2;
			Oled_Clear_All();
			break;
	}
}

/*******************************************************************
 * 蓝牙遥控指令执行函数
 * 仅在遥控模式(work_mode==1)下生效
 ******************************************************************/
void Bluetooth_Run(uint8_t cmd)
{
	if(work_mode != 1) return;

	switch(cmd)
	{
		case 'A':	// 模式切换
			work_mode = !work_mode;
			system_state = 0;
			flag_display = 0;
			break;

		case 'B':	// 进入/切换设置项
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

		case 'C':	// 参数 +
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

		case 'D':	// 参数 -
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
 * 监测核心函数
 * 功能：定时采集、报警判断、蓝牙上传、蓝牙指令解析
 ******************************************************************/
void Monitor_function(void)
{
	// 定时采集传感器数据（非关机模式）
	if(time_num % 5 == 0 && system_state != 2)
	{
		temp_value = DS18B20_Get_Temp();
		heart_rate = get_real_heart_rate();
		spo2       = get_real_spo2();
	}

	// 只有运行模式才判断报警，设置模式不判断、不推送
	if(system_state != 2 && system_state != 1)
	{
		float temp_cur = temp_value / 10.0f;
		_Bool new_alarm_temp  = (temp_cur > temp_max || temp_cur < temp_min);
		_Bool new_alarm_heart = (heart_rate > heart_max || heart_rate < heart_min);
		_Bool new_alarm_spo2  = (spo2 > spo2_max || spo2 < spo2_min);

		// 本地/遥控模式：报警推蓝牙；待机模式不推送
		if(work_mode != 2)
		{
			if(new_alarm_temp)
				UsartPrintf(USART1,"【报警】%s！\r\n", temp_cur>temp_max ? "体温过高" : "体温过低");
			if(new_alarm_heart)
				UsartPrintf(USART1,"【报警】%s！\r\n", heart_rate>heart_max ? "心率过快" : "心率过慢");
			if(new_alarm_spo2)
				UsartPrintf(USART1,"【报警】%s！\r\n", spo2>spo2_max ? "血氧过高" : "血氧过低");
		}

		// 更新报警标志
		alarm_temp  = new_alarm_temp;
		alarm_heart = new_alarm_heart;
		alarm_spo2  = new_alarm_spo2;
	}

	// 定时蓝牙上传数据
	if(time_num % 2 == 0 && system_state != 2)
	{
		if(system_state == 1)	// 设置界面上传阈值
		{
			switch(flag_display)
			{
				case 1: UsartPrintf(USART1,"[设置] 体温上限: %.1f\r\n", temp_max);  break;
				case 2: UsartPrintf(USART1,"[设置] 体温下限: %.1f\r\n", temp_min);  break;
				case 3: UsartPrintf(USART1,"[设置] 心率上限: %d\r\n", heart_max); break;
				case 4: UsartPrintf(USART1,"[设置] 心率下限: %d\r\n", heart_min); break;
				case 5: UsartPrintf(USART1,"[设置] 血氧上限: %d\r\n", spo2_max);  break;
				case 6: UsartPrintf(USART1,"[设置] 血氧下限: %d\r\n", spo2_min);  break;
				default:UsartPrintf(USART1,"[设置] 等待选择...\r\n");               break;
			}
		}
		else	// 运行界面上传实时数据
		{
			UsartPrintf(USART1,"体温:%d.%d\r\n", temp_value/10, temp_value%10);
			UsartPrintf(USART1,"血氧:%d\r\n", heart_rate);
			UsartPrintf(USART1,"心率:%d\r\n", spo2);
			if(work_mode == 0)      UsartPrintf(USART1,"模式:本地\r\n");
			else if(work_mode == 1) UsartPrintf(USART1,"模式:遥控\r\n");
			else                    UsartPrintf(USART1,"模式:待机\r\n");
		}
	}

	// 蓝牙指令接收处理
	if(USART1_WaitRecive() == 0)
	{
		// 指令A：切换模式
		if(strstr((char*)usart1_buf,"A"))
		{
			work_mode++;
			if(work_mode > 2) work_mode = 0;
			system_state = 0;
			flag_display = 0;
			Oled_Clear_All();
		}
		// 指令E：开关机
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
		// 遥控模式下解析B/C/D指令
		if(work_mode == 1)
		{
			if(strstr((char*)usart1_buf,"B")) Bluetooth_Run('B');
			if(strstr((char*)usart1_buf,"C")) Bluetooth_Run('C');
			if(strstr((char*)usart1_buf,"D")) Bluetooth_Run('D');
		}
		USART1_Clear();	// 清空接收缓存
	}
}

/*******************************************************************
 * 报警控制函数
 * 功能：控制蜂鸣器BEEP、指示灯LED
 * 待机/设置/关机：不报警
 ******************************************************************/
void Alarm_function(void)
{
	if(system_state == 2)	// 关机：全关
	{
		BEEP = 0;
		LED  = 0;
		return;
	}

	if(system_state != 0)	// 设置模式：不报警
	{
		BEEP = 0;
		LED  = 0;
		return;
	}

	if(work_mode == 2)		// 待机模式：不报警
	{
		BEEP = 0;
		LED  = 1;
		return;
	}

	// 运行模式：异常报警
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
 * 根据不同模式、状态自动切换界面
 ******************************************************************/
void Display_function(void)
{
	uint8_t current_alarm = (alarm_temp || alarm_heart || alarm_spo2);
	static uint8_t normal_first = 1;
	static uint8_t last_set_page = 0xFF;
	static uint8_t last_sys_state= 0xFF;
	static uint8_t last_work_mode = 0xFF;
	uint8_t sys_changed;
	uint8_t alarm_changed;
	uint8_t page_changed;
	uint8_t draw_lbl;

	if(system_state == 2)	// 关机界面
	{
		if(last_sys_state != 2)
		{
			Oled_Clear_All();
			last_sys_state = 2;
		}
		Oled_ShowCHinese(2,3,"关机");
		last_alarm_state = 0;
		last_set_page = 0xFF;
		return;
	}
	sys_changed   = (last_sys_state != system_state);
	alarm_changed = (current_alarm != last_alarm_state);
	last_sys_state = system_state;

	if(system_state == 1)	// 设置界面
	{
		page_changed = (flag_display != last_set_page);
		if(page_changed)
		{
			Oled_Clear_All();
			last_set_page = flag_display;
		}

		switch(flag_display)
		{
			case 1:
				if(page_changed) Oled_ShowCHinese(1,0,"体温上限");
				sprintf(display_buf,"%.1f",temp_max);
				Oled_ShowString(2,8,display_buf);
				break;
			case 2:
				if(page_changed) Oled_ShowCHinese(1,0,"体温下限");
				sprintf(display_buf,"%.1f",temp_min);
				Oled_ShowString(2,8,display_buf);
				break;
			case 3:
				if(page_changed) Oled_ShowCHinese(1,0,"心率上限");
				sprintf(display_buf,"%03d",heart_max);
				Oled_ShowString(2,8,display_buf);
				break;
			case 4:
				if(page_changed) Oled_ShowCHinese(1,0,"心率下限");
				sprintf(display_buf,"%03d",heart_min);
				Oled_ShowString(2,8,display_buf);
				break;
			case 5:
				if(page_changed) Oled_ShowCHinese(1,0,"血氧上限");
				sprintf(display_buf,"%03d",spo2_max);
				Oled_ShowString(2,8,display_buf);
				break;
			case 6:
				if(page_changed) Oled_ShowCHinese(1,0,"血氧下限");
				sprintf(display_buf,"%03d",spo2_min);
				Oled_ShowString(2,8,display_buf);
				break;
		}
		last_alarm_state = 0;
		return;
	}
	last_set_page = 0xFF;

	// 待机界面：只显示数据，不显示报警
	if(work_mode == 2)
	{
		draw_lbl = normal_first || (last_work_mode != work_mode) || sys_changed || alarm_changed;
		if(normal_first)
		{
			Oled_Clear_All();
			normal_first = 0;
		}
		last_work_mode = work_mode;
		sprintf(display_buf,"%d.%dC   ",temp_value/10,temp_value%10);
		Oled_ShowString(1,6,display_buf);
		if(draw_lbl) Oled_ShowCHinese(1,0,"体温：");

		sprintf(display_buf,"%d    ",heart_rate);
		Oled_ShowString(2,6,display_buf);
		if(draw_lbl) Oled_ShowCHinese(2,0,"心率：");

		sprintf(display_buf,"%d    ",spo2);
		Oled_ShowString(3,6,display_buf);
		if(draw_lbl) Oled_ShowCHinese(3,0,"血氧：");

		if(draw_lbl) Oled_ShowCHinese(4,0,"模式：待机");
		return;
	}

	// 本地/遥控：报警界面
	if(current_alarm != last_alarm_state)
	{
		Oled_Clear_All();
		last_alarm_state = current_alarm;
	}

	if(current_alarm)
	{
		if(temp_value/10.0f > temp_max)      Oled_ShowCHinese(1,0,"体温过高");
		else if(temp_value/10.0f < temp_min) Oled_ShowCHinese(1,0,"体温过低");
		else Oled_ShowCHinese(1,0,"        ");

		if(heart_rate > heart_max)           Oled_ShowCHinese(2,0,"心率过快");
		else if(heart_rate < heart_min)      Oled_ShowCHinese(2,0,"心率过慢");
		else Oled_ShowCHinese(2,0,"        ");

		if(spo2 > spo2_max)                 Oled_ShowCHinese(3,0,"血氧过高");
		else if(spo2 < spo2_min)             Oled_ShowCHinese(3,0,"血氧过低");
		else Oled_ShowCHinese(3,0,"        ");
		return;
	}

	// 正常数据界面
	draw_lbl = normal_first || (last_work_mode != work_mode) || sys_changed || alarm_changed;
	if(normal_first)
	{
		Oled_Clear_All();
		normal_first = 0;
	}
	last_work_mode = work_mode;

	sprintf(display_buf,"%d.%dC   ",temp_value/10,temp_value%10);
	Oled_ShowString(1,6,display_buf);
	if(draw_lbl) Oled_ShowCHinese(1,0,"体温：");

	sprintf(display_buf,"%d    ",heart_rate);
	Oled_ShowString(2,6,display_buf);
	if(draw_lbl) Oled_ShowCHinese(2,0,"心率：");

	sprintf(display_buf,"%d    ",spo2);
	Oled_ShowString(3,6,display_buf);
	if(draw_lbl) Oled_ShowCHinese(3,0,"血氧：");

	if(draw_lbl)
	{
		if(work_mode == 0)
			Oled_ShowCHinese(4,0,"模式：本地");
		else if(work_mode == 1)
			Oled_ShowCHinese(4,0,"模式：遥控");
	}
}
