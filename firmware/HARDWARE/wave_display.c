#include "wave_display.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define SCREEN_Y_TOP    40      // Y坐标最小值（顶部）
#define SCREEN_Y_BOTTOM 200     // Y坐标最大值（底部）

// 数据转换函数
uint16_t DataToY(float value, float min, float max)
{
    uint16_t y;
    uint16_t range = max - min;
    
    if(range == 0) return SCREEN_Y_TOP + (SCREEN_Y_BOTTOM - SCREEN_Y_TOP) / 2;
    
    // 映射：最小值->底部（200），最大值->顶部（40）
    y = SCREEN_Y_BOTTOM - (uint16_t)((value - min) * (SCREEN_Y_BOTTOM - SCREEN_Y_TOP) / range);
    
    // 边界限制
    if(y < SCREEN_Y_TOP) y = SCREEN_Y_TOP;
    if(y > SCREEN_Y_BOTTOM) y = SCREEN_Y_BOTTOM;
    
    return y;
}

#define X_VAL    20      // X轴步进值
#define SCREEN_X_MIN    30      // X坐标最小值（左侧）
#define SCREEN_X_MAX    230     // X坐标最大值（右侧）

// 全局变量定义
static u8 current_x = SCREEN_X_MIN;     // 当前绘制X坐标
static u8 last_x = SCREEN_X_MIN;        // 上一个点的X坐标
static u8 last_y = SCREEN_Y_BOTTOM;     // 上一个点的Y坐标
static u8 coord_buffer[200][2];         // 波形数据缓冲区
static u8 buffer_count = 0;             // 缓冲区有效数据数量
//static u8 axis_drawn = 0;               // 坐标轴是否已绘制
static float current_min = 0;           // 当前显示的最小值
static float current_max = 0;           // 当前显示的最大值

extern u8 axis_drawn;
// 显示数值函数
void DisplayNumber(u16 x, u16 y, float value, u16 color, u8 sizey)
{
    char str[20];
    
    // 根据数值大小决定显示格式
    if(value >= 1000)
        sprintf(str, "%d", (uint16_t)value);
    else if(value >= 100)
        sprintf(str, "%d", (uint16_t)value);
    else if(value >= 10)
        sprintf(str, "%d", (uint16_t)value);
    else if(value >= 1)
        sprintf(str, "%d", (uint16_t)value);
    else
        sprintf(str, "%d", (uint16_t)value);
    
    // 使用LCD显示字符串
    LCD_ShowString(x, y, (u8*)str, color, WHITE, sizey, 0);
}

