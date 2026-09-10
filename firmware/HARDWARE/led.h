#ifndef __LED_H
#define __LED_H	 
#include "sys.h"

#define LED PBout(5)	

#define LED_ON 1
#define LED_OFF 0

void LED_Init(void);//LED≥ı ºªØ
void LED_Mode(uint8_t mode);



#endif
