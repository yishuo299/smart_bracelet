#include "driver.h"
#include <stdlib.h>
#include <string.h>
#include "math.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "beep.h"
#include "lcd.h"
#include "time3.h"
#include "ds18b20.h"
#include "max30102.h"
#include "ds1307.h"
#include "bluetooth.h" 
#include "asrpro.h"
#include "watch_face.h"
#include "ad8232.h"
#include "ecg_display.h"
#include "wave_display.h"

/* 默认阈值：心率下限、心率上限、血氧下限、体温下限、体温上限 */
int16_t threshold[5]={ 	40,		140,		80,		20,		38	};

/* 由蓝牙/App 下发 TH: 指令批量设置，顺序同 threshold[] */
void Driver_Set_Thresholds(int16_t t0, int16_t t1, int16_t t2, int16_t t3, int16_t t4)
{
	threshold[0] = t0;
	threshold[1] = t1;
	threshold[2] = t2;
	threshold[3] = t3;
	threshold[4] = t4;
}

void Driver_Set_Threshold(int8_t index, int16_t val)
{
	if (index >= 0 && index < 5)
		threshold[index] = val;
}

u16 alarm_str[3]={700,1200,2200};/* 三个闹钟时刻 HHMM，如 700 表示 7:00 */
u8 alarm_flag[3]={1,0,0};/* 各闹钟是否使能（奇数为开） */

float temperature;/* 当前体温 */
int8_t warn_mode[4]={0,0,0,0};/* 各监测项报警开关（奇数为开） */
u8 oled_flag=0;/* 当前界面索引 */

u16 warn_count=0;/* 报警闪烁计数 */
int16_t send_freq=0;/* 蓝牙发送周期计数 */
u8 stop_watch_count=0;/* 秒表 10ms 计数 */
u8 mpu_count=0;/* 语音/MPU 相关计数 */
u16 ring_count=0;/* 闹钟响铃时计数 */
u8 clock_start=0;/* 闹钟流程状态：0 无 /1 响铃中 /2 结束 */
u16 AD8232_val[1024]={0};
u16 ad8232_count=0;
 u8 axis_drawn = 0;               /* 波形坐标轴是否已绘制 */

/* 外设初始化 */
void Hardware_Init(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); /* NVIC 分组 2：2 位抢占 + 2 位子优先级 */
	delay_init();
	LCD_Init();
	LCD_Fill(0,0,LCD_W,LCD_H,BLACK);
	LCD_ShowChinese(24,108,"医疗健康监测设备",WHITE,BLACK,24,0);
	LED_Init();
	KEY_Init();
	BEEP_Init();
	TIM3_Int_Init(4999,7199);/* TIM3：周期任务等 */
	TIM4_Int_Init(99,7199);/* TIM4：秒表节拍，约 0.01s */
	DS18B20_Init();
	MAX30102_Init();
	DS1307_Init();
//	DS1307_SetTime(2026,3,15,7,0,35,0);/* 上电写 RTC 时可取消注释 */
	Bluetooth_Init(9600);
	ASRPRO_Init();
	AD8232_Init();
	
//	LCD_Fill(0,0,LCD_W,LCD_H,BLACK);
	LCD_Fill(0,0,LCD_W,LCD_H,WHITE);
}





/* ==================== 滑动平均滤波（预留） ==================== */
#define FILTER_SIZE         4           /* 窗口长度 */
#define ECG_DISPLAY_GAIN    100          /* ECG 显示增益系数（预留） */

/* ==================== 滤波静态缓冲 ==================== */
static uint16_t filter_buffer[FILTER_SIZE];
static uint8_t filter_index = 0;
static uint32_t filter_sum = 0;
static uint8_t filter_count = 0;

uint16_t MovingAverageFilter(uint16_t new_sample)
{
    filter_sum -= filter_buffer[filter_index];
    filter_buffer[filter_index] = new_sample;
    filter_sum += new_sample;
    
    filter_index++;
    if(filter_index >= FILTER_SIZE)
        filter_index = 0;
    
    if(filter_count < FILTER_SIZE)
        filter_count++;
    
    return filter_sum / filter_count;
}


void Test(void)
{
        char str[16];

	temperature=Get_Temp_Val();
        
		sprintf(str," %.1f  ", temperature);
		LCD_ShowString(100,30,(uint8_t *)str,BLACK,WHITE,24,0);
        
        /* 温度波形演示 */
//        Wave_Add(&wave, temperature*10);
        lcd_show_wave(temperature*10,280,370,BLACK, GREEN, GREEN,2);
        delay_ms(500);
}












/* 采集各传感器数据 */
void Get_Data(void)
{
	temperature=Get_Temp_Val();
	MAX30102_GetXlXy(&maxXlXy.xl,&maxXlXy.xy);/* 心率、血氧 */
	DS1307_GetTime(&watch_time.year, &watch_time.month, &watch_time.date, 
				  &watch_time.week, &watch_time.hour, &watch_time.minute, &watch_time.second);
	AD8232_val[ad8232_count] = AD8232_ReadADC();
	if(AD8232_val[ad8232_count]>=4000){AD8232_val[ad8232_count]=4000;}
	if(AD8232_val[ad8232_count]<=500){AD8232_val[ad8232_count]=500;}
	ad8232_count++;
}








