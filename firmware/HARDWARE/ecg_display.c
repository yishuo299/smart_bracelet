#include "ecg_display.h"
#include "lcd.h"
#include <string.h>
#include <stdio.h>

static ECG_Rolling_t ecg = {0};
static uint16_t draw_x = 0;           // 当前绘制X坐标（步进单位）
static uint16_t last_y = 0;            // 上一个点的Y坐标
static uint16_t clear_width = 5;       // 清除宽度（5列）
static uint8_t wrap_flag = 0;          // 换行标志

/**
 * @brief 初始化心电图显示
 */
void ECG_Rolling_Init(void)
{
    uint16_t i;
    
    memset(&ecg, 0, sizeof(ecg));
    
    ecg.write_index = 0;
    ecg.heart_rate = 0;
    ecg.lead_off = 0;
    
    // 初始化缓冲区为基线值
    for(i = 0; i < ECG_BUFFER_SIZE; i++)
    {
        ecg.buffer[i] = ECG_BASELINE_Y;
    }
    
    draw_x = ECG_DISPLAY_X;
    last_y = ECG_BASELINE_Y;
    wrap_flag = 0;
    
    // 清屏
    LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
    
    // 绘制网格
    ECG_Rolling_DrawGrid();
    
    // 绘制信息区域
    ECG_Rolling_DrawInfo();
}

/**
 * @brief 绘制心电图网格
 */
void ECG_Rolling_DrawGrid(void)
{
    uint16_t x, y;
    uint16_t grid_color = GRAY;
    
    for(y = ECG_DISPLAY_Y; y <= ECG_DISPLAY_Y + ECG_DISPLAY_HEIGHT; y += 20)
    {
        LCD_DrawLine(ECG_DISPLAY_X, y, ECG_DISPLAY_X + ECG_DISPLAY_WIDTH - 1, y, grid_color);
    }
    
    LCD_DrawLine(ECG_DISPLAY_X, ECG_BASELINE_Y, 
                 ECG_DISPLAY_X + ECG_DISPLAY_WIDTH - 1, ECG_BASELINE_Y, RED);
    
    for(x = ECG_DISPLAY_X + 20; x < ECG_DISPLAY_X + ECG_DISPLAY_WIDTH; x += 20)
    {
        LCD_DrawLine(x, ECG_DISPLAY_Y, x, ECG_DISPLAY_Y + ECG_DISPLAY_HEIGHT, grid_color);
    }
    
    LCD_DrawRectangle(ECG_DISPLAY_X, ECG_DISPLAY_Y, 
                      ECG_DISPLAY_X + ECG_DISPLAY_WIDTH - 1, 
                      ECG_DISPLAY_Y + ECG_DISPLAY_HEIGHT, WHITE);
}

/**
 * @brief 重绘指定区域的网格
 */
void ECG_Rolling_RedrawGridArea(uint16_t x_start, uint16_t x_end)
{
    uint16_t x, y;
    uint16_t grid_color = GRAY;
    
    if(x_start < ECG_DISPLAY_X) x_start = ECG_DISPLAY_X;
    if(x_end > ECG_DISPLAY_X + ECG_DISPLAY_WIDTH - 1) x_end = ECG_DISPLAY_X + ECG_DISPLAY_WIDTH - 1;
    if(x_start > x_end) return;
    
    for(y = ECG_DISPLAY_Y; y <= ECG_DISPLAY_Y + ECG_DISPLAY_HEIGHT; y += 20)
    {
        LCD_DrawLine(x_start, y, x_end, y, grid_color);
    }
    
    LCD_DrawLine(x_start, ECG_BASELINE_Y, x_end, ECG_BASELINE_Y, RED);
    
    for(x = x_start; x <= x_end; x++)
    {
        if((x - ECG_DISPLAY_X) % 20 == 0)
        {
            LCD_DrawLine(x, ECG_DISPLAY_Y, x, ECG_DISPLAY_Y + ECG_DISPLAY_HEIGHT, grid_color);
        }
    }
    
    if(x_start == ECG_DISPLAY_X)
    {
        LCD_DrawLine(ECG_DISPLAY_X, ECG_DISPLAY_Y, ECG_DISPLAY_X, ECG_DISPLAY_Y + ECG_DISPLAY_HEIGHT, WHITE);
    }
    if(x_end == ECG_DISPLAY_X + ECG_DISPLAY_WIDTH - 1)
    {
        LCD_DrawLine(ECG_DISPLAY_X + ECG_DISPLAY_WIDTH - 1, ECG_DISPLAY_Y, 
                     ECG_DISPLAY_X + ECG_DISPLAY_WIDTH - 1, ECG_DISPLAY_Y + ECG_DISPLAY_HEIGHT, WHITE);
    }
}

/**
 * @brief 清除指定X坐标区域的波形（使用步进值计算清除范围）
 */
void ECG_Rolling_ClearWaveArea(uint16_t x_center)
{
    uint16_t x_start, x_end;
    
    // 使用步进值计算清除范围
    // 清除从 x_center 开始的 clear_width * ECG_STEP 列
    x_start = x_center;
    x_end = x_center + clear_width * ECG_STEP - 1;
    
    if(x_start < ECG_DISPLAY_X) x_start = ECG_DISPLAY_X;
    if(x_end >= ECG_DISPLAY_X + ECG_DISPLAY_WIDTH) x_end = ECG_DISPLAY_X + ECG_DISPLAY_WIDTH - 1;
    if(x_start > x_end) return;
    
    LCD_Fill(x_start, ECG_DISPLAY_Y, x_end, ECG_DISPLAY_Y + ECG_DISPLAY_HEIGHT, BLACK);
    ECG_Rolling_RedrawGridArea(x_start, x_end);
}

