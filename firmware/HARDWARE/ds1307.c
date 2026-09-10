#include "stm32f10x.h"   // Device header
#include "DS1307.h"
#include "delay.h"
#include "math.h"
//#include "DS1307_Reg.h"
//#include "MyI2C.h"

//DS1307从机地址：1101000b  b=0写   b=1读
#define DS1307_ADDRESS 	    0xD0//DS1307的地址

//初始化IIC
void DS1307_IIC_Init(void)
{					     
	GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);  // 关闭JTAG，保留SWD
	DS1307_IIC_SCL_GPIO_CLK_ENABLE();	//使能GPIOB时钟
	DS1307_IIC_SDA_GPIO_CLK_ENABLE();
	
	GPIO_InitStructure.GPIO_Pin = DS1307_IIC_SCL_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;   //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(DS1307_IIC_SCL_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = DS1307_IIC_SDA_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;   //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(DS1307_IIC_SDA_GPIO_PORT, &GPIO_InitStructure);
	
    DS1307_IIC_SCL(1);
    DS1307_IIC_SDA(1);
//	GPIO_SetBits(DS1307_IIC_SCL_GPIO_PORT,DS1307_IIC_SCL_GPIO_PIN); 	//PB6,PB7 输出高
//	GPIO_SetBits(DS1307_IIC_SDA_GPIO_PORT,DS1307_IIC_SDA_GPIO_PIN); 	//PB6,PB7 输出高
}


void DS1307_SDA_IN(void)
{					     
	GPIO_InitTypeDef GPIO_InitStructure;
	DS1307_IIC_SDA_GPIO_CLK_ENABLE();//使能GPIOB时钟
	
	GPIO_InitStructure.GPIO_Pin = DS1307_IIC_SDA_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU ;   //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(DS1307_IIC_SDA_GPIO_PORT, &GPIO_InitStructure);
}

void DS1307_SDA_OUT(void)
{					     
	GPIO_InitTypeDef GPIO_InitStructure;
	DS1307_IIC_SDA_GPIO_CLK_ENABLE();//使能GPIOB时钟
	
	GPIO_InitStructure.GPIO_Pin = DS1307_IIC_SDA_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;   //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(DS1307_IIC_SDA_GPIO_PORT, &GPIO_InitStructure);
}


//产生IIC起始信号
void DS1307_IIC_Start(void)
{
	DS1307_SDA_OUT();     //sda线输出
	DS1307_IIC_SDA(1);	  	  
	DS1307_IIC_SCL(1);
	delay_us(4);
 	DS1307_IIC_SDA(0);//START:when CLK is high,DATA change form high to low 
	delay_us(4);
	DS1307_IIC_SCL(0);//钳住I2C总线，准备发送或接收数据 
}	  
//产生IIC停止信号
void DS1307_IIC_Stop(void)
{
	DS1307_SDA_OUT();//sda线输出
	DS1307_IIC_SCL(0);
	DS1307_IIC_SDA(0);//STOP:when CLK is high DATA change form low to high
 	delay_us(4);
	DS1307_IIC_SCL(1); 
	DS1307_IIC_SDA(1);//发送I2C总线结束信号
	delay_us(4);							   	
}
//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
u8 DS1307_IIC_Wait_Ack(void)
{
	u8 ucErrTime=0;
	DS1307_SDA_IN();      //SDA设置为输入  
	DS1307_IIC_SDA(1);delay_us(1);	   
	DS1307_IIC_SCL(1);delay_us(1);	 
	while(DS1307_IIC_READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			DS1307_IIC_Stop();
			return 1;
		}
	}
	DS1307_IIC_SCL(0);//时钟输出0 	   
	return 0;  
} 
//产生ACK应答
void DS1307_IIC_Ack(void)
{
	DS1307_IIC_SCL(0);
	DS1307_SDA_OUT();
	DS1307_IIC_SDA(0);
	delay_us(2);
	DS1307_IIC_SCL(1);
	delay_us(2);
	DS1307_IIC_SCL(0);
}
//不产生ACK应答		    
void DS1307_IIC_NAck(void)
{
	DS1307_IIC_SCL(0);
	DS1307_SDA_OUT();
	DS1307_IIC_SDA(1);
	delay_us(2);
	DS1307_IIC_SCL(1);
	delay_us(2);
	DS1307_IIC_SCL(0);
}					 				     
//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答			  
void DS1307_IIC_Send_Byte(u8 txd)
{                        
    u8 t;   
	DS1307_SDA_OUT(); 	    
    DS1307_IIC_SCL(0);//拉低时钟开始数据传输
    for(t=0;t<8;t++)
    {              
        DS1307_IIC_SDA((BitAction)((txd&0x80)>>7));
        txd<<=1; 	  
		delay_us(2);   //对TEA5767这三个延时都是必须的
		DS1307_IIC_SCL(1);
		delay_us(2); 
		DS1307_IIC_SCL(0);	
		delay_us(2);
    }	 
} 	    
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK   
u8 DS1307_IIC_Read_Byte(unsigned char ack)
{
	unsigned char i,receive=0;
	DS1307_SDA_IN();//SDA设置为输入
    for(i=0;i<8;i++ )
	{
        DS1307_IIC_SCL(0); 
        delay_us(2);
		DS1307_IIC_SCL(1);
        receive<<=1;
        if(DS1307_IIC_READ_SDA)receive++;   
		delay_us(1); 
    }					 
    if (!ack)
        DS1307_IIC_NAck();//发送nACK
    else
        DS1307_IIC_Ack(); //发送ACK   
    return receive;
}



