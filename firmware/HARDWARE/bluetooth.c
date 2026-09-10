#include "bluetooth.h" 
#include "driver.h"
#include "stdio.h"	 	 
#include "string.h"	 
#include "stdarg.h"	 
#include "stdlib.h"

#define BLE_RX_MAX 64
u8 BLE_RX_BUF[100];     /* 蓝牙接收缓冲区（备用） */
u8 BLE_TX_BUF[100];     /* 蓝牙发送缓冲区 */
u16 BLE_RX_STA=0;       /* 当前行接收长度/状态 */
u8 BLE_RX_LINE[BLE_RX_MAX];  /* 一行命令缓冲 */

void Bluetooth_Init(u32 bound)
{
	/* GPIO 与串口初始化 */
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	BLE_TX_GPIO_CLK_ENABLE();
	BLE_RX_GPIO_CLK_ENABLE();
	BLE_USART_CLK_ENABLE();
	
	/* USART TX（见 bluetooth.h：如 PA2） */
	GPIO_InitStructure.GPIO_Pin = BLE_TX_GPIO_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	/* 复用推挽输出 */
	GPIO_Init(BLE_TX_GPIO_PORT, &GPIO_InitStructure);

	/* USART RX（如 PA3） */
	GPIO_InitStructure.GPIO_Pin = BLE_RX_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;/* 浮空输入 */
	GPIO_Init(BLE_RX_GPIO_PORT, &GPIO_InitStructure);

	/* USART NVIC 配置 */
	NVIC_InitStructure.NVIC_IRQChannel = BLE_USART_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;/* 抢占优先级 3 */
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		/* 子优先级 3 */
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			/* 使能 IRQ 通道 */
	NVIC_Init(&NVIC_InitStructure);	/* 根据指定参数初始化 NVIC */

	/* USART 参数配置 */

	USART_InitStructure.USART_BaudRate = bound;/* 波特率 */
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;/* 8 位数据 */
	USART_InitStructure.USART_StopBits = USART_StopBits_1;/* 1 位停止位 */
	USART_InitStructure.USART_Parity = USART_Parity_No;/* 无校验 */
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;/* 无硬件流控 */
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	/* 收发 */

	USART_Init(BLE_USART, &USART_InitStructure); /* 初始化 USART2 */
	USART_ITConfig(BLE_USART, USART_IT_RXNE, ENABLE);/* 使能接收非空中断 */
	USART_Cmd(BLE_USART, ENABLE);                    /* 使能 USART */
}

void Bluetooth_Send_Data(char* fmt,...)  
{  
	u16 i,j; 
	va_list ap; 
	va_start(ap,fmt);
	vsprintf((char*)BLE_TX_BUF,fmt,ap);
	va_end(ap);
	i=strlen((const char*)BLE_TX_BUF);		/* 待发数据长度 */
	for(j=0;j<i;j++)							/* 逐字节发送 */
	{
		while(USART_GetFlagStatus(BLE_USART,USART_FLAG_TC)==RESET); /* 等待发送完成 */
		USART_SendData(BLE_USART,BLE_TX_BUF[j]); 
	} 
}



static void BLE_Parse_Threshold_Cmd(const char* buf)
{
	const char* p = strstr(buf, "TH:");
	if (!p) return;
	p += 3;
	if (p[0] >= '0' && p[0] <= '4' && p[1] == ':') {
		int idx = p[0] - '0';
		int val = atoi(p + 2);
		Driver_Set_Threshold((int8_t)idx, (int16_t)val);
		return;
	}
	{
		int16_t t[5];
		int n = 0;
		for (n = 0; n < 5 && *p; n++) {
			t[n] = (int16_t)atoi(p);
			while (*p && *p != ',' && *p != '\r' && *p != '\n') p++;
			if (*p == ',') p++;
		}
		if (n >= 5) {
			Driver_Set_Thresholds(t[0], t[1], t[2], t[3], t[4]);
		}
	}
}

u8 usart2_rx_count=0;
void USART2_IRQHandler(void)
{
	u8 Res;
	if(USART_GetITStatus(BLE_USART, USART_IT_RXNE) != RESET)
	{
		Res = USART_ReceiveData(BLE_USART);
		USART_SendData(BLE_USART, Res);
		if (BLE_RX_STA < BLE_RX_MAX - 1) {
			BLE_RX_LINE[BLE_RX_STA++] = Res;
			BLE_RX_LINE[BLE_RX_STA] = '\0';
			if (Res == '\r' || Res == '\n') {
				BLE_Parse_Threshold_Cmd((char*)BLE_RX_LINE);
				BLE_RX_STA = 0;
			}
		} else {
			BLE_RX_STA = 0;
		}
	}
}
