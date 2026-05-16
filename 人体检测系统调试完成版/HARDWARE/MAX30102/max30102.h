/**********************************
���ߣ���Ƭ�����ֲ�
��վ��https://www.mcuclub.cn/
**********************************/


#ifndef __MAX30102_H
#define __MAX30102_H


/**********************************
����ͷ�ļ�
**********************************/
#include "sys.h"


#define MAX30102_SCL_GPIO_CLK_ENABLE      RCC_APB2Periph_GPIOA				//MAX30102_SCL����
#define MAX30102_SCL_PORT                 GPIOA
#define MAX30102_SCL_PIN                  GPIO_Pin_11
#define MAX30102_SCL  										PAout(11)

#define MAX30102_SDA_GPIO_CLK_ENABLE      RCC_APB2Periph_GPIOA				//MAX30102_SDA����
#define MAX30102_SDA_PORT                 GPIOA
#define MAX30102_SDA_PIN                  GPIO_Pin_8

#define MAX30102_SDA_OUT  								PAout(8)
#define MAX30102_SDA_IN  								  PAin(8)

#define MAX30102_IIC_Address 0xAE

#define I2C_WRITE_ADDR 0xAE
#define I2C_READ_ADDR 0xAF

#define INTERRUPT_STATUS1 0X00
#define INTERRUPT_STATUS2 0X01
#define INTERRUPT_ENABLE1 0X02
#define INTERRUPT_ENABLE2 0X03

#define FIFO_WR_POINTER 0X04
#define FIFO_OV_COUNTER 0X05
#define FIFO_RD_POINTER 0X06
#define FIFO_DATA 0X07

#define FIFO_CONFIGURATION 0X08
#define MODE_CONFIGURATION 0X09
#define SPO2_CONFIGURATION 0X0A
#define LED1_PULSE_AMPLITUDE 0X0C
#define LED2_PULSE_AMPLITUDE 0X0D

#define MULTILED1_MODE 0X11
#define MULTILED2_MODE 0X12

#define TEMPERATURE_INTEGER 0X1F
#define TEMPERATURE_FRACTION 0X20
#define TEMPERATURE_CONFIG 0X21

#define VERSION_ID 0XFE
#define PART_ID 0XFF

void max30102_init(void);
void max30102_fifo_read(uint32_t *data);
void max30102_read_red_ir(uint32_t *red, uint32_t *ir);  // 同时读取Red和IR数据
uint8_t MAX30102_ReadOneByte(uint8_t ReadAddr);
uint8_t get_real_heart_rate(void);  // 获取心率
uint8_t get_real_spo2(void);        // 获取血氧

//IIC���в�������
//void IIC_Init(void);                //��ʼ��IIC��IO��				 
//void IIC_Start(void);				//����IIC��ʼ�ź�
//void IIC_Stop(void);	  			//����IICֹͣ�ź�
//void IIC_Send_Byte(u8 txd);			//IIC����һ���ֽ�
//u8 IIC_Read_Byte(unsigned char ack);//IIC��ȡһ���ֽ�
//u8 IIC_Wait_Ack(void); 				//IIC�ȴ�ACK�ź�
//void IIC_Ack(void);					//IIC����ACK�ź�
//void IIC_NAck(void);				//IIC������ACK�ź�

//void IIC_Write_One_Byte(u8 daddr,u8 addr,u8 data);
//void IIC_Read_One_Byte(u8 daddr,u8 addr,u8* data);

//void IIC_WriteBytes(u8 WriteAddr,u8* data,u8 dataLength);
//void IIC_ReadBytes(u8 deviceAddr, u8 writeAddr,u8* data,u8 dataLength);


//#define MAX30102_INT PBin(10)

//#define I2C_WR	0		/* д����bit */
//#define I2C_RD	1		/* ������bit */



//#define I2C_WRITE_ADDR 0xAE
//#define I2C_READ_ADDR 0xAF

