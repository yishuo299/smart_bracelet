#ifndef _DS1307_H
#define _DS1307_H
#include "sys.h"


// 修正引脚定义 - 使用PB6/PB7，因为这是标准的I2C引脚
#define DS1307_IIC_SCL_GPIO_PORT               GPIOB
#define DS1307_IIC_SCL_GPIO_PIN                GPIO_Pin_3  // 改为PB6
#define DS1307_IIC_SCL_GPIO_CLK_ENABLE()       do{RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); }while(0)

#define DS1307_IIC_SDA_GPIO_PORT               GPIOB
#define DS1307_IIC_SDA_GPIO_PIN                 GPIO_Pin_4  // 改为PB7
#define DS1307_IIC_SDA_GPIO_CLK_ENABLE()       do{RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); }while(0)

// DS1307寄存器地址
#define DS1307_REG_SECOND   0x00
#define DS1307_REG_MINUTE   0x01
#define DS1307_REG_HOUR     0x02
#define DS1307_REG_WEEK     0x03
#define DS1307_REG_DATE     0x04
#define DS1307_REG_MONTH    0x05
#define DS1307_REG_YEAR     0x06
#define DS1307_REG_CONTROL  0x07

// IO操作函数
#define DS1307_IIC_SCL(X)   GPIO_WriteBit(DS1307_IIC_SCL_GPIO_PORT, DS1307_IIC_SCL_GPIO_PIN, (BitAction)X)
#define DS1307_IIC_SDA(X)   GPIO_WriteBit(DS1307_IIC_SDA_GPIO_PORT, DS1307_IIC_SDA_GPIO_PIN, (BitAction)X)
#define DS1307_IIC_READ_SDA GPIO_ReadInputDataBit(DS1307_IIC_SDA_GPIO_PORT, DS1307_IIC_SDA_GPIO_PIN)



void DS1307_IIC_Init(void);
void DS1307_SDA_IN(void);
void DS1307_SDA_OUT(void);
void DS1307_IIC_Start(void);
void DS1307_IIC_Stop(void);
u8 DS1307_IIC_Wait_Ack(void);
void DS1307_IIC_Ack(void);
void DS1307_IIC_NAck(void);
void DS1307_IIC_Send_Byte(u8 txd);
u8 DS1307_IIC_Read_Byte(unsigned char ack);



void DS1307_WriteReg(uint8_t RegAddress, uint8_t Data);
uint8_t DS1307_ReadReg(uint8_t RegAddress);
void DS1307_Init(void);
void DS1307_SetTime(u16 year, u8 month, u8 date, u8 week, u8 hour, u8 minute, u8 second);
void DS1307_GetTime(u16 *year, u8 *month, u8 *date, u8 *week, u8 *hour, u8 *minute, u8 *second);
void DS1307_SQSET(uint8_t sqmode);
u8 DS1307_Get_Week(u16 year, u8 month, u8 day);

#endif
