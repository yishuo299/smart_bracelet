#ifndef __AD8232_H
#define __AD8232_H

#include "sys.h"

// 引脚定义
#define AD8232_OUT_PORT      GPIOB
#define AD8232_OUT_PIN       GPIO_Pin_0

#define AD8232_SDN_PORT      GPIOB
#define AD8232_SDN_PIN       GPIO_Pin_1

// 函数声明
void AD8232_Init(void);
void AD8232_Start(void);
void AD8232_Stop(void);
uint16_t AD8232_ReadADC(void);
float AD8232_ReadVoltage(void);

#endif