////register addresses
//#define REG_INTR_STATUS_1 0x00
//#define REG_INTR_STATUS_2 0x01
//#define REG_INTR_ENABLE_1 0x02
//#define REG_INTR_ENABLE_2 0x03
//#define REG_FIFO_WR_PTR 0x04
//#define REG_OVF_COUNTER 0x05
//#define REG_FIFO_RD_PTR 0x06
//#define REG_FIFO_DATA 0x07
//#define REG_FIFO_CONFIG 0x08
//#define REG_MODE_CONFIG 0x09
//#define REG_SPO2_CONFIG 0x0A
//#define REG_LED1_PA 0x0C
//#define REG_LED2_PA 0x0D
//#define REG_PILOT_PA 0x10
//#define REG_MULTI_LED_CTRL1 0x11
//#define REG_MULTI_LED_CTRL2 0x12
//#define REG_TEMP_INTR 0x1F
//#define REG_TEMP_FRAC 0x20
//#define REG_TEMP_CONFIG 0x21
//#define REG_PROX_INT_THRESH 0x30
//#define REG_REV_ID 0xFE
//#define REG_PART_ID 0xFF

//void max30102_init(void);  
//void max30102_reset(void);
//u8 max30102_Bus_Write(u8 Register_Address, u8 Word_Data);
//u8 max30102_Bus_Read(u8 Register_Address);
//void max30102_FIFO_ReadWords(u8 Register_Address,u16  Word_Data[][2],u8 count);
//void max30102_FIFO_ReadBytes(u8 Register_Address,u8* Data);

//void maxim_max30102_write_reg(uint8_t uch_addr, uint8_t uch_data);
//void maxim_max30102_read_reg(uint8_t uch_addr, uint8_t *puch_data);
//void maxim_max30102_read_fifo(uint32_t *pun_red_led, uint32_t *pun_ir_led);


//#define true 1
//#define false 0
//#define FS 100
//#define BUFFER_SIZE  (FS* 5) 
//#define HR_FIFO_SIZE 7
//#define MA4_SIZE  4 // DO NOT CHANGE
//#define HAMMING_SIZE  5// DO NOT CHANGE
//#define min(x,y) ((x) < (y) ? (x) : (y))

////const uint16_t auw_hamm[31]={ 41,    276,    512,    276,     41 }; //Hamm=  long16(512* hamming(5)');
//////uch_spo2_table is computed as  -45.060*ratioAverage* ratioAverage + 30.354 *ratioAverage + 94.845 ;
////const uint8_t uch_spo2_table[184]={ 95, 95, 95, 96, 96, 96, 97, 97, 97, 97, 97, 98, 98, 98, 98, 98, 99, 99, 99, 99, 
////                            99, 99, 99, 99, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 
////                            100, 100, 100, 100, 99, 99, 99, 99, 99, 99, 99, 99, 98, 98, 98, 98, 98, 98, 97, 97, 
////                            97, 97, 96, 96, 96, 96, 95, 95, 95, 94, 94, 94, 93, 93, 93, 92, 92, 92, 91, 91, 
////                            90, 90, 89, 89, 89, 88, 88, 87, 87, 86, 86, 85, 85, 84, 84, 83, 82, 82, 81, 81, 
////                            80, 80, 79, 78, 78, 77, 76, 76, 75, 74, 74, 73, 72, 72, 71, 70, 69, 69, 68, 67, 
////                            66, 66, 65, 64, 63, 62, 62, 61, 60, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50, 
////                            49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 31, 30, 29, 
////                            28, 27, 26, 25, 23, 22, 21, 20, 19, 17, 16, 15, 14, 12, 11, 10, 9, 7, 6, 5, 
////                            3, 2, 1 } ;
////static  int32_t an_dx[ BUFFER_SIZE-MA4_SIZE]; // delta
////static  int32_t an_x[ BUFFER_SIZE]; //ir
////static  int32_t an_y[ BUFFER_SIZE]; //red


//void maxim_heart_rate_and_oxygen_saturation(uint32_t *pun_ir_buffer ,  int32_t n_ir_buffer_length, uint32_t *pun_red_buffer ,   int32_t *pn_spo2, int8_t *pch_spo2_valid ,  int32_t *pn_heart_rate , int8_t  *pch_hr_valid);
//void maxim_find_peaks( int32_t *pn_locs, int32_t *pn_npks,  int32_t *pn_x, int32_t n_size, int32_t n_min_height, int32_t n_min_distance, int32_t n_max_num );
//void maxim_peaks_above_min_height( int32_t *pn_locs, int32_t *pn_npks,  int32_t *pn_x, int32_t n_size, int32_t n_min_height );
//void maxim_remove_close_peaks( int32_t *pn_locs, int32_t *pn_npks,   int32_t  *pn_x, int32_t n_min_distance );
//void maxim_sort_ascend( int32_t *pn_x, int32_t n_size );
//void maxim_sort_indices_descend(  int32_t  *pn_x, int32_t *pn_indx, int32_t n_size);

#endif
















