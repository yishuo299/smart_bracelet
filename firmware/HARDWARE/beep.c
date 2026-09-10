#include "beep.h"

//蜂鸣器初始化
void BEEP_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);					 //根据设定参数初始
	
	BEEP=0;
}

//报警(时间：ms)
void BEEP_Hint(uint16_t tim)
{
	BEEP=1;
	delay_ms(tim);
	BEEP=0;
}



