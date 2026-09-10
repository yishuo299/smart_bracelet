#include "mpu6050.h"
#include "delay.h"
#include "stdio.h"
//#include "oled.h"

//初始化IIC
void MPU_IIC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB, ENABLE );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11;//使用模拟IIC
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD ;   //开漏输出模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    MPU_IIC_Idle_State();
}

//IIC空闲状态
//当IIC总线的SDA和SCL两条信号线同时处于高电平时，规定为IIC总线的空闲状态
void MPU_IIC_Idle_State()
{
    MPU_IIC_SDA_H;
    MPU_IIC_SCL_H;
    delay_us(4);
}

//IIC开始信号
//当IIC SCL线处于高电平时，SDA线由高电平向低电平跳变，为IIC开始信号，配置开始信号前必须保证IIC总线处于空闲状态
void MPU_IIC_Start()
{
    MPU_IIC_SDA_H;
    MPU_IIC_SCL_H;
    delay_us(4);
    MPU_IIC_SDA_L;
    delay_us(4);
    MPU_IIC_SCL_L;
    delay_us(4);
}

//IIC停止信号
//当IIC SCL线处于高电平时，SDA线由低电平向高电平跳变，为IIC停止信号
void MPU_IIC_Stop()
{
    MPU_IIC_SDA_L;
    MPU_IIC_SCL_H;
    delay_us(4);
    MPU_IIC_SDA_H;
}

//IIC发送一个字节数据（即8bit）
void MPU_IIC_Send_Byte(u8 data)
{
    u8 i;
    //先发送字节的高位bit7
    for (i = 0; i < 8; i++)
    {
        if (data & 0x80)  //判断8位数据每一位的值（0或1）
        {
            MPU_IIC_SDA_H;
        }
        else
        {
            MPU_IIC_SDA_L;
        }

        delay_us(4);      //控制SCL线产生高低电平跳变，产生通讯时钟，同时利用延时函数在SCL为高电平期间读取SDA线电平逻辑
        MPU_IIC_SCL_H;
        delay_us(4);
        MPU_IIC_SCL_L;

        if (i == 7)
        {
            MPU_IIC_SDA_H;    //控制SDA线输出高电平，释放总线，等待接收方应答信号
        }

        data <<= 1;       //左移一个bit
        delay_us(4);
    }
}

//IIC读取一个字节
u8 MPU_IIC_Read_Byte(unsigned char ack)
{
    u8 i;
    u8 value;
    //读到第1个bit为数据的bit7
    value = 0;
    for(i = 0; i < 8; i++)
    {
        value <<= 1;
        MPU_IIC_SCL_H;
        delay_us(4);
        if (MPU_IIC_SDA_READ()) //利用延时函数在SCL为高电平期间读取SDA线电平逻辑
        {
            value++;
        }
        MPU_IIC_SCL_L;
        delay_us(4);
    }
    if (!ack)
        MPU_IIC_NACK();//发送nACK
    else
        MPU_IIC_ACK(); //发送ACK
    return value;
}


//IIC等待应答信号
u8 MPU_IIC_Wait_Ack(void)
{
    uint8_t rvalue;

    MPU_IIC_SDA_H;     //发送端释放SDA总线，由接收端控制SDA线
    delay_us(4);
    MPU_IIC_SCL_H;     //在SCL为高电平期间等待响应，若SDA线为高电平，表示NACK信号，反之则为ACK信号
    delay_us(4);
    if(MPU_IIC_SDA_READ())  //读取SDA线状态判断响应类型，高电平，返回去，为NACK信号，反之则为ACK信号
    {
        rvalue = 1;
    }
    else
    {
        rvalue = 0;
    }
    MPU_IIC_SCL_L;
    delay_us(4);
    return rvalue;
}

//产生应答信号ACK
void MPU_IIC_ACK(void)
{
    MPU_IIC_SDA_L;
    delay_us(4);
    MPU_IIC_SCL_H;   //在SCL线为高电平期间读取SDA线为低电平，则为ACK响应
    delay_us(4);
    MPU_IIC_SCL_L;
    delay_us(4);
    MPU_IIC_SDA_H;
}

