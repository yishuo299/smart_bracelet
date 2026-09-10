#include "max30102.h"
#include "max30102_fir.h"
#include "delay.h"
//#include "Sensor_Task.h"
MaxXlXy maxXlXy;
#if MAX30102_START_STATE
/*  VCC<->3.3V
    GND<->GND
    SCL<->PXX
    SDA<->PXX
    INT<->PXX
    */
//MAX30102引脚输出模式控制
void MAX30102_IIC_SDA_OUT(void)//SDA输出方向配置
{
    GPIO_InitTypeDef GPIO_InitStructure;    
    
    GPIO_InitStructure.GPIO_Pin=MAX30102_IIC_SDA_PIN;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;//SDA推挽输出
    GPIO_Init(MAX30102_IIC_PORT,&GPIO_InitStructure);                         

}

void MAX30102_IIC_SDA_IN(void)//SDA输入方向配置
{
    GPIO_InitTypeDef GPIO_InitStructure;    
    
    GPIO_InitStructure.GPIO_Pin=MAX30102_IIC_SDA_PIN;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;//SCL上拉输入
    GPIO_Init(MAX30102_IIC_PORT,&GPIO_InitStructure);
    
}

//产生IIC起始信号
void MAX30102_IIC_Start(void)
{
    MAX30102_IIC_SDA_OUT();     //sda线输出
    MAX30102_IIC_SDA=1;            
    MAX30102_IIC_SCL=1;
    delay_us(4);
     MAX30102_IIC_SDA=0;//START:when CLK is high,DATA change form high to low 
    delay_us(4);
    MAX30102_IIC_SCL=0;//钳住I2C总线，准备发送或接收数据 
}      
//产生IIC停止信号
void MAX30102_IIC_Stop(void)
{
    MAX30102_IIC_SDA_OUT();//sda线输出
    MAX30102_IIC_SCL=0;
    MAX30102_IIC_SDA=0;//STOP:when CLK is high DATA change form low to high
     delay_us(4);
    MAX30102_IIC_SCL=1; 
    MAX30102_IIC_SDA=1;//发送I2C总线结束信号
    delay_us(4);                                   
}
//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
uint8_t MAX30102_IIC_Wait_Ack(void)
{
    uint8_t ucErrTime=0;
    MAX30102_IIC_SDA_IN();      //SDA设置为输入  
    MAX30102_IIC_SDA=1;delay_us(1);       
    MAX30102_IIC_SCL=1;delay_us(1);     
    while(MAX30102_READ_SDA)
    {
        ucErrTime++;
        if(ucErrTime>250)
        {
            MAX30102_IIC_Stop();
            return 1;
        }
    }
    MAX30102_IIC_SCL=0;//时钟输出0        
    return 0;  
} 
//产生ACK应答
void MAX30102_IIC_Ack(void)
{
    MAX30102_IIC_SCL=0;
    MAX30102_IIC_SDA_OUT();
    MAX30102_IIC_SDA=0;
    delay_us(2);
    MAX30102_IIC_SCL=1;
    delay_us(2);
    MAX30102_IIC_SCL=0;
}
//不产生ACK应答            
void MAX30102_IIC_NAck(void)
{
    MAX30102_IIC_SCL=0;
    MAX30102_IIC_SDA_OUT();
    MAX30102_IIC_SDA=1;
    delay_us(2);
    MAX30102_IIC_SCL=1;
    delay_us(2);
    MAX30102_IIC_SCL=0;
}                                          
//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答              
void MAX30102_IIC_Send_Byte(uint8_t txd)
{                        
    uint8_t t;   
    MAX30102_IIC_SDA_OUT();         
    MAX30102_IIC_SCL=0;//拉低时钟开始数据传输
    for(t=0;t<8;t++)
    {              
        MAX30102_IIC_SDA=(txd&0x80)>>7;
        txd<<=1;       
        delay_us(2);   //对TEA5767这三个延时都是必须的
        MAX30102_IIC_SCL=1;
        delay_us(2); 
        MAX30102_IIC_SCL=0;    
        delay_us(2);
    }     
}         
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK   
uint8_t MAX30102_IIC_Read_Byte(unsigned char ack)
{
    unsigned char i,receive=0;
    MAX30102_IIC_SDA_IN();//SDA设置为输入
    for(i=0;i<8;i++ )
    {
        MAX30102_IIC_SCL=0; 
        delay_us(2);
        MAX30102_IIC_SCL=1;
        receive<<=1;
        if(MAX30102_READ_SDA)receive++;   
        delay_us(1); 
    }                     
    if (!ack)
        MAX30102_IIC_NAck();//发送nACK
    else
        MAX30102_IIC_Ack(); //发送ACK   
    return receive;
}
/***********************************************************************************/
void MAX30102_IIC_Read_One_Byte(uint8_t daddr,uint8_t addr,uint8_t* data)
{                                                                                                 
    MAX30102_IIC_Start();  
    
    MAX30102_IIC_Send_Byte(daddr);       //发送写命令
    MAX30102_IIC_Wait_Ack();
    MAX30102_IIC_Send_Byte(addr);//发送地址
    MAX30102_IIC_Wait_Ack();         
    MAX30102_IIC_Start();              
    MAX30102_IIC_Send_Byte(daddr|0X01);//进入接收模式               
    MAX30102_IIC_Wait_Ack();     
    *data = MAX30102_IIC_Read_Byte(0);           
    MAX30102_IIC_Stop();//产生一个停止条件        
}