/* 主循环业务：报警、蓝牙上报、闹钟、语音模块 */
void Hardware_Hander(void)
{
				   /* 语音模块协议帧头 */
	u8 asrpro_str[5]={0x55,0x02,0x01,0x01,0xBB};
	static u16 time_count=0;
	static u8 warn_flag[4]={0};
	u8 i=0;
	static float last_mpu_val[3]={0};
	char send_str[1024];
	static u8 clock_off_flag=0;/* 用户按键关闹钟标志 */
	u8 key_val=0;
	u8 asrpro_state=0;
	char show_str[1024];
    uint16_t len;
	
	asrpro_state=ASRPRO_RX();
	/********************* 超限检测 *********************/
	if(warn_mode[0]%2)/* 心率报警使能 */
	{
		if(maxXlXy.xl<threshold[0]||maxXlXy.xl>threshold[1]){warn_flag[0]=1;}/* 超阈置 1 */
		else{warn_flag[0]=0;}
	}else{warn_flag[0]=0;}/* 关闭则不判 */
	if(warn_mode[1]%2)/* 血氧报警使能 */
	{
		if(maxXlXy.xy<threshold[2]){warn_flag[1]=1;}/* 低于下限 */
		else{warn_flag[1]=0;}
	}else{warn_flag[1]=0;}
	if(warn_mode[2]%2)/* 体温报警使能 */
	{
		if(temperature<threshold[3]||temperature>threshold[4]){warn_flag[2]=1;}
		else{warn_flag[2]=0;}
	}else{warn_flag[2]=0;}
	
	if(warn_flag[0]==1||warn_flag[1]==1||warn_flag[2]==1||warn_flag[3]==1)/* 任一报警 */
	{
		if(warn_count%2==0)/* 闪烁：蜂鸣/LED 与蓝牙 Danger */
		{	/* 亮 LED、发蓝牙 */
			Bluetooth_Send_Data("\r\nDanger!!!\r\n");
			LED_Mode(LED_ON);
			BEEP=1;
		}else{/* 灭 LED */
			LED_Mode(LED_OFF);
			BEEP=0;
		}
		if(mpu_count>=30){mpu_count=0;warn_flag[3]=0;}
		if(warn_flag[3]==1&&mpu_count%10==0){asrpro_state=2;}
	}else {/* 无报警：关声光 */
		LED_Mode(LED_OFF);
		BEEP=0;
		warn_count=0;
		mpu_count=0;
	}
	/********************* 蓝牙定时上报 ***********************/
	if(send_freq >= 1)  /* 约 1s 一次 */
	{
		
		/* 体温 */
		len = sprintf(send_str, "\r\nTemperature:  %.1f C\r\n", temperature);
		
		/* 心率 */
		if(maxXlXy.xl > 0 && maxXlXy.xl < 150)
			len += sprintf(send_str + len, "Heart Rate:  %d bpm\r\n", maxXlXy.xl);
		else
			len += sprintf(send_str + len, "Heart Rate:  ---\r\n");
		
		/* 血氧 */
		if(maxXlXy.xy > 30 && maxXlXy.xy < 110)
			len += sprintf(send_str + len, "Blood Oxygen: %d %%\r\n", maxXlXy.xy);
		else
			len += sprintf(send_str + len, "Blood Oxygen: ---\r\n");
		
		/* AD8232 采样缓冲 */
		len += sprintf(send_str + len, "AD8232:");
		for(i = 0; i < ad8232_count; i++)
		{
			len += sprintf(send_str + len, " %d", AD8232_val[i]);
			if(len >= sizeof(send_str) - 20) break;
		}
		len += sprintf(send_str + len, "\r\n");
		
		/* 清空已上报的 ECG 缓冲 */
		memset(AD8232_val, 0, ad8232_count * sizeof(uint16_t));
		ad8232_count = 0;
		
		/* 发送 */
		Bluetooth_Send_Data(send_str);
		send_freq = 0;
	}
	/********************* 闹钟逻辑 ***********************/
	for(i=0;i<3;i++)/* 最多三个闹钟 */
	{
		if(alarm_flag[i]%2)/* 该闹钟开启 */
		{
			if((watch_time.hour*100+watch_time.minute)==alarm_str[i]&&clock_off_flag==0)/* 到点且未手动关 */
			{
				clock_start=1;/* 进入响铃 */
			}
			else if((watch_time.hour*100+watch_time.minute)!=alarm_str[i]&&clock_start==1)/* 已过设定时刻 */
			{
				clock_start=2;/* 结束响铃流程 */
				clock_off_flag=0;
			}
			else if((watch_time.hour*100+watch_time.minute)!=alarm_str[i]&&clock_off_flag==1)/* 已按键跳过本分钟 */
			{
				clock_off_flag=0;
			}
		}
	}
	if(clock_start==1)/* 响铃中 */
	{
		key_val=KEY_Scan();
		if(key_val!=0)/* 任意键停止 */
		{
			clock_off_flag=1;
			clock_start=2;
		}
		if(ring_count%20==0)/* 周期性发语音播放命令 */
		{
			asrpro_str[1]=0x02;
			asrpro_str[2]=0x01;
			ASRPRO_Send_Data(asrpro_str);
		}
	}else if(clock_start==2)/* 发送停止播放 */
	{
		asrpro_str[1]=0x03;
		ASRPRO_Send_Data(asrpro_str);
		clock_start=0;/* 恢复空闲 */
	}
	
	/********************* 语音识别应答 ***********************/
	if(asrpro_state!=0)/* 非 0 时处理语音结果 */
	{
		if(asrpro_state==1)
		{
			asrpro_str[1]=0x01;
			asrpro_str[2]=watch_time.hour;
			asrpro_str[3]=watch_time.minute;
			ASRPRO_Send_Data(asrpro_str);
			asrpro_state=0;
		}else if(asrpro_state==2)
		{
			asrpro_str[1]=0x02;
			asrpro_str[2]=0x02;
			ASRPRO_Send_Data(asrpro_str);
			asrpro_state=0;
		}
	}
}


/* 根据 oled_flag 切换各界面 */
void OLED_Show(void)
{
	if(clock_start==0)
	{
		if(oled_flag==0)/* 表盘 */
		{
			Watch_Show();
		}
		else if(oled_flag==1)/* 心率 */
		{
			LCD_Show_XL();
		}
		else if(oled_flag==2)/* 血氧 */
		{
			LCD_Show_XY();
		}
		else if(oled_flag==3)/* 体温 */
		{
			LCD_Show_Temp();
		}
		else if(oled_flag==4)/* 心电 */
		{
			show_ad8232();
		}
		else if(oled_flag==5)/* 功能菜单 */
		{
			Set_Mode_Show();
		}
		else if(oled_flag==6)/* 秒表 */
		{
			Stop_Watch();
		}
		else if(oled_flag==7)/* 校时 */
		{
			Set_Clock_Show();
		}
		else if(oled_flag==8)/* 闹钟 */
		{
			Set_Alarm_Show();
		}
		else if(oled_flag==9)/* 报警开关 */
		{
			Set_Warn_Show();
		}
		else if(oled_flag==10)/* 阈值设置 */
		{
			Set_Threshold_Show();
		}
	}
}

/* 表盘：时间刷新与切页 */
void Watch_Show(void)
{
    static u8 sec;
	u8 key_val=0;
	static u8 show_flag=0;
	if(show_flag==0)/* 首次进入：画表盘 */
	{
		Watch_Init();
		show_flag=1;
	}
	key_val=KEY_Scan();
	if(key_val==1)
	{
		LCD_Fill(0,0,LCD_W,LCD_H,WHITE);
		oled_flag++;
		show_flag=0;
	}
	/* 秒变化时刷新时间 */
	if(sec!=watch_time.second)
	{
		Watch_UpdateTime(&watch_time);
		sec=watch_time.second;
	}
}



void LCD_Show_XL(void)
{
	u8 key_val=0;
	static u8 show_flag=0;
	static u16 time_count=0;
	char show_str[50];
	
	if(show_flag==0)/* 首次进入：清屏与波形复位 */
	{
		LCD_Fill(0,0,LCD_W,LCD_H,WHITE);
		lcd_wave_reset();
		lcd_axis_clear();
		axis_drawn=0;
		show_flag=1;
	}
	if(time_count%200==0)
	{
		LCD_ShowChinese(96,6,(uint8_t *)"心率",RED,WHITE,24,0);
		if(maxXlXy.xl>0&&maxXlXy.xl<150){
			sprintf(show_str,"  %d bmp  ", maxXlXy.xl);
			LCD_ShowString((240-strlen(show_str)*12)/2,30,(uint8_t *)show_str,BLACK,WHITE,24,0);
		}else
		{
			maxXlXy.xl=0;
			sprintf(show_str,"   ---   ");
			LCD_ShowString((240-strlen(show_str)*12)/2,30,(uint8_t *)show_str,BLACK,WHITE,24,0);
		}
		lcd_show_wave(maxXlXy.xl,0,150,BLACK, RED, RED,2);
	}
	key_val=KEY_Scan();
	if(key_val==1)
	{
		oled_flag++;
		show_flag=0;
		time_count=0;
	}
	delay_ms(1);
	time_count++;
}