// 绘制坐标轴函数（带数值显示）
void DrawAxis(u16 axis_color, float min, float max)
{
    u8 i;
    u8 tick_count = 0;
    float value;
    u8 y_pos;
    
    // 保存当前范围
    current_min = min;
    current_max = max;
    
    // 绘制X轴（水平线）
    LCD_DrawLine(SCREEN_X_MIN - 5, SCREEN_Y_BOTTOM+5, SCREEN_X_MAX + 5, SCREEN_Y_BOTTOM+5, axis_color);
    
    // 绘制Y轴（垂直线）
    LCD_DrawLine(SCREEN_X_MIN - 5, SCREEN_Y_TOP - 5, SCREEN_X_MIN - 5, SCREEN_Y_BOTTOM + 5, axis_color);
    
    // 绘制箭头（X轴箭头）
    LCD_DrawLine(SCREEN_X_MAX + 5, SCREEN_Y_BOTTOM+5, SCREEN_X_MAX, SCREEN_Y_BOTTOM - 2+5, axis_color);
    LCD_DrawLine(SCREEN_X_MAX + 5, SCREEN_Y_BOTTOM+5, SCREEN_X_MAX, SCREEN_Y_BOTTOM + 2+5, axis_color);
    
    // 绘制箭头（Y轴箭头）
    LCD_DrawLine(SCREEN_X_MIN - 5, SCREEN_Y_TOP - 5, SCREEN_X_MIN - 2, SCREEN_Y_TOP - 2, axis_color);
    LCD_DrawLine(SCREEN_X_MIN - 5, SCREEN_Y_TOP - 5, SCREEN_X_MIN - 8, SCREEN_Y_TOP - 2, axis_color);
    
    // 绘制Y轴刻度及数值（每隔一个刻度显示数值）
    tick_count = 0;
    for(i = SCREEN_Y_TOP; i <= SCREEN_Y_BOTTOM; i += 20)
    {
        // 绘制刻度线
        LCD_DrawLine(SCREEN_X_MIN - 7, i, SCREEN_X_MIN - 3, i, axis_color);
        
        // 每隔一个刻度显示数值（即每40像素显示一次）
        if(tick_count % 2 == 0)
        {
            // 计算当前刻度对应的数值
            value = max - (float)(i - SCREEN_Y_TOP) * (max - min) / (SCREEN_Y_BOTTOM - SCREEN_Y_TOP);
            
            // 显示数值（向左偏移避免覆盖Y轴）
            DisplayNumber(SCREEN_X_MIN - 28, i - 6, value, axis_color, 12);
        }
        tick_count++;
    }
    
    // 绘制X轴刻度
    tick_count = 0;
    for(i = SCREEN_X_MIN; i <= SCREEN_X_MAX; i += 20)
    {
        LCD_DrawLine(i, SCREEN_Y_BOTTOM - 2+5, i, SCREEN_Y_BOTTOM + 2+5, axis_color);
        tick_count++;
    }
    
    // 显示Y轴标签
    LCD_ShowString(SCREEN_X_MIN - 28, SCREEN_Y_TOP - 12, (u8*)"Max", axis_color, WHITE, 12, 0);
    LCD_ShowString(SCREEN_X_MIN - 28, SCREEN_Y_BOTTOM - 8, (u8*)"Min", axis_color, WHITE, 12, 0);
    
    // 显示X轴标签
    LCD_ShowString(96, SCREEN_Y_BOTTOM + 12+5, (u8*)"Time", axis_color, WHITE, 12, 0);
}

// 更新Y轴数值（当min/max变化时）
void UpdateAxisValues(float min, float max, u16 axis_color)
{
    u8 i;
    u8 tick_count = 0;
    float value;
    
    // 清除旧的数值显示区域
    LCD_Fill(SCREEN_X_MIN - 45, SCREEN_Y_TOP - 10, SCREEN_X_MIN - 5, SCREEN_Y_BOTTOM + 10, WHITE);
    
    // 重新显示数值
    for(i = SCREEN_Y_TOP; i <= SCREEN_Y_BOTTOM; i += 20)
    {
        if(tick_count % 2 == 0)
        {
            value = max - (float)(i - SCREEN_Y_TOP) * (max - min) / (SCREEN_Y_BOTTOM - SCREEN_Y_TOP);
            DisplayNumber(SCREEN_X_MIN - 28, i - 6, value, axis_color, 12);
        }
        tick_count++;
    }
    
    current_min = min;
    current_max = max;
}

// 绘制数据点函数（使用圆形绘制）
void DrawPoint(u8 x, u8 y, u8 radius, u16 color)
{
    if(radius <= 0)
    {
        LCD_DrawPoint(x, y, color);
    }
    else
    {
        Draw_Circle(x, y, radius, color);
    }
}

/**
 * @brief 显示波形
 * @param data      当前数据值
 * @param min       数据最小值
 * @param max       数据最大值
 * @param axis_color    坐标轴颜色
 * @param wave_color    波形颜色
 * @param point_color   数据点颜色
 * @param point_radius  数据点半径
 */
