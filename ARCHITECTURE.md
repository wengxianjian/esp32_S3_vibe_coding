# ESP32 LED Vibe Coding - 项目架构分析

## 1. 项目概述

本项目是基于 **ESP-IDF** 框架的 ESP32-S3 嵌入式应用，实现了 LED 灯光效果控制，支持呼吸灯和闪烁两种模式，通过按键切换。

- **目标芯片**: ESP32-S3
- **开发框架**: ESP-IDF (Espressif IoT Development Framework)
- **构建系统**: CMake + Ninja
- **编程语言**: C

---

## 2. 目录结构

```
esp32_led_vibe_coding/
├── CMakeLists.txt              # 项目级 CMake 配置
├── main/                       # 主程序目录
│   ├── CMakeLists.txt          # main 组件注册
│   └── main.c                  # 程序入口 (app_main)
├── components/                 # 自定义组件目录
│   ├── BSP/                    # 板级支持包 (Board Support Package)
│   │   ├── CMakeLists.txt      # BSP 组件注册
│   │   ├── LED/                # LED 驱动模块
│   │   │   ├── led.h           # LED 接口定义
│   │   │   └── led.c           # LED 驱动实现
│   │   └── BUTTON/             # 按键驱动模块
│   │       ├── button.h        # 按键接口定义
│   │       └── button.c        # 按键驱动实现
│   └── express/                # 表达式组件 (未使用)
│       ├── CMakeLists.txt
│       ├── include/
│       │   └── express.h
│       └── express.c
├── .vscode/                    # VS Code 开发配置
│   ├── c_cpp_properties.json
│   ├── launch.json
│   ├── settings.json
│   └── tasks.json
└── build/                      # 编译输出目录 (自动生成)
```

---

## 3. 架构分层

```
┌─────────────────────────────────────────────────────┐
│                   Application Layer                  │
│                    main.c (app_main)                 │
│         - 主循环控制                                  │
│         - NVS 初始化                                  │
│         - 模式切换逻辑                                │
├─────────────────────────────────────────────────────┤
│                   BSP Layer (components/BSP)         │
│  ┌─────────────────────┐  ┌────────────────────────┐ │
│  │       LED Module    │  │      BUTTON Module     │ │
│  │  - LEDC PWM 控制    │  │  - GPIO 输入检测        │ │
│  │  - 呼吸灯效果       │  │  - 按键扫描             │ │
│  │  - 闪烁效果         │  │  - 回调函数机制         │ │
│  └─────────────────────┘  └────────────────────────┘ │
├─────────────────────────────────────────────────────┤
│                   ESP-IDF HAL/Driver Layer           │
│  - driver/gpio        - GPIO 驱动                    │
│  - driver/ledc        - LED PWM 控制器驱动           │
│  - nvs_flash          - 非易失性存储                 │
│  - freertos           - 实时操作系统                 │
├─────────────────────────────────────────────────────┤
│                   Hardware Layer                     │
│  - ESP32-S3 芯片                                     │
│  - GPIO1  -> LED                                     │
│  - GPIO0  -> BUTTON                                  │
└─────────────────────────────────────────────────────┘
```

---

## 4. 模块详细说明

### 4.1 main.c - 主程序

**职责**: 系统初始化、主循环控制、模式管理

```
初始化流程:
  1. 打印启动信息
  2. 初始化 NVS (Non-Volatile Storage)
  3. 初始化 LED 模块
  4. 初始化 BUTTON 模块并注册回调

主循环 (while(1)):
  1. button_scan() - 扫描按键状态
  2. 根据 led_mode_breathing 标志选择:
     - true:  ledc_breathing_led()  - 呼吸灯效果
     - false: led_blink_500ms()     - 500ms 闪烁
  3. vTaskDelay(20) - 延时 20ms (50Hz 循环频率)
```

**回调机制**:
- 按键按下时触发 `button_pressed_callback()`
- 回调函数切换 `led_mode_breathing` 标志

### 4.2 LED 模块 (components/BSP/LED/)

**硬件配置**:
| 参数 | 值 |
|------|-----|
| GPIO 引脚 | GPIO_NUM_1 |
| PWM 频率 | 1 kHz |
| 占空比分辨率 | 8-bit (0-255) |
| LEDC 定时器 | TIMER_0 |
| LEDC 通道 | CHANNEL_0 |
| LEDC 模式 | LOW_SPEED_MODE |