/**
  * 函    数：DS1307写寄存器
  * 参    数：RegAddress 寄存器地址
  * 参    数：Data 要写入寄存器的数据，范围：0x00~0xFF
  * 返 回 值：无
  */
void DS1307_WriteReg(uint8_t RegAddress, uint8_t Data)
{
	DS1307_IIC_Start();						//I2C起始
	DS1307_IIC_Send_Byte(DS1307_ADDRESS);	    //发送从机地址，读写位为0，表示即将写入
	DS1307_IIC_Wait_Ack();					//接收应答
	DS1307_IIC_Send_Byte(RegAddress);			//发送寄存器地址
	DS1307_IIC_Wait_Ack();					//接收应答
	DS1307_IIC_Send_Byte(Data);				//发送要写入寄存器的数据
	DS1307_IIC_Wait_Ack();					//接收应答
	DS1307_IIC_Stop();						//I2C终止
}

/**
  * 函    数：DS1307读寄存器
  * 参    数：RegAddress 寄存器地址
  * 返 回 值：读取寄存器的数据，范围：0x00~0xFF
  */
uint8_t DS1307_ReadReg(uint8_t RegAddress)
{
	uint8_t Data;
	
	DS1307_IIC_Start();						//I2C起始
	DS1307_IIC_Send_Byte(DS1307_ADDRESS);	    //发送从机地址，读写位为0，表示即将写入
	DS1307_IIC_Wait_Ack();					//接收应答
	DS1307_IIC_Send_Byte(RegAddress);			//发送寄存器地址
	DS1307_IIC_Wait_Ack();					//接收应答
	
	DS1307_IIC_Start();						    //I2C重复起始
	DS1307_IIC_Send_Byte(DS1307_ADDRESS | 0x01);	//发送从机地址，读写位为1，表示即将读取
	DS1307_IIC_Wait_Ack();					    //接收应答
//	Data = DS1307_IIC_ReceiveByte();			    //接收指定寄存器的数据
//	DS1307_IIC_SendAck(1);					    //发送应答，给从机非应答，终止从机的数据输出
	Data=DS1307_IIC_Read_Byte(1);
	DS1307_IIC_Stop();						    //I2C终止
	
	return Data;
}

void DS1307_Init(void)
{
	DS1307_IIC_Init();									//先初始化底层的I2C
}

uint8_t BCD2DEC(u8 bcd)
{
    u8 tens = (bcd >> 4) & 0x0F;   // 高4位是十位
    u8 ones = bcd & 0x0F;           // 低4位是个位
    
    // 检查是否是有效的BCD码
    if(tens > 9 || ones > 9) return 0;  // 无效BCD码返回0
    
    return tens * 10 + ones;
}

uint8_t DEC2BCD(u8 dec)
{
	u8 ones;
	u8 tens;
    if(dec > 99) return 0;  // 最大99
    
     tens = dec / 10;
     ones = dec % 10;
    
    return (tens << 4) | ones;
}

/**
 * 时间设置 - 修正版本
 */
