#include "delay.h"
#include "asrpro.h"
#include "stdio.h"	 	 
#include "string.h"	 

uint8_t ASRPRO_RX_BUF[5];
//extern u8 send_time_flag;

void UART1_Init(u32 bound)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
		
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);		//使能USART2，GPIOA时钟
	USART_DeInit(USART1);		//复位串口1
							
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;		//USART2_RX
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;		//复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure);		//初始化PA2  
							
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;		//USA``````````````````RT2_TX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;		//浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);		//初始化PA3

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority= 2;		//抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//子优先级2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;		//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);		//根据指定的参数初始化VIC寄存器						

	USART_InitStructure.USART_BaudRate = bound;		//波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;		//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;		//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;		//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;		//收发模式

	USART_Init(USART1, &USART_InitStructure);		//初始化串口
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);		//开启中断
	USART_Cmd(USART1, ENABLE);		//使能串口 
}

void ASRPRO_Init(void)
{
	UART1_Init(9600);
}


//串口2中断处理函数
uint16_t Res;
uint8_t usart_flag=0,receive_num=0;
void USART1_IRQHandler(void)                
{
	u8 i=0;
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  
	{
		Res = USART_ReceiveData(USART1);
		if(Res==0x55){usart_flag=1;}
		if(usart_flag==1)
		{
			ASRPRO_RX_BUF[receive_num]=Res ;
			receive_num++;
			if(Res==0xBB){usart_flag=2;}
		}
	}
}


void ASRPRO_Send_Data(uint8_t *p)
{
	uint8_t i=0;
	for(i=0;i<strlen((char*)p);i++)
	{
		USART_SendData(USART1,p[i]);
		delay_ms(5);
	}
}

u8 ASRPRO_RX(void)
{
	u8 clock_str[5]={0x55,0x01,0x01,0x01,0xBB};
	u8 state=0;
	
	if(usart_flag==2)
	{
		if(receive_num==5)
		{
			
			if(ASRPRO_RX_BUF[0]==0x55&&ASRPRO_RX_BUF[4]==0xBB)
			{
				if(ASRPRO_RX_BUF[1]==0x01&&ASRPRO_RX_BUF[2]==0x01&&ASRPRO_RX_BUF[3]==0x01)//
				{
					state=1;
				}
			}
			receive_num=0;
			memset(ASRPRO_RX_BUF,0,5);
		}
		usart_flag=0;
	}
	return state;
}









