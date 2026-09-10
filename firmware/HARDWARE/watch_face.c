#include "watch_face.h"
#include "ds1307.h"
#include "math.h"
#include "string.h"
#include "stdio.h"
#include "lcd.h"

// 上次指针位置记录
static u8 last_hour = 0, last_minute = 0, last_second = 0;
static u8 last_date = 0, last_month = 0;
static u16 last_year = 0;

// 记录上一帧的指针终点坐标
static s16 last_hour_x = 0, last_hour_y = 0;
static s16 last_min_x = 0, last_min_y = 0;
static s16 last_sec_x = 0, last_sec_y = 0;

Watch_TimeTypeDef watch_time;

#define PI 3.14159

/**
 * @brief 根据角度获取圆上点坐标
 */
static void GetPointOnCircle(u16 center_x, u16 center_y, u16 radius, float angle, s16 *x, s16 *y)
{
	float rad = angle * PI / 180.0f;
	*x = center_x + (s16)(radius * cos(rad));
	*y = center_y - (s16)(radius * sin(rad));
}

/**
 * @brief 清除指针 - 修正版
 */
void Watch_ClearHands(void)
{
	s16 x, y;
	u8 i;
	float sec_angle;
	float min_angle;
	float hour_angle;
	// ===== 清除秒针 - 无论是否为0都要清除 =====
	// 不能判断 last_second != 0，因为0点方向也需要清除
	// 只需要判断是否有上一次的记录（初始化时last_second=0，第一次不执行）
	// 但第一次之后，即使指向0也要清除

	// 更好的方法是：只要不是第一次运行（用一个标志判断），就执行清除

	// 方法1：去掉判断，每次都清除（但第一次会有无效清除）
	sec_angle = 90.0f - (last_second * 6.0f);
	GetPointOnCircle(CENTER_X, CENTER_Y, SECOND_HAND_LEN, sec_angle, &x, &y);
	for (i = 0; i < 3; i++)
	{
		LCD_DrawLine(CENTER_X - 1 + i, CENTER_Y, x - 1 + i, y, DIAL_COLOR);
	}

	// 清除分针 - 同样的问题，应该去掉 !=0 判断
	min_angle = 90.0f - (last_minute * 6.0f + last_second * 0.1f);
	GetPointOnCircle(CENTER_X, CENTER_Y, MINUTE_HAND_LEN, min_angle, &x, &y);
	for (i = 0; i < 3; i++)
	{
		LCD_DrawLine(CENTER_X - 1 + i, CENTER_Y, x - 1 + i, y, DIAL_COLOR);
	}

	// 清除时针
	hour_angle = 90.0f - ((last_hour % 12) * 30.0f + last_minute * 0.5f);
	GetPointOnCircle(CENTER_X, CENTER_Y, HOUR_HAND_LEN, hour_angle, &x, &y);
	for (i = 0; i < 4; i++)
	{
		LCD_DrawLine(CENTER_X - 2 + i, CENTER_Y, x - 2 + i, y, DIAL_COLOR);
	}

	// 彻底清除中心点区域
	Draw_Circle(CENTER_X, CENTER_Y, 12, DIAL_COLOR);
}

/**
 * @brief 绘制指针
 */
void Watch_DrawHands(u8 hour, u8 minute, u8 second)
{
	s16 x, y;
	u8 i;
	float hour_angle;
	float min_angle;
	float sec_angle;
	//
	//    // 先清除旧的
	//    Watch_ClearHands();

	// 保存新位置
	last_hour = hour;
	last_minute = minute;
	last_second = second;

	hour_angle = 90.0f - ((hour % 12) * 30.0f + minute * 0.5f);
	min_angle = 90.0f - (minute * 6.0f + second * 0.1f);
	sec_angle = 90.0f - (second * 6.0f);

	// 绘制秒针（最上层）
	GetPointOnCircle(CENTER_X, CENTER_Y, SECOND_HAND_LEN, sec_angle, &x, &y);
	LCD_DrawLine(CENTER_X, CENTER_Y, x, y, SEC_HAND_COLOR);

	// 绘制分针
	GetPointOnCircle(CENTER_X, CENTER_Y, MINUTE_HAND_LEN, min_angle, &x, &y);
	for (i = 0; i < 3; i++)
	{
		LCD_DrawLine(CENTER_X - 1 + i, CENTER_Y, x - 1 + i, y, MIN_HAND_COLOR);
	}

	// 绘制时针
	GetPointOnCircle(CENTER_X, CENTER_Y, HOUR_HAND_LEN, hour_angle, &x, &y);
	for (i = 0; i < 4; i++)
	{
		LCD_DrawLine(CENTER_X - 2 + i, CENTER_Y, x - 2 + i, y, HOUR_HAND_COLOR);
	}

	// 最后重绘中心点（确保在最上层）
	Draw_Circle(CENTER_X, CENTER_Y, 6, CENTER_DOT_COLOR);
	Draw_Circle(CENTER_X, CENTER_Y, 3, WHITE);

	// 再画一个小点增强中心
	LCD_DrawPoint(CENTER_X, CENTER_Y, CENTER_DOT_COLOR);
}

/**
 * @brief 绘制表盘
 */