void LCD_Show_XY(void)
{
	u8 key_val=0;
	static u8 show_flag=0;
	static u16 time_count=0;
	char show_str[50];
	
	if(show_flag==0)/* 首次进入：清屏与波形复位 */
	{
		LCD_Fill(0,0,LCD_W,LCD_H,WHITE);
		lcd_wave_reset();
		lcd_axis_clear();
		axis_drawn=0;
		show_flag=1;
	}
	if(time_count%200==0)
	{
		LCD_ShowChinese(96,6,(uint8_t *)"血氧",RED,WHITE,24,0);
		if(maxXlXy.xy>30&&maxXlXy.xy<=100){
			sprintf(show_str,"  %d %%  ", maxXlXy.xy);
			LCD_ShowString((240-strlen(show_str)*12)/2,30,(uint8_t *)show_str,BLACK,WHITE,24,0);
		}else
		{
			maxXlXy.xy=30;
			sprintf(show_str,"   ---   ");
			LCD_ShowString((240-strlen(show_str)*12)/2,30,(uint8_t *)show_str,BLACK,WHITE,24,0);
		}
		lcd_show_wave(maxXlXy.xy,30,100,BLACK, GREEN, GREEN,2);
	}
	key_val=KEY_Scan();
	if(key_val==1)
	{
		oled_flag++;
		show_flag=0;
		time_count=0;
	}
	delay_ms(1);
	time_count++;
}



void LCD_Show_Temp(void)
{
	u8 key_val=0;
	static u8 show_flag=0;
	static u16 time_count=0;
	char show_str[50];
	
	if(show_flag==0)/* 首次进入 */
	{
		LCD_Fill(0,0,LCD_W,LCD_H,WHITE);
		lcd_wave_reset();
		lcd_axis_clear();
		axis_drawn=0;
		show_flag=1;
	}
	if(time_count%200==0)
	{
		LCD_ShowChinese(96,6,(uint8_t *)"体温",RED,WHITE,24,0);
		sprintf(show_str,"  %.1f C  ", temperature);
		LCD_ShowString((240-strlen(show_str)*12)/2,30,(uint8_t *)show_str,BLACK,WHITE,24,0);
        lcd_show_wave(temperature,28,37,BLACK, BLUE, BLUE,2);
	}
	key_val=KEY_Scan();
	if(key_val==1)
	{
		oled_flag++;
		show_flag=0;
		time_count=0;
	}
	delay_ms(1);
	time_count++;
}
const uint16_t ecg_simple[] = {
    /* 基线 */
    2048, 2048, 2048, 2048, 2048,
    
    /* P 波 */
    2050, 2060, 2070, 2080, 2090, 2100, 2110, 2120, 2130, 2140,
    2150, 2150, 2140, 2130, 2120, 2110, 2100, 2090, 2080, 2070,
    
    /* PR 段 */
    2060, 2050, 2048, 2048, 2048,
    
    /* Q 波下降肢 */
    2040, 2030, 2020, 2010, 2000, 1990, 1980, 1970, 1960, 1950,
    
    /* R 波上升肢 */
    2000, 2100, 2200, 2300, 2400, 2500, 2600, 2700, 2800, 2900,
    3000, 3100, 3200, 3300, 3400, 3500, 3600, 3700, 3800, 3900,
    
    /* R 波峰 */
    4000, 4050, 4080, 4095, 4090, 4080, 4070, 4060, 4050, 4040,
    
    /* S 波下降肢 */
    4000, 3900, 3800, 3700, 3600, 3500, 3400, 3300, 3200, 3100,
    3000, 2900, 2800, 2700, 2600, 2500, 2400, 2300, 2200, 2100,
    2000, 1900, 1800, 1700, 1600, 1500, 1400, 1300, 1200, 1100,
    
    /* ST 段 */
    1150, 1200, 1250, 1300, 1350, 1400, 1450, 1500, 1550, 1600,
    
    /* T 波上升 */
    1650, 1700, 1750, 1800, 1850, 1900, 1950, 2000, 2050, 2100,
    2150, 2200, 2250, 2300, 2350, 2400, 2450, 2500, 2550, 2600,
    
    /* T 波下降 */
    2550, 2500, 2450, 2400, 2350, 2300, 2250, 2200, 2150, 2100,
    2050, 2000, 1950, 1900, 1850, 1800, 1750, 1700, 1650, 1600,
    
    /* 回到基线 */
    1700, 1800, 1900, 2000, 2048, 2048, 2048,
};
const uint16_t ecg_mini[34] = {
    /* 基线 */
    2048, 2048, 2048,
    
    /* P 波（缩小） */
    2050, 2070, 2090, 2110, 2130, 2150, 2150, 2130, 2110, 2090,
    
    /* Q 波 */
    2070, 2050, 2030, 2010, 1990,
    
    /* R 波（主波） */
    2100, 2300, 2600, 3000, 3500, 4000, 4095, 4000, 3500, 3000,
    
    /* S 波与 T 波 */
    2500, 2200, 2000, 2100, 2200, 2048
};
void show_ad8232(void)
{
    uint16_t adc_value;
	u8 key_val=0;
	static u8 show_flag=0;
	static u16 time_count=0;
	char show_str[50];
	static u8 index=0;
	/* 实时 ECG：读 ADC，滚动显示（约 60Hz 量级取决于主循环） */

	
	if(show_flag==0)
	{
		LCD_Fill(0,0,LCD_W,LCD_H,BLACK);
		show_flag=1;
	}
//	if(time_count%2==0)
//	{
		adc_value = AD8232_ReadADC();
	if(adc_value>=4000){adc_value=4000;}
	else if(adc_value<=500){adc_value=500;}
//		adc_value = ecg_mini[index];
		index++;
		if(index >= 34){index = 0;}
		sprintf(show_str,"  %d  ", adc_value);
		LCD_ShowString((240-strlen(show_str)*12)/2,30,(uint8_t *)show_str,WHITE,BLACK,16,0);
		/* 采样值送入滚动波形 */
		ECG_Rolling_AddPoint(adc_value);
		ECG_Rolling_Update();
//	}
	key_val=KEY_Scan();
	if(key_val==1)
	{
		oled_flag++;
		show_flag=0;
		time_count=0;
		LCD_Fill(0,0,LCD_W,LCD_H,WHITE);
	}
	delay_ms(1);
	time_count++;
}