**API 接口**:
```c
void led_init(void);                    // 初始化 LEDC
void ledc_breathing_led(void);          // 呼吸灯效果 (正弦波)
void led_set_duty(uint32_t duty);       // 设置占空比 (0-255)
void led_blink_500ms(void);             // 500ms 周期闪烁
```

**呼吸灯实现**:
- 使用正弦函数 `sin(phase)` 生成平滑亮度变化
- 相位每次递增 0.05，完整周期约 125 次调用
- 占空比范围: 0 ~ 255

**闪烁实现**:
- 静态计数器，每 25 次调用 (500ms) 切换一次状态
- 亮: 设置占空比为 0 (低电平有效)
- 灭: 调用 `ledc_stop()` 强制输出高电平

### 4.3 BUTTON 模块 (components/BSP/BUTTON/)

**硬件配置**:
| 参数 | 值 |
|------|-----|
| GPIO 引脚 | GPIO_NUM_0 |
| 模式 | GPIO_MODE_INPUT |
| 上拉 | 启用 (GPIO_PULLUP_ENABLE) |
| 中断 | 禁用 (轮询方式) |

**API 接口**:
```c
void button_init(void);                           // 初始化 GPIO
uint8_t button_get_state(void);                   // 获取按键状态
void button_scan(void);                           // 扫描按键 (检测边沿)
void button_register_callback(button_callback_t); // 注册回调函数
```

**按键检测逻辑**:
- 低电平有效 (按下 = 低电平)
- 检测上升沿 (RELEASED → PRESSED) 触发回调
- 无软件消抖，依赖主循环 20ms 周期

---

## 5. 数据流与控制流

```
                    ┌──────────┐
                    │  上电启动 │
                    └────┬─────┘
                         │
                    ┌────▼─────┐
                    │ app_main │
                    └────┬─────┘
                         │
          ┌──────────────┼──────────────┐
          │              │              │
     ┌────▼────┐   ┌─────▼─────┐  ┌────▼────┐
     │NVS Init │   │LED Init   │  │BTN Init │
     └─────────┘   └───────────┘  └────┬────┘
                                       │
                              ┌────────▼────────┐
                              │ Register Callback│
                              └────────┬────────┘
                                       │
                              ┌────────▼────────┐
                              │   Main Loop     │
                              │   (50Hz)        │
                              └────────┬────────┘
                                       │
                    ┌──────────────────┼──────────────────┐
                    │                  │                  │
              ┌─────▼─────┐     ┌──────▼──────┐    ┌──────▼──────┐
              │button_scan│     │breathing mode│    │ blink mode  │
              └─────┬─────┘     └──────┬──────┘    └──────┬──────┘
                    │                  │                  │
              ┌─────▼─────┐     ┌──────▼──────┐    ┌──────▼──────┐
              │callback?  │     │sin wave duty│    │toggle 500ms │
              └───────────┘     └─────────────┘    └─────────────┘
```

---

## 6. 依赖关系

```
main
├── freertos (task, delay)
├── nvs_flash
├── BSP
│   ├── driver (gpio, ledc)
│   └── math (sin function)
└── esp_system (error handling)
```

---

## 7. 关键设计特点

| 特点 | 说明 |
|------|------|
| **模块化设计** | LED 和 BUTTON 独立封装为 BSP 组件 |
| **回调机制** | 按键模块使用函数指针回调，解耦业务逻辑 |
| **轮询方式** | 按键采用轮询扫描，非中断驱动 |
| **PWM 控制** | 使用 ESP32 硬件 LEDC 实现 PWM |
| **正弦波呼吸** | 使用数学正弦函数实现平滑亮度渐变 |
| **FreeRTOS 延时** | 使用 `vTaskDelay()` 实现非阻塞延时 |

---

## 8. 构建与烧录

```bash
# 配置项目
idf.py set-target esp32s3

# 构建项目
idf.py build

# 烧录到设备
idf.py -p /dev/ttyUSB0 flash

# 监视串口输出
idf.py -p /dev/ttyUSB0 monitor
```

---

## 9. 未使用组件

- **express 组件** (`components/express/`): 存在于项目中但未被 main 或其他组件引用，可能是预留功能。