void Watch_DrawDial(void)
{
	s16 x1, y1, x2, y2;
	u8 i;
	char num_str[3];
	float angle;
	s16 num_x, num_y;
	u8 hour_num;

	LCD_Fill(0, 0, 239, 239, DIAL_COLOR);

	// 绘制外圆
	Draw_Circle(CENTER_X, CENTER_Y, RADIUS, SCALE_COLOR);
	Draw_Circle(CENTER_X, CENTER_Y, RADIUS - 1, LGRAY);

	// 绘制刻度
	for (i = 0; i < 60; i++)
	{
		angle = 90.0f - i * 6.0f;

		if (i % 5 == 0)
		{
			// 整点长刻度
			GetPointOnCircle(CENTER_X, CENTER_Y, RADIUS, angle, &x1, &y1);
			GetPointOnCircle(CENTER_X, CENTER_Y, RADIUS - 15, angle, &x2, &y2);
			LCD_DrawLine(x1, y1, x2, y2, SCALE_COLOR);

			// 数字 - 调整位置避免遮挡
			hour_num = i / 5;
			if (hour_num == 0)
				hour_num = 12;

			GetPointOnCircle(CENTER_X, CENTER_Y, RADIUS - 25, angle, &num_x, &num_y);

			sprintf(num_str, "%d", hour_num);
			if (hour_num >= 10)
				LCD_ShowString(num_x - 8, num_y - 8, (u8 *)num_str, SCALE_COLOR, DIAL_COLOR, 16, 0);
			else
				LCD_ShowString(num_x - 4, num_y - 8, (u8 *)num_str, SCALE_COLOR, DIAL_COLOR, 16, 0);
		}
		else
		{
			// 分钟短刻度
			GetPointOnCircle(CENTER_X, CENTER_Y, RADIUS, angle, &x1, &y1);
			GetPointOnCircle(CENTER_X, CENTER_Y, RADIUS - 8, angle, &x2, &y2);
			LCD_DrawLine(x1, y1, x2, y2, LGRAY);
		}
	}

	// 中心点
	Draw_Circle(CENTER_X, CENTER_Y, 6, CENTER_DOT_COLOR);
	Draw_Circle(CENTER_X, CENTER_Y, 3, WHITE);

	// 日期背景 - 移到更上方避免遮挡刻度
	//    LCD_DrawRectangle(50, 10, 190, 30, DATE_BG_COLOR);
	//    LCD_Fill(51, 11, 189, 29, DATE_BG_COLOR);
}

/**
 * @brief 显示日期
 */
void Watch_ShowDate(u16 year, u8 month, u8 date, u8 week)
{
	char date_str[20];
	char week_str[10];

	//    if(last_date == date && last_month == month && last_year == year)
	//        return;

	last_date = date;
	last_month = month;
	last_year = year;

	// 清除日期显示区域
	//    LCD_Fill(88, 80, 189, 29, DATE_BG_COLOR);

	// 格式化日期 - 缩短格式避免过长
	sprintf(date_str, "%04d-%02d-%02d", year, month, date);
	LCD_ShowString(80, 75, (u8 *)date_str, BLACK, DATE_BG_COLOR, 16, 0);

	// 显示星期缩写
	switch (week)
	{
	case 1:
		LCD_ShowChinese(96, 91, "星期一", BLACK, DATE_BG_COLOR, 16, 0);
		break;
	case 2:
		LCD_ShowChinese(96, 91, "星期二", BLACK, DATE_BG_COLOR, 16, 0);
		break;
	case 3:
		LCD_ShowChinese(96, 91, "星期三", BLACK, DATE_BG_COLOR, 16, 0);
		break;
	case 4:
		LCD_ShowChinese(96, 91, "星期四", BLACK, DATE_BG_COLOR, 16, 0);
		break;
	case 5:
		LCD_ShowChinese(96, 91, "星期五", BLACK, DATE_BG_COLOR, 16, 0);
		break;
	case 6:
		LCD_ShowChinese(96, 91, "星期六", BLACK, DATE_BG_COLOR, 16, 0);
		break;
	case 7:
		LCD_ShowChinese(96, 91, "星期日", BLACK, DATE_BG_COLOR, 16, 0);
		break;
	default:
		LCD_ShowChinese(96, 91, "星期一", BLACK, DATE_BG_COLOR, 16, 0);
		break;
	}
	//    LCD_ShowString(112, 91, (u8*)week_str, BLACK, DATE_BG_COLOR, 16, 0);
}

/**
 * @brief 初始化
 */
void Watch_Init(void)
{
	last_hour = 0;
	last_minute = 0;
	last_second = 0;
	last_date = 0;
	last_month = 0;
	last_year = 0;

	last_hour_x = 0;
	last_hour_y = 0;
	last_min_x = 0;
	last_min_y = 0;
	last_sec_x = 0;
	last_sec_y = 0;

	Watch_DrawDial();
}

/**
 * @brief 更新时间
 */
void Watch_UpdateTime(Watch_TimeTypeDef *time)
{
	static u8 last_min_check = 0;

	if (time == NULL)
		return;

	// 先清除旧的
	Watch_ClearHands();
	Watch_ShowDate(time->year, time->month, time->date, time->week);
	Watch_DrawHands(time->hour, time->minute, time->second);

	//    if(time->second != last_min_check)
	//    {
	//        last_min_check = time->second;
	//        Watch_ShowDate(time->year, time->month, time->date, time->week);
	//    }
}

/**
 * @brief 强制重绘整个表盘
 */
void Watch_RedrawFull(void)
{
	Watch_DrawDial();
	Watch_UpdateTime(&watch_time);
}