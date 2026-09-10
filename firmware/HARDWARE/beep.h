#include "sys.h"
#include "delay.h"
#include "led.h"

#define BEEP PAout(1)


void BEEP_Init(void);//BEEP初始化
void BEEP_Hint(uint16_t tim);//报警(时间：ms)


