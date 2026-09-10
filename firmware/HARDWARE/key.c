#include "key.h"

extern 	uint8_t oled_flag;

//按键初始化
void KEY_Init(void)
{ 
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);//使能PB端口时钟
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;//端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;		//上拉输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//IO口速度为50MHz
	
	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化PB4 PB5
}

//获取按键值
u8 KEY_Scan(void)
{
	u8 num=0;
	if(KEY1==0||KEY2==0||KEY3==0||KEY4==0)
	{
		delay_ms(10);//去抖动 
		if(KEY1==0){
			delay_ms(10);
			while(KEY1==0);
			num=1;
		}else if(KEY2==0){
			delay_ms(10);
			while(KEY2==0);
			num=2;
		}else if(KEY3==0){
			delay_ms(10);
			while(KEY3==0);
			num=3;
		}else if(KEY4==0){
			delay_ms(10);
			while(KEY4==0);
			num=4;
		}
	}
 	return num;// 无按键按下
}

