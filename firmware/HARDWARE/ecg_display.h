#ifndef __ECG_ROLLING_H
#define __ECG_ROLLING_H

#include "sys.h"
#include "lcd.h"

// 显示区域定义
#define ECG_DISPLAY_X       0
#define ECG_DISPLAY_Y       50
#define ECG_DISPLAY_WIDTH   240
#define ECG_DISPLAY_HEIGHT  170

// 波形参数
#define ECG_BASELINE_Y      (ECG_DISPLAY_Y + ECG_DISPLAY_HEIGHT / 2)

// 步进控制（每个波形点占用的像素宽度）
// ECG_STEP = 6 时，一个屏幕显示 240/6 = 40 个波形点
#define ECG_STEP            6

// 滚动参数
#define ECG_BUFFER_SIZE     500

// 心电图显示结构体
typedef struct {
    uint16_t buffer[ECG_BUFFER_SIZE];
    uint16_t write_index;
    uint16_t heart_rate;
    uint8_t  lead_off;
} ECG_Rolling_t;

// 函数声明
void ECG_Rolling_Init(void);
void ECG_Rolling_AddPoint(uint16_t adc_value);
void ECG_Rolling_Update(void);
void ECG_Rolling_SetHeartRate(uint16_t hr);
void ECG_Rolling_SetLeadOff(uint8_t off);
void ECG_Rolling_DrawGrid(void);
void ECG_Rolling_DrawInfo(void);
void ECG_Rolling_ClearWaveArea(uint16_t x_center);
void ECG_Rolling_RedrawGridArea(uint16_t x_start, uint16_t x_end);

#endif