void MAX30102_IIC_Write_One_Byte(uint8_t daddr,uint8_t addr,uint8_t data)
{                                                                                                  
    MAX30102_IIC_Start();  
    
    MAX30102_IIC_Send_Byte(daddr);        //发送写命令
    MAX30102_IIC_Wait_Ack();
    MAX30102_IIC_Send_Byte(addr);//发送地址
    MAX30102_IIC_Wait_Ack();                                                                 
    MAX30102_IIC_Send_Byte(data);     //发送字节                               
    MAX30102_IIC_Wait_Ack();                     
    MAX30102_IIC_Stop();//产生一个停止条件 
    delay_ms(10);     
}
uint8_t max30102_Bus_Read(uint8_t Register_Address)
{
    uint8_t  data;


    /* 第1步：发起I2C总线启动信号 */
    MAX30102_IIC_Start();

    /* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    MAX30102_IIC_Send_Byte(I2C_WRITE_ADDR | I2C_WR);    /* 此处是写指令 */

    /* 第3步：发送ACK */
    if (MAX30102_IIC_Wait_Ack() != 0)
    {
        goto cmd_fail;    /* EEPROM器件无应答 */
    }

    /* 第4步：发送字节地址， */
    MAX30102_IIC_Send_Byte((uint8_t)Register_Address);
    if (MAX30102_IIC_Wait_Ack() != 0)
    {
        goto cmd_fail;    /* EEPROM器件无应答 */
    }
    

    /* 第6步：重新启动I2C总线。下面开始读取数据 */
    MAX30102_IIC_Start();

    /* 第7步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    MAX30102_IIC_Send_Byte(I2C_WRITE_ADDR | I2C_RD);    /* 此处是读指令 */

    /* 第8步：发送ACK */
    if (MAX30102_IIC_Wait_Ack() != 0)
    {
        goto cmd_fail;    /* EEPROM器件无应答 */
    }

    /* 第9步：读取数据 */
    {
        data = MAX30102_IIC_Read_Byte(0);    /* 读1个字节 */

        MAX30102_IIC_NAck();    /* 最后1个字节读完后，CPU产生NACK信号(驱动SDA = 1) */
    }
    /* 发送I2C总线停止信号 */
    MAX30102_IIC_Stop();
    return data;    /* 执行成功 返回data值 */

cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
    /* 发送I2C总线停止信号 */
    MAX30102_IIC_Stop();
    return 0;
}
void max30102_FIFO_ReadBytes(uint8_t Register_Address,uint8_t* Data)
{    
    max30102_Bus_Read(INTERRUPT_STATUS1);
    max30102_Bus_Read(INTERRUPT_STATUS2);
    
    /* 第1步：发起I2C总线启动信号 */
    MAX30102_IIC_Start();

    /* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    MAX30102_IIC_Send_Byte(I2C_WRITE_ADDR | I2C_WR);    /* 此处是写指令 */

    /* 第3步：发送ACK */
    if (MAX30102_IIC_Wait_Ack() != 0)
    {
        goto cmd_fail;    /* EEPROM器件无应答 */
    }

    /* 第4步：发送字节地址， */
    MAX30102_IIC_Send_Byte((uint8_t)Register_Address);
    if (MAX30102_IIC_Wait_Ack() != 0)
    {
        goto cmd_fail;    /* EEPROM器件无应答 */
    }
    

    /* 第6步：重新启动I2C总线。下面开始读取数据 */
    MAX30102_IIC_Start();

    /* 第7步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    MAX30102_IIC_Send_Byte(I2C_WRITE_ADDR | I2C_RD);    /* 此处是读指令 */

    /* 第8步：发送ACK */
    if (MAX30102_IIC_Wait_Ack() != 0)
    {
        goto cmd_fail;    /* EEPROM器件无应答 */
    }

    /* 第9步：读取数据 */
    Data[0] = MAX30102_IIC_Read_Byte(1);    
    Data[1] = MAX30102_IIC_Read_Byte(1);    
    Data[2] = MAX30102_IIC_Read_Byte(1);    
    Data[3] = MAX30102_IIC_Read_Byte(1);
    Data[4] = MAX30102_IIC_Read_Byte(1);    
    Data[5] = MAX30102_IIC_Read_Byte(0);
    /* 最后1个字节读完后，CPU产生NACK信号(驱动SDA = 1) */
    /* 发送I2C总线停止信号 */
    MAX30102_IIC_Stop();

cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
    /* 发送I2C总线停止信号 */
    MAX30102_IIC_Stop();
}
void maxim_max30102_write_reg(uint8_t uch_addr, uint8_t uch_data)
{
    MAX30102_IIC_Write_One_Byte(I2C_WRITE_ADDR,uch_addr,uch_data);
}