/**
 * @brief 绘制信息区域
 */
void ECG_Rolling_DrawInfo(void)
{
    char str[32];
    
    LCD_ShowString(10, 5, (uint8_t*)"ECG Monitor", GREEN, BLACK, 16, 0);
    LCD_ShowString(10, 28, (uint8_t*)"HR:", WHITE, BLACK, 12, 0);
    
    if(ecg.heart_rate > 0)
    {
        sprintf(str, "%3d", ecg.heart_rate);
        LCD_ShowString(35, 28, (uint8_t*)str, YELLOW, BLACK, 16, 0);
        LCD_ShowString(70, 28, (uint8_t*)"BPM", WHITE, BLACK, 12, 0);
    }
    else
    {
        LCD_ShowString(35, 28, (uint8_t*)"---", YELLOW, BLACK, 16, 0);
    }
    
    if(ecg.lead_off)
    {
        LCD_ShowString(180, 5, (uint8_t*)"LEAD OFF", RED, BLACK, 12, 0);
    }
    else
    {
        LCD_Fill(180, 5, 235, 17, BLACK);
    }
    
    LCD_DrawLine(0, 48, LCD_W, 48, GRAY);
}

/**
 * @brief 设置心率
 */
void ECG_Rolling_SetHeartRate(uint16_t hr)
{
    char str[32];
    ecg.heart_rate = hr;
    
    LCD_Fill(35, 28, 68, 44, BLACK);
    if(hr > 0)
    {
        sprintf(str, "%3d", hr);
        LCD_ShowString(35, 28, (uint8_t*)str, YELLOW, BLACK, 16, 0);
    }
    else
    {
        LCD_ShowString(35, 28, (uint8_t*)"---", YELLOW, BLACK, 16, 0);
    }
}

/**
 * @brief 设置导联脱落状态
 */
void ECG_Rolling_SetLeadOff(uint8_t off)
{
    ecg.lead_off = off;
    if(off)
    {
        LCD_ShowString(180, 5, (uint8_t*)"LEAD OFF", RED, BLACK, 12, 0);
    }
    else
    {
        LCD_Fill(180, 5, 235, 17, BLACK);
    }
}

/**
 * @brief 添加ECG数据点
 */
void ECG_Rolling_AddPoint(uint16_t adc_value)
{
    uint16_t y;
    
    y = ECG_DISPLAY_Y + (4095 - adc_value) * ECG_DISPLAY_HEIGHT / 4095;
    
    if(y < ECG_DISPLAY_Y) y = ECG_DISPLAY_Y;
    if(y > ECG_DISPLAY_Y + ECG_DISPLAY_HEIGHT) y = ECG_DISPLAY_Y + ECG_DISPLAY_HEIGHT;
    
    ecg.buffer[ecg.write_index] = y;
    
    ecg.write_index++;
    if(ecg.write_index >= ECG_BUFFER_SIZE)
    {
        ecg.write_index = 0;
    }
}

/**
 * @brief 动态绘制心电图（带步进控制）
 */
void ECG_Rolling_Update(void)
{
    uint16_t current_y;
    uint16_t prev_y;
    uint16_t prev_x;
    uint16_t current_index;
    uint16_t prev_index;
    
    // 获取当前点坐标（最新的采样点）
    current_index = (ecg.write_index == 0) ? (ECG_BUFFER_SIZE - 1) : (ecg.write_index - 1);
    current_y = ecg.buffer[current_index];
    
    // 获取上一个点坐标（前一个显示的点）
    prev_index = (current_index - 1 + ECG_BUFFER_SIZE) % ECG_BUFFER_SIZE;
    prev_y = ecg.buffer[prev_index];
    
    // 计算上一个点的X坐标（使用步进值）
    if(draw_x == ECG_DISPLAY_X)
    {
        prev_x = ECG_DISPLAY_X + ECG_DISPLAY_WIDTH - ECG_STEP;
    }
    else
    {
        prev_x = draw_x - ECG_STEP;
    }
    
    // 检查是否需要换行
    if(draw_x + ECG_STEP > ECG_DISPLAY_X + ECG_DISPLAY_WIDTH)
    {
        draw_x = ECG_DISPLAY_X;
        wrap_flag = 1;
        
        // 清除左边即将绘制的区域
        ECG_Rolling_ClearWaveArea(draw_x);
        
        return;
    }
    
    // 正常绘制：先清除即将绘制的位置
    ECG_Rolling_ClearWaveArea(draw_x);
    
    // 绘制线段（避免换行时的斜线）
    if(!wrap_flag && draw_x > ECG_DISPLAY_X)
    {
        LCD_DrawLine(prev_x, prev_y, draw_x, current_y, GREEN);
    }
    else if(draw_x == ECG_DISPLAY_X && !wrap_flag)
    {
        // 第一个点，只画点不画线
        LCD_DrawPoint(draw_x, current_y, GREEN);
    }
    
    // 重置换行标志
    wrap_flag = 0;
    
    // 保存当前Y坐标
    last_y = current_y;
    
    // 更新绘制X坐标（步进值为ECG_STEP）
    draw_x += ECG_STEP;
}