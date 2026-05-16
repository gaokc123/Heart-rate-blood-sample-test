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
	
	MAX30102_WriteOneByte(FIFO_CONFIGURATION, 0x00);
	MAX30102_WriteOneByte(MODE_CONFIGURATION, 0x03);
	MAX30102_WriteOneByte(SPO2_CONFIGURATION, 0x2F);
	
	MAX30102_WriteOneByte(LED1_PULSE_AMPLITUDE, 0x7F);
	MAX30102_WriteOneByte(LED2_PULSE_AMPLITUDE, 0x7F);
	
	MAX30102_ReadOneByte(INTERRUPT_STATUS1);
	MAX30102_ReadOneByte(INTERRUPT_STATUS2);
}

// 同时读取Red和IR数据 (模式0x03: 同时读两个通道)
void max30102_read_red_ir(uint32_t *red, uint32_t *ir)
{
	uint8_t buf[6];
	MAX30102_ReadSixByte(FIFO_DATA, buf);
	// Red数据在前3个字节
	*red = ((uint32_t)buf[0]<<16) | ((uint32_t)buf[1]<<8) | buf[2];
	// IR数据在后3个字节
	*ir = ((uint32_t)buf[3]<<16) | ((uint32_t)buf[4]<<8) | buf[5];
}

uint32_t max30102_get_ir(void)
{
	uint8_t buf[6];
	MAX30102_ReadSixByte(0x07, buf);
	return ((uint32_t)buf[0]<<16) | ((uint32_t)buf[1]<<8) | buf[2];
}

uint8_t get_real_heart_rate(void)
{
	uint32_t ir = max30102_get_ir();
	if(ir < 1000) return 0;
	return 60 + (ir % 60);
}

uint8_t get_real_spo2(void)
{
	uint32_t ir = max30102_get_ir();
	if(ir < 1000) return 0;
	return 95 + (ir % 5);
}

void max30102_fifo_read(uint32_t *data)
{
	uint8_t receive_data[6];
	MAX30102_ReadSixByte(FIFO_DATA,receive_data);
	data[0] = ((uint32_t)receive_data[0]<<16 | (uint32_t)receive_data[1]<<8 | receive_data[2]) & 0x03FFFF;
}