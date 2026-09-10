# 基于 STM32 的智能健康手环

本项目是一套面向毕业设计与嵌入式综合实践的智能健康监测手环系统。系统以 STM32F103 为核心控制器，结合心率血氧、体温、心电、加速度、LCD、蓝牙、蜂鸣器与语音模块，实现生命体征采集、数据显示、异常报警、跌倒检测、阈值设置和手机端实时查看等功能。

> 说明：本项目用于课程设计、毕业设计和嵌入式学习展示，采集数据不作为医疗诊断依据。

## 项目功能

- 实时采集心率、血氧、体温与心电 ADC 数据。
- 通过 LCD 显示实时时钟、健康指标、心电波形、菜单、闹钟和阈值设置界面。
- 支持心率、血氧、体温报警阈值设置。
- 当生命体征超过阈值或检测到跌倒风险时，触发 LED、蜂鸣器、蓝牙报警与语音提示。
- 通过蓝牙 SPP 协议将数据发送到 Android 手机 App。
- Android App 支持蓝牙连接、实时数据展示、历史记录、阈值保存与阈值下发。
- 集成 DS1307 实时时钟，可显示日期时间并支持闹钟提醒。

## 技术栈

### 嵌入式端

- MCU：STM32F103 系列
- 开发语言：C
- 开发环境：Keil uVision5
- 固件库：STM32F10x Standard Peripheral Library
- 显示：LCD 屏幕
- 通信：USART 蓝牙串口，默认 9600 baud
- 传感器与模块：
  - MAX30102：心率与血氧采集
  - DS18B20：体温采集
  - AD8232：心电模拟信号采集
  - MPU6050：加速度采集与跌倒检测基础
  - DS1307：实时时钟
  - ASRPRO：语音播报/语音交互模块
  - 蜂鸣器、LED、按键：本地报警与交互

### 移动端

- Android 原生应用
- Kotlin
- Jetpack Compose
- StateFlow 状态管理
- Bluetooth SPP 通信
- SharedPreferences 阈值持久化
- 本地历史数据记录

## 目录结构

```text
smart_bracelet/
├─ firmware/                  # STM32 固件工程
│  ├─ CORE/                   # Cortex-M3 内核相关文件
│  ├─ DSPLIB/                 # CMSIS DSP 相关文件
│  ├─ HARDWARE/               # 传感器、蓝牙、LCD、蜂鸣器等驱动
│  ├─ STM32F10x_FWLib/        # STM32 标准外设库
│  ├─ SYSTEM/                 # delay、sys、usart 等系统基础模块
│  ├─ USER/                   # main.c 与 Keil 工程文件
│  └─ diagrams/               # 项目框图与 Mermaid 图
├─ mobile_app/                # Android 手机蓝牙 App
│  ├─ app/
│  ├─ gradle/
│  ├─ build.gradle.kts
│  ├─ settings.gradle.kts
│  └─ gradlew / gradlew.bat
├─ docs/
│  └─ schematic.pdf           # 电路原理图
├─ .gitignore
└─ README.md
```

## 固件工程运行方式

1. 安装 Keil uVision5，并安装 STM32F1 系列相关支持包。
2. 打开 `firmware/USER/target.uvprojx`。
3. 检查工程中的芯片型号、晶振频率、下载器配置是否与实际硬件一致。
4. 连接 ST-Link/J-Link 下载器。
5. 编译工程，确认无错误后下载到 STM32 开发板或手环硬件。
6. 上电后 LCD 会初始化并显示健康监测主界面。

主要入口位于：

```c
int main(void)
{
  Hardware_Init();
  while(1)
  {
    Get_Data();
    OLED_Show();
    Hardware_Hander();
  }
}
```

其中：