/* 综合数据页：心率、血氧、体温同屏 */
void Data_Show(void)
{
	char show_str[50];
	u8 key_val=0;
	static u8 show_flag=0;
	static u16 count=0;
	
	key_val=KEY_Scan();
	if(show_flag==0)/* 首次：绘制标题 */
	{
		LCD_ShowChinese(96,30,(uint8_t *)"心率",RED,WHITE,24,0);
		LCD_ShowChinese(96,90,(uint8_t *)"血氧",RED,WHITE,24,0);
		LCD_ShowChinese(96,150,(uint8_t *)"体温",RED,WHITE,24,0);
		show_flag=1;
	}
	if(count%200==0)/* 周期性刷新数值 */
	{
		if(maxXlXy.xl>0&&maxXlXy.xl<150){
			sprintf(show_str,"   %d bpm   ",maxXlXy.xl);
			LCD_ShowString((240-strlen(show_str)*12)/2,60,(uint8_t *)show_str,BLACK,WHITE,24,0);
		}else{
			sprintf(show_str,"   ---   ");
			LCD_ShowString((240-strlen(show_str)*12)/2,60,(uint8_t *)show_str,BLACK,WHITE,24,0);
		}
		if(maxXlXy.xy>30&&maxXlXy.xy<110){
			sprintf(show_str,"   %d %%   ",maxXlXy.xy);
			LCD_ShowString((240-strlen(show_str)*12)/2,120,(uint8_t *)show_str,BLACK,WHITE,24,0);
		}else{
			sprintf(show_str,"   ---   ");
			LCD_ShowString((240-strlen(show_str)*12)/2,120,(uint8_t *)show_str,BLACK,WHITE,24,0);
		}
		sprintf(show_str,"   %.1f C   ",temperature);
		LCD_ShowString((240-strlen(show_str)*12)/2,180,(uint8_t *)show_str,BLACK,WHITE,24,0);
	}
	if(key_val==1)/* 按键切回 */
	{
		LCD_Fill(0,0,LCD_W,LCD_H,WHITE);
		oled_flag++;
		show_flag=0;
	}
	count++;
	delay_ms(1);
}

/* 功能菜单：选择进入子功能 */
void Set_Mode_Show(void)
{
	u8 i=0;
	u8 key_val=0;
	static int8_t sel_flag=0;      /* 当前选中项 0~4 */
	static u8 show_flag=0;          /* 是否已画过本页 */
	static u8 last_sel_flag=10;      /* 上次选中，用于高亮刷新 */
	
	key_val=KEY_Scan();
	
	/* 首次进入：绘制菜单项（未选中白底，选中灰底） */
	if(show_flag==0)
	{
		LCD_ShowChinese(96,30*0+45,(uint8_t *)"秒表",BLACK,WHITE,24,0);
		LCD_ShowChinese(72,30*1+45,(uint8_t *)"校时",BLACK,WHITE,24,0);
		LCD_ShowChinese(72,30*2+45,(uint8_t *)"闹钟",BLACK,WHITE,24,0);
		LCD_ShowChinese(72,30*3+45,(uint8_t *)"报警",BLACK,WHITE,24,0);
		LCD_ShowChinese(72,30*4+45,(uint8_t *)"阈值",BLACK,WHITE,24,0);
		last_sel_flag = 10;
		show_flag = 1;
	}
	/* 键 2：下移 */
	if(key_val == 2)
	{
		sel_flag++;
		if(sel_flag >= 5) sel_flag = 0;
	}
	if(last_sel_flag != sel_flag)
	{
		/* 恢复上一项为普通色 */
		switch(last_sel_flag)
		{
			case 0:
				LCD_ShowChinese(96,30*0+45,(uint8_t *)"秒表",BLACK,WHITE,24,0);
				break;
			case 1:
				LCD_ShowChinese(72,30*1+45,(uint8_t *)"校时",BLACK,WHITE,24,0);
				break;
			case 2:
				LCD_ShowChinese(72,30*2+45,(uint8_t *)"闹钟",BLACK,WHITE,24,0);
				break;
			case 3:
				LCD_ShowChinese(72,30*3+45,(uint8_t *)"报警",BLACK,WHITE,24,0);
				break;
			case 4:
				LCD_ShowChinese(72,30*4+45,(uint8_t *)"阈值",BLACK,WHITE,24,0);
				break;
		}
		
		/* 高亮当前项 */
		switch(sel_flag)
		{
			case 0:
				LCD_ShowChinese(96,30*0+45,(uint8_t *)"秒表",BLACK,LGRAY,24,0);
				break;
			case 1:
				LCD_ShowChinese(72,30*1+45,(uint8_t *)"校时",BLACK,LGRAY,24,0);
				break;
			case 2:
				LCD_ShowChinese(72,30*2+45,(uint8_t *)"闹钟",BLACK,LGRAY,24,0);
				break;
			case 3:
				LCD_ShowChinese(72,30*3+45,(uint8_t *)"报警",BLACK,LGRAY,24,0);
				break;
			case 4:
				LCD_ShowChinese(72,30*4+45,(uint8_t *)"阈值",BLACK,LGRAY,24,0);
				break;
		}
		
		last_sel_flag = sel_flag;
	}
	
	/* 键 4：进入子功能 oled_flag=6~10 */
	if(key_val == 4)
	{
		oled_flag = sel_flag + 6;
		show_flag = 0;
		last_sel_flag = 10;
		LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
	}
	/* 键 1：返回表盘 */
	if(key_val == 1)
	{
		LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
		oled_flag = 0;
		show_flag = 0;
		last_sel_flag = 10;
	}
}