//产生非应答信号NACK
void MPU_IIC_NACK(void)
{
    MPU_IIC_SDA_H;
    delay_us(4);
    MPU_IIC_SCL_H;   //在SCL线为高电平期间读取SDA线为高电平，则为NACK响应
    delay_us(4);
    MPU_IIC_SCL_L;
    delay_us(4);
}




//初始化MPU6050
//返回值:0,成功
//    其他,错误代码
u8 MPU_Init(void)
{
    u8 res;
    MPU_IIC_Init();                             //初始化IIC总线
    MPU_Write_Byte(MPU_PWR_MGMT1_REG,0X80); //复位MPU6050
    delay_ms(100);
    MPU_Write_Byte(MPU_PWR_MGMT1_REG,0X00); //唤醒MPU6050
    MPU_Set_Gyro_Fsr(3);                    //陀螺仪传感器,±2000dps
    MPU_Set_Accel_Fsr(0);                   //加速度传感器,±2g
    MPU_Set_Rate(50);                       //设置采样率50Hz
    MPU_Write_Byte(MPU_INT_EN_REG,0X00);    //关闭所有中断
    MPU_Write_Byte(MPU_USER_CTRL_REG,0X00); //I2C主模式关闭
    MPU_Write_Byte(MPU_FIFO_EN_REG,0X00);   //关闭FIFO
    MPU_Write_Byte(MPU_INTBP_CFG_REG,0X80); //INT引脚低电平有效
    res=MPU_Read_Byte(MPU_DEVICE_ID_REG);
    if(res==MPU_ADDR)//器件ID正确
    {
        MPU_Write_Byte(MPU_PWR_MGMT1_REG,0X01); //设置CLKSEL,PLL X轴为参考
        MPU_Write_Byte(MPU_PWR_MGMT2_REG,0X00); //加速度与陀螺仪都工作
        MPU_Set_Rate(50);                       //设置采样率为50Hz
    }else return 1;
    return 0;
}

//设置MPU6050陀螺仪传感器满量程范围
//fsr:0,±250dps;1,±500dps;2,±1000dps;3,±2000dps
//返回值:0,设置成功
//    其他,设置失败
u8 MPU_Set_Gyro_Fsr(u8 fsr)
{
    return MPU_Write_Byte(MPU_GYRO_CFG_REG,fsr<<3);//设置陀螺仪满量程范围
}

//设置MPU6050加速度传感器满量程范围
//fsr:0,±2g;1,±4g;2,±8g;3,±16g
//返回值:0,设置成功
//    其他,设置失败
u8 MPU_Set_Accel_Fsr(u8 fsr)
{
    return MPU_Write_Byte(MPU_ACCEL_CFG_REG,fsr<<3);//设置加速度传感器满量程范围
}

//设置MPU6050的数字低通滤波器
//lpf:数字低通滤波频率(Hz)
//返回值:0,设置成功
//    其他,设置失败
u8 MPU_Set_LPF(u16 lpf)
{
    u8 data=0;
    if(lpf>=188)data=1;
    else if(lpf>=98)data=2;
    else if(lpf>=42)data=3;
    else if(lpf>=20)data=4;
    else if(lpf>=10)data=5;
    else data=6;
    return MPU_Write_Byte(MPU_CFG_REG,data);//设置数字低通滤波器
}

//设置MPU6050的采样率(假定Fs=1KHz)
//rate:4~1000(Hz)
//返回值:0,设置成功
//    其他,设置失败
u8 MPU_Set_Rate(u16 rate)
{
    u8 data;
    if(rate>1000)rate=1000;
    if(rate<4)rate=4;
    data=1000/rate-1;
    data=MPU_Write_Byte(MPU_SAMPLE_RATE_REG,data);  //设置数字低通滤波器
    return MPU_Set_LPF(rate/2); //自动设置LPF为采样率的一半
}