void lcd_show_wave(float data, float min, float max, 
                   u16 axis_color, u16 wave_color, u16 point_color, u8 point_radius)
{
    u8 i;
    u8 current_y;
    
    // 第一次调用时绘制坐标轴
    if(!axis_drawn)
    {
        DrawAxis(axis_color, min, max);
        axis_drawn = 1;
    }
    else
    {
        // 如果min/max发生变化，更新Y轴数值
        if(current_min != min || current_max != max)
        {
            UpdateAxisValues(min, max, axis_color);
        }
    }
    
	
	DrawAxis(axis_color, min, max);
	
    // 数据转换为Y坐标
    current_y = DataToY(data, min, max);
    
    // 边界限制
    if(current_y < SCREEN_Y_TOP) current_y = SCREEN_Y_TOP;
    if(current_y > SCREEN_Y_BOTTOM) current_y = SCREEN_Y_BOTTOM;
    
    // 判断是否需要滚动
    if(current_x >= SCREEN_X_MAX)
    {
        // ========== 滚动显示逻辑 ==========
        // 1. 用背景色（黑色）清除当前显示的所有波形和点
        for(i = 0; i < buffer_count - 1; i++)
        {
            LCD_DrawLine(coord_buffer[i][0], coord_buffer[i][1], 
                        coord_buffer[i+1][0], coord_buffer[i+1][1], WHITE);
        }
        
        // 清除所有数据点
        for(i = 0; i < buffer_count; i++)
        {
            DrawPoint(coord_buffer[i][0], coord_buffer[i][1], point_radius, WHITE);
        }
        
        // 2. 整体左移一个步进
        for(i = 0; i < buffer_count - 1; i++)
        {
            coord_buffer[i][0] = coord_buffer[i+1][0] - X_VAL;
            coord_buffer[i][1] = coord_buffer[i+1][1];
        }
        
        // 3. 更新缓冲区数量
        buffer_count--;
        current_x = SCREEN_X_MAX - X_VAL;
        
        // 4. 用波形颜色重新绘制移动后的波形
        for(i = 0; i < buffer_count - 1; i++)
        {
            LCD_DrawLine(coord_buffer[i][0], coord_buffer[i][1], 
                        coord_buffer[i+1][0], coord_buffer[i+1][1], wave_color);
        }
        
        // 5. 重新绘制移动后的数据点
        for(i = 0; i < buffer_count; i++)
        {
            DrawPoint(coord_buffer[i][0], coord_buffer[i][1], point_radius, point_color);
        }
        
        // 6. 更新上一个点坐标
        if(buffer_count > 0)
        {
            last_x = coord_buffer[buffer_count-1][0];
            last_y = coord_buffer[buffer_count-1][1];
        }
    }
    
    // ========== 绘制新点 ==========
    // 如果不是第一个点，绘制连线
    if(buffer_count > 0)
    {
        LCD_DrawLine(last_x, last_y, current_x, current_y, wave_color);
    }
    
    // 保存当前点到缓冲区
    coord_buffer[buffer_count][0] = current_x;
    coord_buffer[buffer_count][1] = current_y;
    buffer_count++;
    
    // 绘制数据点
    DrawPoint(current_x, current_y, point_radius, point_color);
    
    // 更新上一个点坐标
    last_x = current_x;
    last_y = current_y;
    
    // X坐标递增
    current_x += X_VAL;
}

// 可选：重置波形显示（清空所有数据）
void lcd_wave_reset(void)
{
    u8 i;
    
    // 清除所有波形和点
    for(i = 0; i < buffer_count - 1; i++)
    {
        LCD_DrawLine(coord_buffer[i][0], coord_buffer[i][1], 
                    coord_buffer[i+1][0], coord_buffer[i+1][1], WHITE);
    }
    
    for(i = 0; i < buffer_count; i++)
    {
        DrawPoint(coord_buffer[i][0], coord_buffer[i][1], 1, WHITE);
    }
    
    // 重置全局变量
    current_x = SCREEN_X_MIN;
    last_x = SCREEN_X_MIN;
    last_y = SCREEN_Y_BOTTOM;
    buffer_count = 0;
    axis_drawn = 0;
}

// 可选：清除坐标轴
void lcd_axis_clear(void)
{
    LCD_Fill(SCREEN_X_MIN - 50, SCREEN_Y_TOP - 20, SCREEN_X_MAX + 20, SCREEN_Y_BOTTOM + 20, WHITE);
    axis_drawn = 0;
}