#include "sys.h"   

//IO操作函数											   
#define	DS18B20_DQ_OUT PBout(7) //数据端口	PA0 
#define	DS18B20_DQ_IN  PBin(7)  //数据端口	PA0 
   	
u8 DS18B20_Init(void);//初始化DS18B20
void DS18B20_Start(void);//开始温度转换
void DS18B20_Write_Byte(u8 dat);//写入一个字节
u8 DS18B20_Read_Byte(void);//读出一个字节
u8 DS18B20_Read_Bit(void);//读出一个位
u8 DS18B20_Check(void);//检测是否存在DS18B20
void DS18B20_Rst(void);//复位DS18B20    

float Get_Temp_Val(void);//获取温度