/* 秒表 */
void Stop_Watch(void)
{
	static char show_str[50];
	static u8 hour_count=0;
	static u8 minute_count=0;
	static u8 second_count=0;
	u8 key_val=0;
	static u8 start_flag=0;
	
	LCD_ShowChinese(96,30,(uint8_t *)"秒表",RED,WHITE,24,0);
	key_val=KEY_Scan();
	if(key_val==3)/* 键 3：复位 */
	{
		start_flag=0;
		hour_count=0;
		minute_count=0;
		second_count=0;
		stop_watch_count=0;
		TIM_Cmd(TIM4, DISABLE);
	}
	if(key_val==4)/* 键 4：启停 */
	{
		start_flag++;
		if(start_flag%2==1){TIM_Cmd(TIM4, ENABLE);}/* 启动 */
		else{TIM_Cmd(TIM4, DISABLE);}/* 停止 */
	}
	if(stop_watch_count>99){second_count++;stop_watch_count=0;}
	if(second_count>59){minute_count++;second_count=0;}/* 秒满 60 进分 */
	if(minute_count>59){hour_count++;minute_count=0;}/* 分满 60 进时 */
	
	sprintf(show_str,"%02d:%02d:%02d:%02d",hour_count,minute_count,second_count,stop_watch_count);/* 时:分:秒:百分秒 */
	if(start_flag==0){
		LCD_ShowString(58,108,(uint8_t *)show_str,BLACK,WHITE,24,0);
	}else if(start_flag%2==1){
		LCD_ShowString(58,108,(uint8_t *)show_str,RED,WHITE,24,0);
	}else if(start_flag%2==0){
		LCD_ShowString(58,108,(uint8_t *)show_str,BLUE,WHITE,24,0);
	}
	if(key_val==1)/* 键 1：返回菜单 */
	{
		start_flag=0;
		hour_count=0;
		minute_count=0;
		second_count=0;
		stop_watch_count=0;
		TIM_Cmd(TIM4, DISABLE);
		oled_flag=5;
		LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
	}
}
/* 校时：DS1307 */
void Set_Clock_Show(void)
{
	u8 i=0;
	u8 key_val=0;
	static int16_t sel_flag=0;      /* 选中项 0~6 */
	static u8 show_flag=0;
	static u8 last_sel_flag=10;
	static int16_t set_time[5]={0};
	char show_str[50];
	
	key_val=KEY_Scan();
	
	if(show_flag==0)
	{
		/* 从 RTC 读出当前时间作为初值 */
		DS1307_GetTime(&watch_time.year, &watch_time.month, &watch_time.date, 
			&watch_time.week, &watch_time.hour, &watch_time.minute, &watch_time.second);
		if(watch_time.year>2000&&watch_time.month!=0&&watch_time.date!=0)
		{
			set_time[0]=watch_time.year-2000;
			set_time[1]=watch_time.month;
			set_time[2]=watch_time.date;
			set_time[3]=watch_time.hour;
			set_time[4]=watch_time.minute;
		}else{
			set_time[0]=26;
			set_time[1]=4;
			set_time[2]=15;
			set_time[3]=12;
			set_time[4]=30;
		}
		LCD_ShowChinese(72,26*0+29,(uint8_t *)"年份",BLACK,WHITE,24,0);
		sprintf(show_str,":%02d",set_time[0]);
		LCD_ShowString(120,26*0+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
		LCD_ShowChinese(72,26*1+29,(uint8_t *)"月份",BLACK,WHITE,24,0);
		sprintf(show_str,":%02d",set_time[1]);
		LCD_ShowString(120,26*1+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
		LCD_ShowChinese(72,26*2+29,(uint8_t *)"日期",BLACK,WHITE,24,0);
		sprintf(show_str,":%02d",set_time[2]);
		LCD_ShowString(120,26*2+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
		LCD_ShowChinese(72,26*3+29,(uint8_t *)"小时",BLACK,WHITE,24,0);
		sprintf(show_str,":%02d",set_time[3]);
		LCD_ShowString(120,26*3+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
		LCD_ShowChinese(72,26*4+29,(uint8_t *)"分钟",BLACK,WHITE,24,0);
		sprintf(show_str,":%02d",set_time[4]);
		LCD_ShowString(120,26*4+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
		LCD_ShowChinese(72,26*5+29,(uint8_t *)"保存时间",BLACK,WHITE,24,0);
		LCD_ShowChinese(72,26*6+29,(uint8_t *)"取消返回",BLACK,WHITE,24,0);
		sel_flag = 0;
		last_sel_flag = 10;
		show_flag = 1;
	}
	
	/* 键 1/2：上下移动选中项 */
	if(key_val == 1){sel_flag--;}
	if(key_val == 2){sel_flag++;}
	limit_value(&sel_flag,0,6,0);
	if(sel_flag<5)
	{
		if(key_val==3){set_time[sel_flag]--;}
		if(key_val==4){set_time[sel_flag]++;}
		limit_value(&set_time[0],0,99,0);
		limit_value(&set_time[1],1,12,0);
		limit_value(&set_time[2],1,31,0);
		limit_value(&set_time[3],0,23,0);
		limit_value(&set_time[4],0,59,0);
	}
	
	if(last_sel_flag != sel_flag)
	{
		switch(last_sel_flag)
		{
			case 0:
				LCD_ShowChinese(72,26*0+29,(uint8_t *)"年份",BLACK,WHITE,24,0);
				sprintf(show_str,":%02d",set_time[0]);
				LCD_ShowString(120,26*0+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
				break;
			case 1:
				LCD_ShowChinese(72,26*1+29,(uint8_t *)"月份",BLACK,WHITE,24,0);
				sprintf(show_str,":%02d",set_time[1]);
				LCD_ShowString(120,26*1+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
				break;
			case 2:
				LCD_ShowChinese(72,26*2+29,(uint8_t *)"日期",BLACK,WHITE,24,0);
				sprintf(show_str,":%02d",set_time[2]);
				LCD_ShowString(120,26*2+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
				break;
			case 3:
				LCD_ShowChinese(72,26*3+29,(uint8_t *)"小时",BLACK,WHITE,24,0);
				sprintf(show_str,":%02d",set_time[3]);
				LCD_ShowString(120,26*3+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
				break;
			case 4:
				LCD_ShowChinese(72,26*4+29,(uint8_t *)"分钟",BLACK,WHITE,24,0);
				sprintf(show_str,":%02d",set_time[4]);
				LCD_ShowString(120,26*4+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
				break;
			case 5:
				LCD_ShowChinese(72,26*5+29,(uint8_t *)"保存时间",BLACK,WHITE,24,0);
				break;
			case 6:
				LCD_ShowChinese(72,26*6+29,(uint8_t *)"取消返回",BLACK,WHITE,24,0);
				break;
		}
		last_sel_flag = sel_flag;
	}
	switch(sel_flag)
	{
		case 0:
			LCD_ShowChinese(72,26*0+29,(uint8_t *)"年份",BLACK,LGRAY,24,0);
			sprintf(show_str,":%02d",set_time[0]);
			LCD_ShowString(120,26*0+29,(uint8_t *)show_str,BLACK,LGRAY,24,0);
			break;
		case 1:
			LCD_ShowChinese(72,26*1+29,(uint8_t *)"月份",BLACK,LGRAY,24,0);
			sprintf(show_str,":%02d",set_time[1]);
			LCD_ShowString(120,26*1+29,(uint8_t *)show_str,BLACK,LGRAY,24,0);
			break;
		case 2:
			LCD_ShowChinese(72,26*2+29,(uint8_t *)"日期",BLACK,LGRAY,24,0);
			sprintf(show_str,":%02d",set_time[2]);
			LCD_ShowString(120,26*2+29,(uint8_t *)show_str,BLACK,LGRAY,24,0);
			break;
		case 3:
			LCD_ShowChinese(72,26*3+29,(uint8_t *)"小时",BLACK,LGRAY,24,0);
			sprintf(show_str,":%02d",set_time[3]);
			LCD_ShowString(120,26*3+29,(uint8_t *)show_str,BLACK,LGRAY,24,0);
			break;
		case 4:
			LCD_ShowChinese(72,26*4+29,(uint8_t *)"分钟",BLACK,LGRAY,24,0);
			sprintf(show_str,":%02d",set_time[4]);
			LCD_ShowString(120,26*4+29,(uint8_t *)show_str,BLACK,LGRAY,24,0);
			break;
		case 5:
			LCD_ShowChinese(72,26*5+29,(uint8_t *)"保存时间",BLACK,LGRAY,24,0);
			if(key_val == 4||key_val == 3)
			{
				watch_time.week=DS1307_Get_Week(set_time[0]+2000, set_time[1], set_time[2]);
				DS1307_SetTime(set_time[0], set_time[1], set_time[2], watch_time.week, set_time[3], set_time[4], 0);
				LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
				oled_flag = 2;
				show_flag = 0;
				sel_flag=0;
				last_sel_flag = 10;
			}
			break;
		case 6:
			LCD_ShowChinese(72,26*6+29,(uint8_t *)"取消返回",BLACK,LGRAY,24,0);
			if(key_val == 4||key_val == 3)
			{
				LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
				oled_flag = 5;
				show_flag = 0;
				sel_flag=0;
				last_sel_flag = 10;
			}
			break;
	}
}

/* 闹钟设置 */
void Set_Alarm_Show(void)
{
	u8 i=0;
	u8 key_val=0;
	static int16_t sel_flag=0;
	static u8 show_flag=0;
	static u8 last_sel_flag=10;
	static int16_t set_time[6]={0};
	char show_str[50];
	
	key_val=KEY_Scan();
	
	if(show_flag==0)
	{
		set_time[0]=alarm_str[0]/100;
		set_time[1]=alarm_str[0]%100;
		set_time[2]=alarm_str[1]/100;
		set_time[3]=alarm_str[1]%100;
		set_time[4]=alarm_str[2]/100;
		set_time[5]=alarm_str[2]%100;
		if(alarm_flag[0]%2){LCD_ShowChinese(60,26*0+29,(uint8_t *)"闹钟一",RED,WHITE,24,0);}
		else{LCD_ShowChinese(60,26*0+29,(uint8_t *)"闹钟一",BLACK,WHITE,24,0);}
		sprintf(show_str,"%02d:%02d",set_time[0],set_time[1]);
		LCD_ShowString(90,26*1+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
		
		if(alarm_flag[1]%2){LCD_ShowChinese(60,26*2+29,(uint8_t *)"闹钟二",RED,WHITE,24,0);}
		else{LCD_ShowChinese(60,26*2+29,(uint8_t *)"闹钟二",BLACK,WHITE,24,0);}
		sprintf(show_str,"%02d:%02d",set_time[2],set_time[3]);
		LCD_ShowString(90,26*3+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
		
		if(alarm_flag[2]%2){LCD_ShowChinese(60,26*4+29,(uint8_t *)"闹钟三",RED,WHITE,24,0);}
		else{LCD_ShowChinese(60,26*4+29,(uint8_t *)"闹钟三",BLACK,WHITE,24,0);}
		sprintf(show_str,"%02d:%02d",set_time[4],set_time[5]);
		LCD_ShowString(90,26*5+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
		
		LCD_ShowChinese(72,26*6+29,(uint8_t *)"保存退出",BLACK,WHITE,24,0);
		sel_flag = 0;
		last_sel_flag = 10;
		show_flag = 1;
	}
	
	/* 键 1/2：移动焦点 */
	if(key_val == 1){sel_flag--;}
	if(key_val == 2){sel_flag++;}
	limit_value(&sel_flag,0,9,0);
	if(sel_flag%3==0)
	{
		if(key_val==3){alarm_flag[sel_flag/3]--;}
		if(key_val==4){alarm_flag[sel_flag/3]++;}
	}
	
	if(last_sel_flag != sel_flag)
	{
		switch(last_sel_flag)
		{
			case 0:
				if(alarm_flag[0]%2){LCD_ShowChinese(60,26*0+29,(uint8_t *)"闹钟一",RED,WHITE,24,0);}
				else{LCD_ShowChinese(60,26*0+29,(uint8_t *)"闹钟一",BLACK,WHITE,24,0);}
				break;
			case 1: 
				sprintf(show_str,"%02d",set_time[0]);
				LCD_ShowString(90,26*1+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
				break;
			case 2:
				sprintf(show_str,"%02d",set_time[1]);
				LCD_ShowString(90+12*3,26*1+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
				break;
			case 3: 
				if(alarm_flag[1]%2){LCD_ShowChinese(60,26*2+29,(uint8_t *)"闹钟二",RED,WHITE,24,0);}
				else{LCD_ShowChinese(60,26*2+29,(uint8_t *)"闹钟二",BLACK,WHITE,24,0);}
				break;
			case 4: 
				sprintf(show_str,"%02d",set_time[2]);
				LCD_ShowString(90,26*3+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
				break;
			case 5: 
				sprintf(show_str,"%02d",set_time[3]);
				LCD_ShowString(90+12*3,26*3+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
				break;
			case 6: 
				if(alarm_flag[2]%2){LCD_ShowChinese(60,26*4+29,(uint8_t *)"闹钟三",RED,WHITE,24,0);}
				else{LCD_ShowChinese(60,26*4+29,(uint8_t *)"闹钟三",BLACK,WHITE,24,0);}
				break;
			case 7: 
				sprintf(show_str,"%02d",set_time[4]);
				LCD_ShowString(90,26*5+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
				break;
			case 8: 
				sprintf(show_str,"%02d",set_time[5]);
				LCD_ShowString(90+12*3,26*5+29,(uint8_t *)show_str,BLACK,WHITE,24,0);
				break;
			case 9: 
				LCD_ShowChinese(72,26*6+29,(uint8_t *)"保存退出",BLACK,WHITE,24,0);
				break;
		}
		last_sel_flag = sel_flag;
	}
	switch(sel_flag)
	{
		case 0:
			if(alarm_flag[0]%2){LCD_ShowChinese(60,26*0+29,(uint8_t *)"闹钟一",RED,LGRAY,24,0);}
			else{LCD_ShowChinese(60,26*0+29,(uint8_t *)"闹钟一",BLACK,LGRAY,24,0);}
			break;
		case 1: 
			sprintf(show_str,"%02d",set_time[0]);
			LCD_ShowString(90,26*1+29,(uint8_t *)show_str,BLACK,LGRAY,24,0);
			if(key_val==3){set_time[0]--;}
			if(key_val==4){set_time[0]++;}
			limit_value(&set_time[0],0,23,0);
			break;
		case 2:
			sprintf(show_str,"%02d",set_time[1]);
			LCD_ShowString(90+12*3,26*1+29,(uint8_t *)show_str,BLACK,LGRAY,24,0);
			if(key_val==3){set_time[1]--;}
			if(key_val==4){set_time[1]++;}
			limit_value(&set_time[1],0,59,0);
			break;
		case 3: 
			if(alarm_flag[1]%2){LCD_ShowChinese(60,26*2+29,(uint8_t *)"闹钟二",RED,LGRAY,24,0);}
			else{LCD_ShowChinese(60,26*2+29,(uint8_t *)"闹钟二",BLACK,LGRAY,24,0);}
			break;
		case 4: 
			sprintf(show_str,"%02d",set_time[2]);
			LCD_ShowString(90,26*3+29,(uint8_t *)show_str,BLACK,LGRAY,24,0);
			if(key_val==3){set_time[2]--;}
			if(key_val==4){set_time[2]++;}
			limit_value(&set_time[1],0,23,0);
			break;
		case 5: 
			sprintf(show_str,"%02d",set_time[3]);
			LCD_ShowString(90+12*3,26*3+29,(uint8_t *)show_str,BLACK,LGRAY,24,0);
			if(key_val==3){set_time[3]--;}
			if(key_val==4){set_time[3]++;}
			limit_value(&set_time[3],0,59,0);
			break;
		case 6: 
			if(alarm_flag[2]%2){LCD_ShowChinese(60,26*4+29,(uint8_t *)"闹钟三",RED,LGRAY,24,0);}
			else{LCD_ShowChinese(60,26*4+29,(uint8_t *)"闹钟三",BLACK,LGRAY,24,0);}
			break;
		case 7: 
			sprintf(show_str,"%02d",set_time[4]);
			LCD_ShowString(90,26*5+29,(uint8_t *)show_str,BLACK,LGRAY,24,0);
			if(key_val==3){set_time[4]--;}
			if(key_val==4){set_time[4]++;}
			limit_value(&set_time[4],0,23,0);
			break;
		case 8: 
			sprintf(show_str,"%02d",set_time[5]);
			LCD_ShowString(90+12*3,26*5+29,(uint8_t *)show_str,BLACK,LGRAY,24,0);
			if(key_val==3){set_time[5]--;}
			if(key_val==4){set_time[5]++;}
			limit_value(&set_time[5],0,59,0);
			break;
		case 9: 
			LCD_ShowChinese(72,26*6+29,(uint8_t *)"保存退出",BLACK,LGRAY,24,0);
			if(key_val == 4||key_val == 3)
			{
				alarm_str[0]=set_time[0]*100+set_time[1];
				alarm_str[1]=set_time[2]*100+set_time[3];
				alarm_str[2]=set_time[4]*100+set_time[5];
				LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
				oled_flag = 5;
				show_flag = 0;
				sel_flag=0;
				last_sel_flag = 10;
			}
			break;
	}
}

/* 报警开关：心率/血氧/体温 */
void Set_Warn_Show(void)
{
	u8 i=0;
	u8 key_val=0;
	static int16_t sel_flag=0;
	static u8 show_flag=0;
	static u8 last_sel_flag=10;
	static int16_t set_time[6]={0};
	char show_str[50];
	
	key_val=KEY_Scan();
	if(show_flag==0)
	{
		if(warn_mode[0]%2){LCD_ShowChinese(48,30*0+45,(uint8_t *)"心率报警",RED,WHITE,24,0);}
		else{LCD_ShowChinese(48,30*0+45,(uint8_t *)"心率报警",BLACK,WHITE,24,0);}
		if(warn_mode[1]%2){LCD_ShowChinese(48,30*1+45,(uint8_t *)"血氧报警",RED,WHITE,24,0);}
		else{LCD_ShowChinese(48,30*1+45,(uint8_t *)"血氧报警",BLACK,WHITE,24,0);}
		if(warn_mode[2]%2){LCD_ShowChinese(48,30*2+45,(uint8_t *)"体温报警",RED,WHITE,24,0);}
		else{LCD_ShowChinese(48,30*2+45,(uint8_t *)"体温报警",BLACK,WHITE,24,0);}
		
		LCD_ShowChinese(72,30*3+45,(uint8_t *)"返回",BLACK,WHITE,24,0);
		sel_flag = 0;
		last_sel_flag = 10;
		show_flag = 1;
	}
	
	if(key_val == 1){sel_flag--;}
	if(key_val == 2){sel_flag++;}
	limit_value(&sel_flag,0,3,0);
	if(sel_flag<4)
	{
		if(key_val==3){warn_mode[sel_flag]--;}
		if(key_val==4){warn_mode[sel_flag]++;}
	}
	
	if(last_sel_flag != sel_flag)
	{
		switch(last_sel_flag)
		{
			case 0:
				if(warn_mode[0]%2){LCD_ShowChinese(48,30*0+45,(uint8_t *)"心率报警",RED,WHITE,24,0);}
				else{LCD_ShowChinese(48,30*0+45,(uint8_t *)"心率报警",BLACK,WHITE,24,0);}
				break;
			case 1: 
				if(warn_mode[1]%2){LCD_ShowChinese(48,30*1+45,(uint8_t *)"血氧报警",RED,WHITE,24,0);}
				else{LCD_ShowChinese(48,30*1+45,(uint8_t *)"血氧报警",BLACK,WHITE,24,0);}
				break;
			case 2:
				if(warn_mode[2]%2){LCD_ShowChinese(48,30*2+45,(uint8_t *)"体温报警",RED,WHITE,24,0);}
				else{LCD_ShowChinese(48,30*2+45,(uint8_t *)"体温报警",BLACK,WHITE,24,0);}
				break;
			case 3: 
				LCD_ShowChinese(72,30*3+45,(uint8_t *)"返回",BLACK,WHITE,24,0);
				break;
			case 4: 
				break;
		}
		last_sel_flag = sel_flag;
	}
	switch(sel_flag)
	{
		case 0:
			if(warn_mode[0]%2){LCD_ShowChinese(48,30*0+45,(uint8_t *)"心率报警",RED,LGRAY,24,0);}
			else{LCD_ShowChinese(48,30*0+45,(uint8_t *)"心率报警",BLACK,LGRAY,24,0);}
			break;
		case 1: 
			if(warn_mode[1]%2){LCD_ShowChinese(48,30*1+45,(uint8_t *)"血氧报警",RED,LGRAY,24,0);}
			else{LCD_ShowChinese(48,30*1+45,(uint8_t *)"血氧报警",BLACK,LGRAY,24,0);}
			break;
		case 2:
			if(warn_mode[2]%2){LCD_ShowChinese(48,30*2+45,(uint8_t *)"体温报警",RED,LGRAY,24,0);}
			else{LCD_ShowChinese(48,30*2+45,(uint8_t *)"体温报警",BLACK,LGRAY,24,0);}
			break;
		case 3: 
			LCD_ShowChinese(72,30*3+45,(uint8_t *)"返回",BLACK,LGRAY,24,0);
			if(key_val == 4||key_val == 3)
			{
				LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
				oled_flag = 5;
				show_flag = 0;
				sel_flag=0;
				last_sel_flag = 10;
			}
			break;
		case 4: 
			break;
	}
}

/* 阈值设置 */
void Set_Threshold_Show(void)
{
	u8 key_val=0;
	static int16_t sel_flag=0;
	static u8 show_flag=0;
	static u8 last_sel_flag=10;
	char show_str[50];
	
	key_val=KEY_Scan();
	if(show_flag==0)/* 首次绘制各阈值行 */
	{
		LCD_ShowChinese(72,26*0+29,(uint8_t *)"心率阈值",BLACK,WHITE,24,0);
		sprintf(show_str," %d|",threshold[0]);
		LCD_ShowString(120-strlen(show_str)*12,26*1+29,(uint8_t *)show_str,BLUE,WHITE,24,0);
		sprintf(show_str,"|%d ",threshold[1]);
		LCD_ShowString(120,26*1+29,(uint8_t *)show_str,RED,WHITE,24,0);
		
		LCD_ShowChinese(72,26*2+29,(uint8_t *)"血氧下限",BLACK,WHITE,24,0);
		sprintf(show_str," %d ",threshold[2]);
		LCD_ShowString((240-strlen(show_str)*12)/2,26*3+29,(uint8_t *)show_str,BLUE,WHITE,24,0);
		
		LCD_ShowChinese(72,26*4+29,(uint8_t *)"体温范围",BLACK,WHITE,24,0);
		sprintf(show_str," %d|",threshold[3]);
		LCD_ShowString(120-strlen(show_str)*12,26*5+29,(uint8_t *)show_str,BLUE,WHITE,24,0);
		sprintf(show_str,"|%d ",threshold[4]);
		LCD_ShowString(120,26*5+29,(uint8_t *)show_str,RED,WHITE,24,0);
		
		LCD_ShowChinese(72,26*6+29,(uint8_t *)"保存返回",BLACK,WHITE,24,0);
		show_flag=1;
	}
	if(key_val == 1){sel_flag--;}
	if(key_val == 2){sel_flag++;}
	limit_value(&sel_flag,0,5,0);
	if(sel_flag<5)
	{
		if(key_val==3){threshold[sel_flag]--;}
		if(key_val==4){threshold[sel_flag]++;}
	}
	
	if(last_sel_flag != sel_flag)
	{
		switch(last_sel_flag)
		{
			case 0:
				sprintf(show_str,"%d|",threshold[0]);
				LCD_Fill(120-strlen(show_str)*12-24,26*1+29,120-strlen(show_str)*12, 26*1+29+24, WHITE);
				LCD_ShowString(120-strlen(show_str)*12,26*1+29,(uint8_t *)show_str,BLUE,WHITE,24,0);
				break;
			case 1: 
				sprintf(show_str,"|%d",threshold[1]);
				LCD_Fill(120+strlen(show_str)*12,26*1+29,120+strlen(show_str)*12+24, 26*1+29+24, WHITE);
				LCD_ShowString(120,26*1+29,(uint8_t *)show_str,RED,WHITE,24,0);
				break;
			case 2:
				sprintf(show_str,"%d",threshold[2]);
				LCD_Fill(120-strlen(show_str)*12/2-24,26*3+29,120-strlen(show_str)*12/2, 26*3+29+24, WHITE);
				LCD_Fill(120+strlen(show_str)*12/2,26*3+29,120+strlen(show_str)*12/2+24, 26*3+29+24, WHITE);
				LCD_ShowString((240-strlen(show_str)*12)/2,26*3+29,(uint8_t *)show_str,BLUE,WHITE,24,0);
				break;
			case 3: 
				sprintf(show_str,"%d|",threshold[3]);
				LCD_Fill(120-strlen(show_str)*12-24,26*5+29,120-strlen(show_str)*12, 26*5+29+24, WHITE);
				LCD_ShowString(120-strlen(show_str)*12,26*5+29,(uint8_t *)show_str,BLUE,WHITE,24,0);
				break;
			case 4: 
				sprintf(show_str,"|%d",threshold[4]);
				LCD_Fill(120+strlen(show_str)*12,26*5+29,120+strlen(show_str)*12+24, 26*5+29+24, WHITE);
				LCD_ShowString(120,26*5+29,(uint8_t *)show_str,RED,WHITE,24,0);
				break;
			case 5: 
				LCD_ShowChinese(72,26*6+29,(uint8_t *)"保存返回",BLACK,WHITE,24,0);
				break;
		}
		last_sel_flag = sel_flag;
	}
	switch(sel_flag)
	{
		case 0:
			sprintf(show_str," %d|",threshold[0]);
			LCD_ShowString(120-strlen(show_str)*12,26*1+29,(uint8_t *)show_str,BLUE,LGRAY,24,0);
			break;
		case 1: 
			sprintf(show_str,"|%d ",threshold[1]);
			LCD_ShowString(120,26*1+29,(uint8_t *)show_str,RED,LGRAY,24,0);
			break;
		case 2:
			sprintf(show_str," %d ",threshold[2]);
			LCD_ShowString((240-strlen(show_str)*12)/2,26*3+29,(uint8_t *)show_str,BLUE,LGRAY,24,0);
			break;
		case 3: 
			sprintf(show_str," %d|",threshold[3]);
			LCD_ShowString(120-strlen(show_str)*12,26*5+29,(uint8_t *)show_str,BLUE,LGRAY,24,0);
			break;
		case 4: 
			sprintf(show_str,"|%d ",threshold[4]);
			LCD_ShowString(120,26*5+29,(uint8_t *)show_str,RED,LGRAY,24,0);
			break;
		case 5: 
			LCD_ShowChinese(72,26*6+29,(uint8_t *)"保存返回",BLACK,LGRAY,24,0);
			if(key_val == 4||key_val == 3)
			{
				LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
				oled_flag = 5;
				show_flag = 0;
				sel_flag=0;
				last_sel_flag = 10;
			}
			break;
	}
}



/* 去掉 b 个最小、a 个最大后求平均 */
float mean_remove_extremes_select(float *arr, int n, int a, int b)
{
    float *temp;
    int i, j;
    int min_idx, max_idx;
    float t;
    float sum = 0.0f;

    if (n <= 0 || a < 0 || b < 0 || a + b >= n){return 0;}
    temp = (float *)malloc(n * sizeof(float));
    if (temp == NULL)
        return 0.0f;
    memcpy(temp, arr, n * sizeof(float));
    /* 前 b 轮：每次找全局最小换到前端 */
    for (i = 0; i < b; i++)
    {
        min_idx = i;
        for (j = i + 1; j < n; j++)
        {
            if (temp[j] < temp[min_idx])
            {
                min_idx = j;
            }
        }
        if (min_idx != i)
        {
            t = temp[i];
            temp[i] = temp[min_idx];
            temp[min_idx] = t;
        }
    }
    /* 后 a 轮：从尾部起每次找最大换到末尾 */
    for (i = 0; i < a; i++)
    {
        max_idx = n - 1 - i;
        for (j = 0; j < n - i; j++)
        {
            if (temp[j] > temp[max_idx])
            {
                max_idx = j;
            }
        }
        if (max_idx != n - 1 - i)
        {
            t = temp[n - 1 - i];
            temp[n - 1 - i] = temp[max_idx];
            temp[max_idx] = t;
        }
    }
    /* 中间段求和平均 */
    for (i = b; i < n - a; i++)
    {
        sum += temp[i];
    }
    free(temp);
    return sum / (n - a - b);
}


void limit_value(int16_t *data,int16_t min,int16_t max,u8 mode)
{
	if(mode==0)
	{
		if(*data<min){*data=max;}
		if(*data>max){*data=min;}
	}else{
		if(*data<=min){*data=min;}
		if(*data>=max){*data=max;}
	}
}