//IIC连续写
//addr:器件地址
//reg:寄存器地址
//len:写入长度
//buf:数据区
//返回值:0,正常
//    其他,错误代码
u8 MPU_Write_Len(u8 addr,u8 reg,u8 len,u8 *buf)
{
    u8 i;
    MPU_IIC_Start();
    MPU_IIC_Send_Byte((addr<<1)|0);  //发送器件地址+写命令
    if(MPU_IIC_Wait_Ack())           //等待应答
    {
        MPU_IIC_Stop();
        return 1;
    }
    MPU_IIC_Send_Byte(reg); //写寄存器地址
    MPU_IIC_Wait_Ack();     //等待应答
    for(i=0;i<len;i++)
    {
        MPU_IIC_Send_Byte(buf[i]);  //发送数据
        if(MPU_IIC_Wait_Ack())      //等待ACK
        {
            MPU_IIC_Stop();
            return 1;
        }
    }
    MPU_IIC_Stop();
    return 0;
}

//IIC连续读
//addr:器件地址
//reg:要读取的寄存器地址
//len:要读取的长度
//buf:读取到的数据存储区
//返回值:0,正常
//    其他,错误代码
u8 MPU_Read_Len(u8 addr,u8 reg,u8 len,u8 *buf)
{
    MPU_IIC_Start();
    MPU_IIC_Send_Byte((addr<<1)|0);//发送器件地址+写命令
    if(MPU_IIC_Wait_Ack())  //等待应答
    {
        MPU_IIC_Stop();
        return 1;
    }
    MPU_IIC_Send_Byte(reg); //写寄存器地址
    MPU_IIC_Wait_Ack();     //等待应答
    MPU_IIC_Start();
    MPU_IIC_Send_Byte((addr<<1)|1);//发送器件地址+读命令
    MPU_IIC_Wait_Ack();     //等待应答
    while(len)
    {
        if(len==1)*buf=MPU_IIC_Read_Byte(0);//读数据,发送nACK
        else *buf=MPU_IIC_Read_Byte(1);     //读数据,发送ACK
        len--;
        buf++;
    }
    MPU_IIC_Stop(); //产生一个停止条件
    return 0;
}

//IIC写一个字节
//reg:寄存器地址
//data:数据
//返回值:0,正常
//    其他,错误代码
u8 MPU_Write_Byte(u8 reg,u8 data)
{
    MPU_IIC_Start();
    MPU_IIC_Send_Byte((MPU_ADDR<<1)|0);//发送器件地址+写命令
    if(MPU_IIC_Wait_Ack())  //等待应答
    {
        MPU_IIC_Stop();
        return 1;
    }
    MPU_IIC_Send_Byte(reg); //写寄存器地址
    MPU_IIC_Wait_Ack();     //等待应答
    MPU_IIC_Send_Byte(data);//发送数据
    if(MPU_IIC_Wait_Ack())  //等待ACK
    {
        MPU_IIC_Stop();
        return 1;
    }
    MPU_IIC_Stop();
    return 0;
}

//IIC读一个字节
//reg:寄存器地址
//返回值:读到的数据
u8 MPU_Read_Byte(u8 reg)
{
    u8 res;
    MPU_IIC_Start();
    MPU_IIC_Send_Byte((MPU_ADDR<<1)|0);//发送器件地址+写命令
    MPU_IIC_Wait_Ack();     //等待应答
    MPU_IIC_Send_Byte(reg); //写寄存器地址
    MPU_IIC_Wait_Ack();     //等待应答
    MPU_IIC_Start();
    MPU_IIC_Send_Byte((MPU_ADDR<<1)|1);//发送器件地址+读命令
    MPU_IIC_Wait_Ack();     //等待应答
    res=MPU_IIC_Read_Byte(0);//读取数据,发送nACK
    MPU_IIC_Stop();         //产生一个停止条件
    return res;
}


//得到温度值
//返回值:温度值(扩大了100倍)
short MPU_Get_Temperature(void)
{
    u8 buf[2];
    short raw;
    float temp;
    MPU_Read_Len(MPU_ADDR,MPU_TEMP_OUTH_REG,2,buf);
    raw=((u16)buf[0]<<8)|buf[1];
    temp=36.53+((double)raw)/340;
    return temp*100;
}

