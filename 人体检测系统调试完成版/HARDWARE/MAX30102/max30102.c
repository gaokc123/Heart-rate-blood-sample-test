#include "max30102.h"
#include "delay.h"

void MAX30102_GPIO_OUT(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	DBGMCU->CR  &= ~((uint32_t)1<<5);
	
	RCC_APB2PeriphClockCmd(MAX30102_SCL_GPIO_CLK_ENABLE, ENABLE);
	GPIO_InitStructure.GPIO_Pin = MAX30102_SCL_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(MAX30102_SCL_PORT, &GPIO_InitStructure);
	
	RCC_APB2PeriphClockCmd(MAX30102_SDA_GPIO_CLK_ENABLE, ENABLE);
	GPIO_InitStructure.GPIO_Pin = MAX30102_SDA_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(MAX30102_SDA_PORT, &GPIO_InitStructure);
}

void MAX30102_GPIO_IN(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	DBGMCU->CR  &= ~((uint32_t)1<<5);
	
	RCC_APB2PeriphClockCmd(MAX30102_SCL_GPIO_CLK_ENABLE, ENABLE);
	GPIO_InitStructure.GPIO_Pin = MAX30102_SCL_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(MAX30102_SCL_PORT, &GPIO_InitStructure);
	
	RCC_APB2PeriphClockCmd(MAX30102_SDA_GPIO_CLK_ENABLE, ENABLE);
	GPIO_InitStructure.GPIO_Pin = MAX30102_SDA_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(MAX30102_SDA_PORT, &GPIO_InitStructure);
}

void MAX30102_IIC_Start(void)
{
	MAX30102_GPIO_OUT();
	MAX30102_SDA_OUT=1;
	MAX30102_SCL=1;
	Delay_us(4);
 	MAX30102_SDA_OUT=0;
	Delay_us(4);
	MAX30102_SCL=0;
}

void MAX30102_IIC_Stop(void)
{
	MAX30102_GPIO_OUT();
	MAX30102_SCL=0;
	MAX30102_SDA_OUT=0;
 	Delay_us(4);
	MAX30102_SCL=1;
	MAX30102_SDA_OUT=1;
	Delay_us(4);
}

u8 MAX30102_IIC_Wait_Ack(void)
{
	u8 ucErrTime=0;
	MAX30102_GPIO_IN();
	MAX30102_SDA_OUT=1;Delay_us(1);
	MAX30102_SCL=1;Delay_us(1);
	while(MAX30102_SDA_IN)
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			MAX30102_IIC_Stop();
			return 1;
		}
	}
	MAX30102_SCL=0;
	return 0;
}

void MAX30102_IIC_Ack(void)
{
	MAX30102_SCL=0;
	MAX30102_GPIO_OUT();
	MAX30102_SDA_OUT=0;
	Delay_us(2);
	MAX30102_SCL=1;
	Delay_us(2);
	MAX30102_SCL=0;
}

void MAX30102_IIC_NAck(void)
{
	MAX30102_SCL=0;
	MAX30102_GPIO_OUT();
	MAX30102_SDA_OUT=1;
	Delay_us(2);
	MAX30102_SCL=1;
	Delay_us(2);
	MAX30102_SCL=0;
}

void MAX30102_IIC_Send_Byte(u8 txd)
{
	u8 t;
	MAX30102_GPIO_OUT();
	MAX30102_SCL=0;
	for(t=0;t<8;t++)
	{
		MAX30102_SDA_OUT=(txd&0x80)>>7;
		txd<<=1;
		Delay_us(2);
		MAX30102_SCL=1;
		Delay_us(2);
		MAX30102_SCL=0;
		Delay_us(2);
	}
}

u8 MAX30102_IIC_Read_Byte(unsigned char ack)
{
	unsigned char i,receive=0;
	MAX30102_GPIO_IN();
  for(i=0;i<8;i++ )
	{
		MAX30102_SCL=0;
		Delay_us(2);
		MAX30102_SCL=1;
		receive<<=1;
		if(MAX30102_SDA_IN)receive++;
		Delay_us(1);
  }
	if (!ack) MAX30102_IIC_NAck();
	else MAX30102_IIC_Ack();
  return receive;
}

void MAX30102_WriteOneByte(uint8_t WriteAddr,uint8_t DataToWrite)
{
	MAX30102_IIC_Start();
	MAX30102_IIC_Send_Byte(MAX30102_IIC_Address);
	MAX30102_IIC_Wait_Ack();
	MAX30102_IIC_Send_Byte(WriteAddr);
	MAX30102_IIC_Wait_Ack();
	MAX30102_IIC_Send_Byte(DataToWrite);
	MAX30102_IIC_Wait_Ack();
	MAX30102_IIC_Stop();
  Delay_ms(1);
}