void maxim_max30102_read_reg(uint8_t uch_addr, uint8_t *puch_data)
{
    MAX30102_IIC_Read_One_Byte(I2C_WRITE_ADDR,uch_addr,puch_data);
}


//初始化IIC
void MAX30102_IIC_Init(void)
{                         
    GPIO_InitTypeDef GPIO_InitStructure;
    //RCC->APB2ENR|=1<<4;//先使能外设IO PORTC时钟 
    RCC_APB2PeriphClockCmd(    RCC_APB2Periph_GPIOB, ENABLE );    
       
    GPIO_InitStructure.GPIO_Pin = MAX30102_IIC_SCL_PIN|MAX30102_IIC_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;   //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MAX30102_IIC_PORT, &GPIO_InitStructure);
 
    MAX30102_IIC_SCL=1;
    MAX30102_IIC_SDA=1;

}
void MAX30102_INT_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure; 
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;/* 配置 NVIC 中断*/

    /*开启按键GPIO口的时钟*/
    RCC_APB2PeriphClockCmd( MAX30102_INT_GPIO_CLK, ENABLE);
                                            
    

    /* 配置NVIC为优先级组1 */
    /* 提示 NVIC_PriorityGroupConfig() 在整个工程只需要调用一次来配置优先级分组*/
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* 配置中断源：按键1 */
    NVIC_InitStructure.NVIC_IRQChannel = MAX30102_INT_EXTI_IRQn;
    /* 配置抢占优先级 */
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    /* 配置子优先级 */
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    /* 使能中断通道 */
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /*--------------------------IO配置-----------------------------*/
    /* 选择按键用到的GPIO */	
    GPIO_InitStructure.GPIO_Pin = MAX30102_INT_PIN;
    /* 配置为浮空输入 */	
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(MAX30102_INT_GPIO_PORT, &GPIO_InitStructure);

    /* 选择EXTI的信号源 */
    GPIO_EXTILineConfig(MAX30102_INT_EXTI_PORTSOURCE, MAX30102_INT_EXTI_PINSOURCE); 
    EXTI_InitStructure.EXTI_Line = MAX30102_INT_EXTI_LINE;

    /* EXTI为中断模式 */
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    /* 上升沿中断 */
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    /* 使能中断 */	
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
}


