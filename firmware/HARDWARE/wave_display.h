#ifndef __WAVE_SIMPLE_H
#define __WAVE_SIMPLE_H

#include "sys.h"
#include "lcd.h"

void lcd_show_wave(float data, float min, float max, u16 axis_color, u16 wave_color, u16 point_color, u8 point_radius);
void lcd_wave_reset(void);
void lcd_axis_clear(void);
#endif