void DS1307_SetTime(u16 year, u8 month, u8 date, u8 week, u8 hour, u8 minute, u8 second)
{
    // DS1307只存储年份后两位
    uint8_t year_low = year % 100;
    
    // 停止时钟振荡（设置CH位为1）
    uint8_t sec_reg = DEC2BCD(second) | 0x80;  // CH位=1，停止振荡
    
    DS1307_IIC_Start();
    DS1307_IIC_Send_Byte(DS1307_ADDRESS);
    DS1307_IIC_Wait_Ack();
    DS1307_IIC_Send_Byte(DS1307_REG_SECOND);
    DS1307_IIC_Wait_Ack();
    
    // 写入时间数据（BCD格式）
    DS1307_IIC_Send_Byte(sec_reg);              // 秒 + CH位
    DS1307_IIC_Wait_Ack();
    DS1307_IIC_Send_Byte(DEC2BCD(minute));      // 分
    DS1307_IIC_Wait_Ack();
    DS1307_IIC_Send_Byte(DEC2BCD(hour));        // 时
    DS1307_IIC_Wait_Ack();
    DS1307_IIC_Send_Byte(DEC2BCD(week));        // 星期
    DS1307_IIC_Wait_Ack();
    DS1307_IIC_Send_Byte(DEC2BCD(date));        // 日
    DS1307_IIC_Wait_Ack();
    DS1307_IIC_Send_Byte(DEC2BCD(month));       // 月
    DS1307_IIC_Wait_Ack();
    DS1307_IIC_Send_Byte(DEC2BCD(year_low));    // 年
    DS1307_IIC_Wait_Ack();
    DS1307_IIC_Stop();
    
    // 短暂延时后启动时钟（清除CH位）
    delay_ms(10);
    
    // 重新写入秒寄存器，清除CH位启动时钟
    DS1307_IIC_Start();
    DS1307_IIC_Send_Byte(DS1307_ADDRESS);
    DS1307_IIC_Wait_Ack();
    DS1307_IIC_Send_Byte(DS1307_REG_SECOND);
    DS1307_IIC_Wait_Ack();
    DS1307_IIC_Send_Byte(DEC2BCD(second) & 0x7F);  // CH位=0，启动时钟
    DS1307_IIC_Wait_Ack();
    DS1307_IIC_Stop();
}

/**
 * 获取时间 - 修正版本
 */
void DS1307_GetTime(u16 *year, u8 *month, u8 *date, u8 *week, u8 *hour, u8 *minute, u8 *second)
{
    uint8_t buf[7];
    uint8_t year_low;
    uint8_t i;
    
    // 设置读指针到秒寄存器
    DS1307_IIC_Start();
    DS1307_IIC_Send_Byte(DS1307_ADDRESS);
    DS1307_IIC_Wait_Ack();
    DS1307_IIC_Send_Byte(DS1307_REG_SECOND);
    DS1307_IIC_Wait_Ack();
    
    // 重新开始，准备读取数据
    DS1307_IIC_Start();
    DS1307_IIC_Send_Byte(DS1307_ADDRESS | 0x01);
    DS1307_IIC_Wait_Ack();
    
    // 连续读取7个字节
    for(i = 0; i < 7; i++)
    {
        if(i == 6)
            buf[i] = DS1307_IIC_Read_Byte(0);  // 最后一个字节不发应答
        else
            buf[i] = DS1307_IIC_Read_Byte(1);  // 其他字节发应答
    }
    DS1307_IIC_Stop();
    
    // BCD转十进制
    *second = BCD2DEC(buf[0] & 0x7F);  // 清除CH位
    *minute = BCD2DEC(buf[1]);
    *hour = BCD2DEC(buf[2]);
    *week = BCD2DEC(buf[3]);
    *date = BCD2DEC(buf[4]);
    *month = BCD2DEC(buf[5]);
    year_low = BCD2DEC(buf[6]);
    
    // 年份处理：假设为2000年之后
    *year = 2000 + year_low;
}

/**
 * 设置SQW输出 - 修正版本
 */
void DS1307_SQSET(uint8_t sqmode)
{
    uint8_t ctrl_reg;
    
    switch(sqmode)
    {
        case 0: ctrl_reg = 0x10; break;  // 1Hz  (RS0=0, RS1=0, SQWE=1)
        case 1: ctrl_reg = 0x11; break;  // 4.096kHz
        case 2: ctrl_reg = 0x12; break;  // 8.192kHz
        case 3: ctrl_reg = 0x13; break;  // 32.768kHz
        default: return;
    }
    
    DS1307_WriteReg(DS1307_REG_CONTROL, ctrl_reg);
}


u8 DS1307_Get_Week(u16 year, u8 month, u8 day)
{
	int c=0,y=0,m=0,d=0,week=0;
	if (month == 1 || month == 2) 
	{
		month += 12;
		year--;
	}
	c = year / 100;
	y = year % 100;
	m = month;
	d = day;
	// 蔡勒公式
	week = (d + (13 * (m + 1)) / 5 + y + y / 4 + c / 4 + 5 * c) % 7;
	return ((week + 5) % 7) + 1;
}