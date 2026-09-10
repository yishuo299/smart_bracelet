#include "sys.h"
#include "driver.h"

int main(void)
{
	Hardware_Init();//硬件初始化
	while(1)
	{
//		Test();
		
		Get_Data();//获取传感器数据
		OLED_Show();//界面显示
		Hardware_Hander();//执行器处理
	}
}
