#ifndef __MAX30102_H
#define __MAX30102_H
#include "stdbool.h"
#include "sys.h"
#define MAX30102_START_STATE    1 /* 0：禁用文件 1：启用文件 */
#if MAX30102_START_STATE
//==============================================MAX30102硬件接口==================================================
#define         MAX30102_IIC_CLK                RCC_APB2Periph_GPIOB
#define         MAX30102_IIC_PORT               GPIOB
#define         MAX30102_IIC_SCL_PIN            GPIO_Pin_9
#define         MAX30102_IIC_SDA_PIN            GPIO_Pin_8

#define         MAX30102_IIC_SCL                PBout(9)
#define         MAX30102_IIC_SDA                PBout(8)
#define         MAX30102_READ_SDA               PBin(8)  //输入SDA 

#define         MAX30102_INT_PIN               GPIO_Pin_12
#define         MAX30102_INT_GPIO_PORT         GPIOA
#define         MAX30102_INT_EXTI_IRQn         EXTI15_10_IRQn
#define         MAX30102_INT_GPIO_CLK          (RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO)
        
#define         MAX30102_INT_EXTI_PORTSOURCE   GPIO_PortSourceGPIOA
#define         MAX30102_INT_EXTI_PINSOURCE    GPIO_PinSource12
#define         MAX30102_INT_EXTI_LINE         EXTI_Line12
#define         MAX30102_INT_IRQHandler        EXTI15_10_IRQHandler
//=============================================================================================================

////////////////////////////////////////////////////////////////////////////////       

#define I2C_WR    0        /* 写控制bit */
#define I2C_RD    1        /* 读控制bit */

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

#define     MAX30102_RUN_SPACE      100     /* 心率血氧传感器数据采集间隔 时间单位为Sensor_Task执行间隔 */
//extern MaxXlXy maxXlXy;                     /* 心率血氧采集值，供外界使用 */

typedef struct{
    uint8_t xl;         /* 心率值 */
    uint8_t xy;         /* 血氧值 */
    uint8_t int_flag;   /* 中断标志 使用后需清除 */
    uint8_t pre_time;   /* 上次数据更新距离当前时间 */
}MaxXlXy;  

extern MaxXlXy maxXlXy;                     /* 心率血氧采集值，供外界使用 */

void MAX30102_Init(void);
void MAX30102_FIFO_Read(float *data);
void MAX30102_GetXlXy(uint8_t* xl, uint8_t* xy);
uint16_t MAX30102_getHeartRate(float *input_data,uint16_t cache_nums);
float MAX30102_getSpO2(float *ir_input_data,float *red_input_data,uint16_t cache_nums);
#endif

#endif