//得到陀螺仪值(原始值)
//gx,gy,gz:陀螺仪x,y,z轴的原始读数(带符号)
//返回值:0,成功
//    其他,错误代码
u8 MPU_Get_Gyroscope(u16 *Gyro)
{
    int16_t gyro[3]; // 陀螺仪xyz
    u8 buf[6],res;
    res=MPU_Read_Len(MPU_ADDR,MPU_GYRO_XOUTH_REG,6,buf);
    if(res==0)
    {
        Gyro[0] = (buf[0] << 8) | buf[1];
        Gyro[1] = (buf[2] << 8) | buf[3];
        Gyro[2] = (buf[4] << 8) | buf[5];

//        printf("GYRO:  X=%d   Y=%d   Z=%d  \n",gyro[0],gyro[1],gyro[2]);
    }
    return res;;
}
//得到加速度值(原始值)
//gx,gy,gz:陀螺仪x,y,z轴的原始读数(带符号)
//返回值:0,成功
//    其他,错误代码
u8 MPU_Get_Accelerometer(u16 *Acc)
{
    int16_t acc[3]; // 加速度xyz
    uint8_t buf[6],res;

    res=MPU_Read_Len(MPU_ADDR,MPU_ACCEL_XOUTH_REG,6,buf);
    if(res==0)
    {
        Acc[0] = (buf[0] << 8) | buf[1];
        Acc[1] = (buf[2] << 8) | buf[3];
        Acc[2] = (buf[4] << 8) | buf[5];

//        printf("ACC:  X=%d   Y=%d   Z=%d  \n",acc[0],acc[1],acc[2]);
    }
    return res;;
}


//读取MPU6050的加速度数据，转化成m/s2
void MPU6050_ReturnAcc(float *Acc)
{
    int16_t acc[3]; // 加速度xyz
    uint8_t buf[6],res;

    res=MPU_Read_Len(MPU_ADDR,MPU_ACCEL_XOUTH_REG,6,buf);
    if(res==0)
    {
        acc[0] = (buf[0] << 8) | buf[1];
        acc[1] = (buf[2] << 8) | buf[3];
        acc[2] = (buf[4] << 8) | buf[5];

        Acc[0] = (double)acc[0] / 16384.0 * 9.8;
        Acc[1] = (double)acc[1] / 16384.0 * 9.8;
        Acc[2] = (double)acc[2] / 16384.0 * 9.8;
    }
}

//取MPU6050的角速度数据，转化成°/s
void MPU6050_ReturnGyro(float *Gyro)
{
    int16_t gyro[3]; // 陀螺仪xyz
    u8 buf[6],res;
    res=MPU_Read_Len(MPU_ADDR,MPU_GYRO_XOUTH_REG,6,buf);
    if(res==0)
    {
        gyro[0] = (buf[0] << 8) | buf[1];
        gyro[1] = (buf[2] << 8) | buf[3];
        gyro[2] = (buf[4] << 8) | buf[5];

        Gyro[0] = (double)gyro[0]/16.4;
        Gyro[1] = (double)gyro[1]/16.4;
        Gyro[2] = (double)gyro[2]/16.4;
    }
}


void MPU6050_Test(void)
{
    char mpu_str[50];
    float acce[3]; // 加速度xyz

//    MPU6050_ReturnAcc(acce);//加速度
//		sprintf(mpu_str,"X:%.1f m/s2  ",acce[0]);
//		OLED_ShowString(0,0,(uint8_t *)mpu_str,16);
//		sprintf(mpu_str,"Y:%.1f m/s2  ",acce[1]);
//		OLED_ShowString(0,3,(uint8_t *)mpu_str,16);
//		sprintf(mpu_str,"Z:%.1f m/s2  ",acce[2]);
//		OLED_ShowString(0,6,(uint8_t *)mpu_str,16);
		
}

