#include "sys.h" 


#define BLE_TX_GPIO_PORT				GPIOA
#define BLE_TX_GPIO_PIN					GPIO_Pin_2
#define BLE_TX_GPIO_CLK_ENABLE()		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

#define BLE_RX_GPIO_PORT				GPIOA
#define BLE_RX_GPIO_PIN					GPIO_Pin_3
#define BLE_RX_GPIO_CLK_ENABLE()		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

#define BLE_USART_CLK_ENABLE()			RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
#define BLE_USART_IRQn					USART2_IRQn
#define BLE_USART						USART2

void Bluetooth_Init(u32 bound);
void Bluetooth_Send_Data(char* fmt,...);


