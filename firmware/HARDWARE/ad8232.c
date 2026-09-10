#include "ad8232.h"
#include "delay.h"

/**
 * @brief ADC1初始化（使用PB0作为ADC输入）
 * 注意：PB0对应ADC1通道8
 */
static void ADC1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    
    // 使能ADC1和GPIOB时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOB, ENABLE);
    
    // 配置PB0为模拟输入（ADC通道8）
    GPIO_InitStructure.GPIO_Pin = AD8232_OUT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(AD8232_OUT_PORT, &GPIO_InitStructure);
    
    // ADC1配置
    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;  // 连续转换模式
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);
    
    // 配置ADC通道8（对应PB0），采样时间239.5周期
    ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 1, ADC_SampleTime_239Cycles5);
    
    // 使能ADC1
    ADC_Cmd(ADC1, ENABLE);
    
    // 校准ADC
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
    
    // 启动ADC转换
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

/**
 * @brief 初始化AD8232
 */
void AD8232_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIOB时钟（如果之前未使能）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    // 配置SDN引脚为输出（PB1，低电平使能芯片）
    GPIO_InitStructure.GPIO_Pin = AD8232_SDN_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(AD8232_SDN_PORT, &GPIO_InitStructure);
    
    // 初始化ADC
    ADC1_Init();
    
    // 默认启动芯片
    AD8232_Start();
    
    // 延时等待芯片稳定
    delay_ms(100);
}

/**
 * @brief 启动AD8232（使能芯片，SDN拉低）
 */
void AD8232_Start(void)
{
    GPIO_ResetBits(AD8232_SDN_PORT, AD8232_SDN_PIN);
}

/**
 * @brief 停止AD8232（关断芯片，省电模式，SDN拉高）
 */
void AD8232_Stop(void)
{
    GPIO_SetBits(AD8232_SDN_PORT, AD8232_SDN_PIN);
}

/**
 * @brief 读取ADC原始值
 * @return 12位ADC值 (0-4095)
 */
uint16_t AD8232_ReadADC(void)
{
    // 等待转换完成
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    
    return ADC_GetConversionValue(ADC1);
}

/**
 * @brief 读取电压值（单位：mV）
 * @return 电压值，参考电压3.3V，12位ADC
 */
float AD8232_ReadVoltage(void)
{
    uint16_t adc_val = AD8232_ReadADC();
    return (adc_val * 3300.0f) / 4095.0f;
}