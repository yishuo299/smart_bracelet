#ifndef __WATCH_FACE_H
#define __WATCH_FACE_H

#include "sys.h"

// 表盘中心点坐标
#define CENTER_X        120
#define CENTER_Y        120
#define RADIUS          110     // 表盘半径
#define INNER_RADIUS    95      // 内圈半径
#define SECOND_HAND_LEN   75  // 秒针长度
#define MINUTE_HAND_LEN   57  // 分针长度
#define HOUR_HAND_LEN     45  // 时针长度

// 颜色定义
#define DIAL_COLOR      WHITE           // 表盘背景
#define SCALE_COLOR     BLACK           // 刻度颜色
#define HOUR_HAND_COLOR BLACK           // 时针颜色
#define MIN_HAND_COLOR  DARKBLUE        // 分针颜色
#define SEC_HAND_COLOR  RED             // 秒针颜色
#define CENTER_DOT_COLOR RED            // 中心点颜色
#define DATE_BG_COLOR   LGRAY       // 日期背景色

// 时间结构体
typedef struct {
    u16 year;
    u8 month;
    u8 date;
    u8 week;
    u8 hour;
    u8 minute;
    u8 second;
} Watch_TimeTypeDef;

extern Watch_TimeTypeDef watch_time;

// 函数声明
void Watch_DrawDial(void);
void Watch_UpdateTime(Watch_TimeTypeDef *time);
void Watch_DrawHands(u8 hour, u8 minute, u8 second);
void Watch_ClearHands(void);
void Watch_ShowDate(u16 year, u8 month, u8 date, u8 week);
void Watch_Init(void);
void Watch_RedrawFull(void);

#endif