uint8_t MAX30102_ReadOneByte(uint8_t ReadAddr)
{
	uint8_t data = 0;
	MAX30102_IIC_Start();
	MAX30102_IIC_Send_Byte(MAX30102_IIC_Address);
	MAX30102_IIC_Wait_Ack();
	MAX30102_IIC_Send_Byte(ReadAddr);
	MAX30102_IIC_Wait_Ack();
	MAX30102_IIC_Start();
	MAX30102_IIC_Send_Byte(MAX30102_IIC_Address+1);
	MAX30102_IIC_Wait_Ack();
	data = MAX30102_IIC_Read_Byte(0);
	MAX30102_IIC_Stop();
	return data;
}

void MAX30102_ReadSixByte(uint8_t ReadAddr, uint8_t *data)
{
	MAX30102_IIC_Start();
	MAX30102_IIC_Send_Byte(MAX30102_IIC_Address);
	MAX30102_IIC_Wait_Ack();
	MAX30102_IIC_Send_Byte(ReadAddr);
	MAX30102_IIC_Wait_Ack();
	MAX30102_IIC_Start();
	MAX30102_IIC_Send_Byte(MAX30102_IIC_Address+1);
	MAX30102_IIC_Wait_Ack();
	data[0] = MAX30102_IIC_Read_Byte(1);
	data[1] = MAX30102_IIC_Read_Byte(1);
	data[2] = MAX30102_IIC_Read_Byte(1);
	data[3] = MAX30102_IIC_Read_Byte(1);
	data[4] = MAX30102_IIC_Read_Byte(1);
	data[5] = MAX30102_IIC_Read_Byte(0);
	MAX30102_IIC_Stop();
}

void max30102_init(void)
{ 
	MAX30102_WriteOneByte(MODE_CONFIGURATION, 0x40);
	Delay_ms(10);
	
	MAX30102_WriteOneByte(INTERRUPT_ENABLE1, 0x00);
	MAX30102_WriteOneByte(INTERRUPT_ENABLE2, 0x00);
	
	MAX30102_WriteOneByte(FIFO_WR_POINTER, 0x00);
	MAX30102_WriteOneByte(FIFO_OV_COUNTER, 0x00);
	MAX30102_WriteOneByte(FIFO_RD_POINTER, 0x00);
	
	MAX30102_WriteOneByte(FIFO_CONFIGURATION, 0x5F);  /* sample avg=4 (->25Hz net), rollover ON, almost-full=17 */
	MAX30102_WriteOneByte(MODE_CONFIGURATION, 0x03);
	MAX30102_WriteOneByte(SPO2_CONFIGURATION, 0x47);  /* ADC range 8192nA, 100Hz, 411us pulse */
	
	MAX30102_WriteOneByte(LED1_PULSE_AMPLITUDE, 0x24);
	MAX30102_WriteOneByte(LED2_PULSE_AMPLITUDE, 0x24);
	
	MAX30102_ReadOneByte(INTERRUPT_STATUS1);
	MAX30102_ReadOneByte(INTERRUPT_STATUS2);
}

/* =====================================================================
 * MAX30102 心率 / 血氧 测量层（基于 Maxim 官方算法）
 * ---------------------------------------------------------------------
 *  采样率   : SPO2 模式 100Hz 物理 + FIFO 4x 平均 = 25Hz 净输出
 *  缓冲     : 125 样本（5 秒）IR + RED 滑动窗
 *  计算节奏 : 缓冲首次填满 → 立即算一次；之后每读到 25 个新样本（1秒）
 *             滑窗推进一格、保留最近 5 秒数据，
 *             调用 maxim_heart_rate_and_oxygen_saturation 重新计算。
 *  接口     : main.c 周期性调用 max30102_update()，然后通过
 *             get_real_heart_rate() / get_real_spo2() 取最新有效值。
 *             检测不到手指或信号无效时返回 0。
 * ===================================================================== */

#include "algorithm.h"

#define MAX30102_WIN_LEN     BUFFER_SIZE        /* 125 = 5秒 @ 25Hz */
#define MAX30102_REFRESH     FS                 /* 每 25 样本(=1秒) 重算 */

static uint32_t aun_ir_buf [MAX30102_WIN_LEN];
static uint32_t aun_red_buf[MAX30102_WIN_LEN];
static uint16_t buf_count            = 0;       /* 已积累样本数（0..WIN_LEN） */
static uint16_t samples_since_compute = 0;       /* 距离上次计算的新样本数 */
static uint8_t  real_hr   = 0;
static uint8_t  real_spo2 = 0;

