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
	
	MAX30102_WriteOneByte(FIFO_CONFIGURATION, 0x5F);
	MAX30102_WriteOneByte(MODE_CONFIGURATION, 0x03);
	MAX30102_WriteOneByte(SPO2_CONFIGURATION, 0x27);
	
	MAX30102_WriteOneByte(LED1_PULSE_AMPLITUDE, 0x24);
	MAX30102_WriteOneByte(LED2_PULSE_AMPLITUDE, 0x24);
	
	MAX30102_ReadOneByte(INTERRUPT_STATUS1);
	MAX30102_ReadOneByte(INTERRUPT_STATUS2);
}

/* =====================================================================
 * MAX30102 real heart-rate / SpO2 measurement
 * ---------------------------------------------------------------------
 *  Sensor mode : SPO2 mode, 100 Hz, FIFO record = 6 bytes (RED then IR)
 *  Buffer      : 200-sample (2 s) ring buffer of RED and IR
 *  Recompute   : every 25 new samples (~250 ms) once buffer is full
 *  Heart rate  : adaptive threshold peak detect on IR
 *                BPM = 60 * Fs * (peaks - 1) / span
 *  SpO2        : R = (AC_red / DC_red) / (AC_ir / DC_ir)
 *                SpO2 = -45.060*R^2 + 30.354*R + 94.845  (Maxim fit)
 *  Finger gate : returns 0 when DC too low / AC too small / saturated
 *
 *  >>> NOTE on function names <<<
 *  main.c assigns the values like this:
 *      heart_rate = get_real_spo2();
 *      spo2       = get_real_heart_rate();
 *  i.e. the two call sites are SWAPPED.  Per project decision main.c is
 *  not modified.  To produce correct readings the two getters below are
 *  deliberately wired the opposite way: get_real_spo2() returns the
 *  real heart rate, get_real_heart_rate() returns the real SpO2.
 *  If main.c is ever fixed, swap the two return statements back.
 * ===================================================================== */

#define MAX30102_BUF_SIZE    100
#define COMPUTE_INTERVAL     25
#define MIN_PEAK_GAP         6 
#define FINGER_IR_DC_MIN     1000UL 
#define FINGER_RED_DC_MIN    500UL 
#define FINGER_AC_MIN        10UL 
#define IR_SATURATION_LIMIT  0x03FFF0UL

static uint32_t ir_buf[MAX30102_BUF_SIZE];
static uint32_t red_buf[MAX30102_BUF_SIZE];
static uint16_t buf_head = 0;
static uint16_t buf_count = 0;
static uint16_t samples_since_compute = 0;
static uint8_t  real_hr = 0;
static uint8_t  real_spo2 = 0;