/****************************************************************************************************/
void max30102_i2c_write(uint8_t reg_adder,uint8_t data)
{
    maxim_max30102_write_reg(reg_adder,data); 
}

void MAX30102_Init(void)
{ 
    uint8_t data;
    
    MAX30102_IIC_Init();    //初始化I2C接口
    delay_ms(500);
    
    MAX30102_INT_Init();    //中断引脚配置
    
    max30102_i2c_write(MODE_CONFIGURATION,0x40);  //reset the device
    
    delay_ms(5);
    
    max30102_i2c_write(INTERRUPT_ENABLE1,0xE0);
    max30102_i2c_write(INTERRUPT_ENABLE2,0x00);  //interrupt enable: FIFO almost full flag, new FIFO Data Ready,

    max30102_i2c_write(FIFO_WR_POINTER,0x00);
    max30102_i2c_write(FIFO_OV_COUNTER,0x00);
    max30102_i2c_write(FIFO_RD_POINTER,0x00);   //clear the pointer
    
    max30102_i2c_write(FIFO_CONFIGURATION,0x4F); //FIFO configuration: sample averaging(4),FIFO rolls on full(0), FIFO almost full value(15 empty data samples when interrupt is issued)  
    
    max30102_i2c_write(MODE_CONFIGURATION,0x03);  //MODE configuration:SpO2 mode
    
    max30102_i2c_write(SPO2_CONFIGURATION,0x2A); //SpO2 configuration:ACD resolution:15.63pA,sample rate control:200Hz, LED pulse width:215 us 
    
    max30102_i2c_write(LED1_PULSE_AMPLITUDE,0x2f);    //IR LED
    max30102_i2c_write(LED2_PULSE_AMPLITUDE,0x2f); //RED LED current
    
    max30102_i2c_write(TEMPERATURE_CONFIG,0x01);   //temp
    
    
    maxim_max30102_read_reg(INTERRUPT_STATUS1,&data);
    maxim_max30102_read_reg(INTERRUPT_STATUS2,&data);
    /* FIR滤波计算初始化 */
    max30102_fir_init();
}

void MAX30102_FIFO_Read(float *output_data)
{
    uint8_t receive_data[6];
    uint32_t data[2];
    max30102_FIFO_ReadBytes(FIFO_DATA, &receive_data[0]);
    data[0] = ((receive_data[0]<<16 | receive_data[1]<<8 | receive_data[2]) & 0x03ffff);
    data[1] = ((receive_data[3]<<16 | receive_data[4]<<8 | receive_data[5]) & 0x03ffff);
    *output_data = data[0];
    *(output_data+1) = data[1];
}

