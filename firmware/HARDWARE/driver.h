#include "sys.h"


void Hardware_Init(void);/* 外设初始化 */
void Get_Data(void);/* 采集传感器数据 */
void Hardware_Hander(void);/* 主循环业务：报警、蓝牙上报等 */
void OLED_Show(void);/* 界面刷新入口 */

void Watch_Show(void);/* 表盘界面 */
void Data_Show(void);/* 综合数据显示页 */
void Set_Mode_Show(void);/* 功能菜单 */
void Stop_Watch(void);/* 秒表 */
void Set_Clock_Show(void);/* 校时 */
void Set_Alarm_Show(void);/* 闹钟设置 */
void Set_Warn_Show(void);/* 报警开关 */
void Set_Threshold_Show(void);/* 阈值设置界面 */
float mean_remove_extremes_select(float *arr, int n, int a, int b);/* 去极值后求均值 */
void limit_value(int16_t *data,int16_t min,int16_t max,u8 mode);
/* 蓝牙/App 设置阈值：心率下限、心率上限、血氧下限、体温下限、体温上限 */
void Driver_Set_Thresholds(int16_t t0, int16_t t1, int16_t t2, int16_t t3, int16_t t4);
void Driver_Set_Threshold(int8_t index, int16_t val);

void LCD_Show_XL(void);
void LCD_Show_XY(void);
void LCD_Show_Temp(void);

void show_ad8232(void);

void Test(void);

void show_ad8232(void);