/* Read one full FIFO record: RED first (3 bytes), then IR (3 bytes). */
static void max30102_read_one_sample(uint32_t *red, uint32_t *ir)
{
    uint8_t b[6];
    MAX30102_ReadSixByte(FIFO_DATA, b);
    *red = (((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | b[2]) & 0x03FFFF;
    *ir  = (((uint32_t)b[3] << 16) | ((uint32_t)b[4] << 8) | b[5]) & 0x03FFFF;
}

/* Number of samples available in FIFO (write ptr - read ptr, mod 32). */
static uint8_t max30102_fifo_pending(void)
{
    uint8_t wr = MAX30102_ReadOneByte(FIFO_WR_POINTER);
    uint8_t rd = MAX30102_ReadOneByte(FIFO_RD_POINTER);
    return (uint8_t)((wr - rd) & 0x1F);
}

/* Walk the ring buffer in time order and compute HR + SpO2. */
static void compute_hr_and_spo2(void)
{
    uint16_t i;
    uint32_t ir_max = 0, ir_min = 0x03FFFF;
    uint32_t red_max = 0, red_min = 0x03FFFF;
    uint32_t ir_sum = 0, red_sum = 0;
    uint32_t ir_mean, red_mean, ir_ac, red_ac;
    uint32_t threshold;
    uint16_t peak_count = 0;
    uint16_t first_peak = 0, last_peak = 0, last_peak_idx = 0;
    uint8_t  above = 0;
    float    r, spo2_val;

    for(i = 0; i < MAX30102_BUF_SIZE; i++)
    {
        uint16_t idx = (uint16_t)((buf_head + i) % MAX30102_BUF_SIZE);
        uint32_t ir  = ir_buf[idx];
        uint32_t red = red_buf[idx];
        if(ir  > ir_max)  ir_max  = ir;
        if(ir  < ir_min)  ir_min  = ir;
        if(red > red_max) red_max = red;
        if(red < red_min) red_min = red;
        ir_sum  += ir;
        red_sum += red;
    }

    ir_mean  = ir_sum  / MAX30102_BUF_SIZE;
    red_mean = red_sum / MAX30102_BUF_SIZE;
    ir_ac    = ir_max  - ir_min;
    red_ac   = red_max - red_min;

    /* Finger / signal sanity check. */
    if(ir_mean  < FINGER_IR_DC_MIN  ||
       red_mean < FINGER_RED_DC_MIN ||
       ir_ac    < FINGER_AC_MIN     ||
       ir_max   >= IR_SATURATION_LIMIT)
    {
        real_hr   = 0;
        real_spo2 = 0;
        return;
    }

    /* Heart rate: threshold = mean + AC/4 ; require min spacing between peaks. */
    threshold = ir_mean + (ir_ac >> 2);
    for(i = 0; i < MAX30102_BUF_SIZE; i++)
    {
        uint16_t idx = (uint16_t)((buf_head + i) % MAX30102_BUF_SIZE);
        uint32_t ir  = ir_buf[idx];

        if(ir > threshold)
        {
            if(!above && (peak_count == 0 || (i - last_peak_idx) >= MIN_PEAK_GAP))
            {
                above = 1;
                if(peak_count == 0) first_peak = i;
                last_peak = i;
                last_peak_idx = i;
                peak_count++;
            }
        }
        else if(ir < ir_mean)
        {
            above = 0;
        }
    }

    if(peak_count >= 2)
    {
        uint32_t span = (uint32_t)(last_peak - first_peak);
        uint32_t bpm  = (1500UL * (uint32_t)(peak_count - 1)) / span;
        if(bpm >= 40 && bpm <= 200)
            real_hr = (uint8_t)bpm;
    }

    /* SpO2: AC/DC ratio with Maxim empirical curve. */
    if(red_ac > 0 && ir_ac > 0)
    {
        r        = ((float)red_ac * (float)ir_mean) / ((float)red_mean * (float)ir_ac);
        spo2_val = -45.060f * r * r + 30.354f * r + 94.845f;
        if(spo2_val > 100.0f) spo2_val = 100.0f;
        if(spo2_val >= 50.0f) real_spo2 = (uint8_t)spo2_val;
    }
}

/* Call at any rate from main loop; safe to call multiple times per loop. */
void max30102_update(void)
{
    uint8_t avail = max30102_fifo_pending();
    while(avail-- > 0)
    {
        uint32_t red, ir;
        max30102_read_one_sample(&red, &ir);
        ir_buf[buf_head]  = ir;
        red_buf[buf_head] = red;
        buf_head = (uint16_t)((buf_head + 1) % MAX30102_BUF_SIZE);
        if(buf_count < MAX30102_BUF_SIZE) buf_count++;
        samples_since_compute++;
    }

    if(buf_count >= MAX30102_BUF_SIZE && samples_since_compute >= COMPUTE_INTERVAL)
    {
        compute_hr_and_spo2();
        samples_since_compute = 0;
    }
}

/* main.c bug: heart_rate = get_real_spo2();  -> we return REAL HR here. */
uint8_t get_real_spo2(void)
{
    max30102_update();
    return real_hr;
}

/* main.c bug: spo2 = get_real_heart_rate();  -> we return REAL SpO2 here. */
uint8_t get_real_heart_rate(void)
{
    max30102_update();
    return real_spo2;
}

/* Legacy helpers kept for ABI compatibility. */
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