uint16_t MAX30102_getHeartRate(float *input_data,uint16_t cache_nums)
{
        float input_data_sum_aver = 0;
        uint16_t i,temp;
        
        
        for(i=0;i<cache_nums;i++)
        {
        input_data_sum_aver += *(input_data+i);
        }
        input_data_sum_aver = input_data_sum_aver/cache_nums;
        for(i=0;i<cache_nums;i++)
        {
                if((*(input_data+i)>input_data_sum_aver)&&(*(input_data+i+1)<input_data_sum_aver))
                {
                    temp = i;
                    break;
                }
        }
        i++;
        for(;i<cache_nums;i++)
        {
                if((*(input_data+i)>input_data_sum_aver)&&(*(input_data+i+1)<input_data_sum_aver))
                {
                    temp = i - temp;
                    break;
                }
        }
        if((temp>14)&&(temp<100))
        {
            //SpO2采样频率200Hz，FIFO采样平均过滤值为4，所以最终采样频率算为50Hz
            return 3000/temp;//采样一次的周期是0.02s，temp是一次心跳过程中的采样点个数，一次心跳就是temp/50秒,一分钟心跳数就是60/(temp/50)
        }
        else
        {
            return 0;
        }
}

float MAX30102_getSpO2(float *ir_input_data,float *red_input_data,uint16_t cache_nums)
{
            float ir_max=*ir_input_data,ir_min=*ir_input_data;
            float red_max=*red_input_data,red_min=*red_input_data;
            float R;
            uint16_t i;
            for(i=1;i<cache_nums;i++)
            {
                if(ir_max<*(ir_input_data+i))
                {
                    ir_max=*(ir_input_data+i);
                }
                if(ir_min>*(ir_input_data+i))
                {
                    ir_min=*(ir_input_data+i);
                }
                if(red_max<*(red_input_data+i))
                {
                    red_max=*(red_input_data+i);
                }
                if(red_min>*(red_input_data+i))
                {
                    red_min=*(red_input_data+i);
                }
            }
            R=((ir_max-ir_min)*red_min)/((red_max-red_min)*ir_min);
//             R=((ir_max+ir_min)*(red_max-red_min))/((red_max+red_min)*(ir_max-ir_min));
            return ((-45.060)*R*R + 30.354*R + 94.845);
}
#define CACHE_NUMS 150//缓存数
#define PPG_DATA_THRESHOLD 100000   //检测阈值
uint8_t max30102_int_flag=0;        //中断标志

float ppg_data_cache_RED[CACHE_NUMS]={0};   //缓存区
float ppg_data_cache_IR[CACHE_NUMS]={0};    //缓存区
uint16_t cache_counter=0;                   //缓存计数器
float max30102_data[2],fir_output[2]; 

void MAX30102_GetXlXy(uint8_t* xl, uint8_t* xy)
{
    MAX30102_FIFO_Read(max30102_data);                      //读取数据

    ir_max30102_fir(&max30102_data[0],&fir_output[0]);      //实测ir数据采集在前面，red数据在后面
    red_max30102_fir(&max30102_data[1],&fir_output[1]);     //滤波

    if((max30102_data[0]>PPG_DATA_THRESHOLD)&&(max30102_data[1]>PPG_DATA_THRESHOLD))  //大于阈值，说明传感器有接触
    {
        ppg_data_cache_IR[cache_counter]=fir_output[0];
        ppg_data_cache_RED[cache_counter]=fir_output[1];
        cache_counter++;
        
    }
    else                //小于阈值
    {
		*xl = 255;
        *xy = 255;
        cache_counter=0;
    }


    if(cache_counter>=CACHE_NUMS)  //收集满了数据
    {
        *xl = (uint8_t)MAX30102_getHeartRate(ppg_data_cache_IR,CACHE_NUMS);
        *xy = (uint8_t)MAX30102_getSpO2(ppg_data_cache_IR,ppg_data_cache_RED,CACHE_NUMS);
        if(*xy > 100) *xy = 100;
        maxXlXy.pre_time = 0;
        cache_counter=0;
    }
}
void MAX30102_INT_IRQHandler(void)
{
  //确保是否产生了EXTI Line中断
    if(EXTI_GetITStatus(MAX30102_INT_EXTI_LINE) != RESET) 
    {
        maxXlXy.int_flag = 1;
        //清除中断标志位
        EXTI_ClearITPendingBit(MAX30102_INT_EXTI_LINE);     
    }  
}
#endif