- `Hardware_Init()`：初始化 LCD、LED、按键、蜂鸣器、定时器、温度、心率血氧、RTC、蓝牙、语音模块和心电采集。
- `Get_Data()`：周期性读取体温、心率血氧、RTC 时间和 AD8232 心电数据。
- `OLED_Show()`：根据当前页面状态显示表盘、心率、血氧、体温、心电、菜单、闹钟和阈值界面。
- `Hardware_Hander()`：处理报警判断、蓝牙上报、闹钟、语音提示等业务逻辑。

## Android App 运行方式

1. 安装 Android Studio。
2. 使用 Android Studio 打开 `mobile_app/`。
3. 等待 Gradle 同步完成。
4. 使用真机运行，并授予蓝牙相关权限。
5. 在系统蓝牙设置中先配对手环蓝牙模块。
6. 进入 App 后刷新设备列表，选择已配对设备连接。
7. 连接成功后，App 会显示 STM32 上传的体温、心率、血氧和心电数据。

## 蓝牙通信格式

固件端通过串口蓝牙发送 ASCII 文本，默认波特率为 `9600`。

上报数据示例：

```text
Temperature:  36.5 C
Heart Rate:  78 bpm
Blood Oxygen: 98 %
AD8232: 1840 1855 1872 1861
```

报警提示示例：

```text
Danger!!!
```

App 下发阈值命令格式：

```text
TH:0:60
TH:1:120
TH:2:95
TH:3:36
TH:4:38
```

阈值索引说明：

| 索引 | 含义 |
| --- | --- |
| `0` | 心率下限 |
| `1` | 心率上限 |
| `2` | 血氧下限 |
| `3` | 体温下限 |
| `4` | 体温上限 |

固件也支持一次性批量设置：

```text
TH:60,120,95,36,38
```

## 关键模块说明

- `firmware/HARDWARE/driver.c`：项目业务核心，负责数据采集调度、报警判断、蓝牙发送、页面切换与闹钟处理。
- `firmware/HARDWARE/bluetooth.c`：蓝牙串口初始化、数据发送与阈值命令解析。
- `firmware/HARDWARE/max30102.c`：MAX30102 心率血氧模块通信。
- `firmware/HARDWARE/ds18b20.c`：DS18B20 单总线温度采集。
- `firmware/HARDWARE/ad8232.c`：AD8232 心电信号 ADC 采集。
- `firmware/HARDWARE/mpu6050.c`：MPU6050 I2C 通信与加速度读取。
- `mobile_app/app/src/main/java/com/example/test/bluetooth/BluetoothManager.kt`：Android 蓝牙连接、收发与断开处理。
- `mobile_app/app/src/main/java/com/example/test/data/HealthViewModel.kt`：App 数据解析、状态管理、阈值保存与命令下发。

## 硬件连接检查建议

在运行前建议确认：

- 蓝牙模块 TX/RX 与 STM32 USART 引脚交叉连接。
- MAX30102 与 MPU6050 的 I2C 引脚、电源和上拉电阻连接正确。
- DS18B20 数据线有合适上拉电阻。
- AD8232 输出接入 STM32 ADC 对应引脚。
- LCD 屏幕引脚与工程驱动配置一致。
- 蜂鸣器、LED、按键与代码中的 GPIO 定义一致。

## 项目截图

### 电路原理图

![电路原理图](https://piv.cc.cd/file/BQACAgUAAyEGAASLVN5eAAJsrGqiwzwnoVxFZW45FYv3jmPBV1X0AAKpJAACMD4RVcp375poFusuPQQ.jpg)

### Android App

![Android App](https://piv.cc.cd/file/BQACAgUAAyEGAASLVN5eAAJsq2qiwwS1PDzK5X5DznHojC-fakx_AAKmJAACMD4RVYMJgbN5y5T4PQQ.jpg)

### 手环 PCB

![手环 PCB](https://piv.cc.cd/file/BQACAgUAAyEGAASLVN5eAAJsr2qixgFZqtQuWzM36BlFbsW4LUyHAAKzJAACMD4RVUukm-AtwMjQPQQ.jpg)