/* 读取一组 FIFO 数据：RED 在前 3 字节，IR 在后 3 字节 */
static void max30102_read_one_sample(uint32_t *red, uint32_t *ir)
{
    uint8_t b[6];
    MAX30102_ReadSixByte(FIFO_DATA, b);
    *red = (((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | b[2]) & 0x03FFFF;
    *ir  = (((uint32_t)b[3] << 16) | ((uint32_t)b[4] << 8) | b[5]) & 0x03FFFF;
}

/* 当前 FIFO 中待读样本数（写指针 - 读指针, mod 32） */
static uint8_t max30102_fifo_pending(void)
{
    uint8_t wr = MAX30102_ReadOneByte(FIFO_WR_POINTER);
    uint8_t rd = MAX30102_ReadOneByte(FIFO_RD_POINTER);
    return (uint8_t)((wr - rd) & 0x1F);
}

/* 调用 Maxim 算法对当前窗口计算心率 / SpO2，写入 real_hr / real_spo2 */
static void compute_hr_and_spo2(void)
{
    int32_t n_hr  = 0, n_spo2 = 0;
    int8_t  ch_hr_valid = 0, ch_spo2_valid = 0;

    maxim_heart_rate_and_oxygen_saturation(
        aun_ir_buf, MAX30102_WIN_LEN, aun_red_buf,
        &n_spo2, &ch_spo2_valid,
        &n_hr , &ch_hr_valid);

    /* 心率有效范围 30..220 bpm */
    if(ch_hr_valid && n_hr >= 30 && n_hr <= 220)
        real_hr = (uint8_t)n_hr;
    else
        real_hr = 0;

    /* 血氧有效范围 70..100 % */
    if(ch_spo2_valid && n_spo2 >= 70 && n_spo2 <= 100)
        real_spo2 = (uint8_t)n_spo2;
    else
        real_spo2 = 0;
}

/* 把 FIFO 中所有 pending 样本搬入滑动窗，并按节奏触发计算。
 * 主循环只需周期性调用此函数即可。 */
void max30102_update(void)
{
    uint8_t avail;
    uint32_t red, ir;
    uint16_t i;

    avail = max30102_fifo_pending();
    while(avail-- > 0)
    {
        max30102_read_one_sample(&red, &ir);

        if(buf_count < MAX30102_WIN_LEN)
        {
            /* 窗未满，追加到末尾 */
            aun_red_buf[buf_count] = red;
            aun_ir_buf [buf_count] = ir;
            buf_count++;
        }
        else
        {
            /* 窗已满：丢弃最旧一个，整体左移一格，新样本放到末尾。
             * 与参考程序"批量丢弃前 100、保留后 400、再补 100"等价，
             * 只是每个新样本来时就移一次，逻辑更简单。
             * 移动开销在 100Hz 下完全可承受。 */
            for(i = 0; i < MAX30102_WIN_LEN - 1; i++)
            {
                aun_red_buf[i] = aun_red_buf[i + 1];
                aun_ir_buf [i] = aun_ir_buf [i + 1];
            }
            aun_red_buf[MAX30102_WIN_LEN - 1] = red;
            aun_ir_buf [MAX30102_WIN_LEN - 1] = ir;
        }

        samples_since_compute++;
    }

    /* 缓冲填满后，每 MAX30102_REFRESH (=25) 个新样本重新算一次 */
    if(buf_count >= MAX30102_WIN_LEN && samples_since_compute >= MAX30102_REFRESH)
    {
        compute_hr_and_spo2();
        samples_since_compute = 0;
    }
}

/* main.c 中的赋值已经修正为正向，此处按名称返回正确值 */
uint8_t get_real_heart_rate(void)
{
    max30102_update();
    return real_hr;
}

uint8_t get_real_spo2(void)
{
    max30102_update();
    return real_spo2;
}

/* ----- 旧接口保留，供其它模块兼容 ----- */
uint32_t max30102_get_ir(void)
{
    uint8_t buf[6];
    MAX30102_ReadSixByte(FIFO_DATA, buf);
    return (((uint32_t)buf[3] << 16) | ((uint32_t)buf[4] << 8) | buf[5]) & 0x03FFFF;
}

void max30102_fifo_read(uint32_t *data)
{
    uint8_t b[6];
    MAX30102_ReadSixByte(FIFO_DATA, b);
    data[0] = (((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | b[2]) & 0x03FFFF;
    data[1] = (((uint32_t)b[3] << 16) | ((uint32_t)b[4] << 8) | b[5]) & 0x03FFFF;
}