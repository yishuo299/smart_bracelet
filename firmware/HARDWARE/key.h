#include "sys.h"
#include "delay.h"
#include "led.h"

#define KEY1   GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_12)//读取按键1
#define KEY2   GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13)//读取按键2
#define KEY3   GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_14)//读取按键3
#define KEY4   GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_15)//读取按键3

void KEY_Init(void);//按键初始化
u8 KEY_Scan(void);//获取按键值




