# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目简介

基于 **ESP-IDF** 框架的 ESP32-S3 嵌入式应用（正点原子 ATK-DNESP32S3 开发板）。实现 LED 呼吸灯 / 闪烁两种效果，通过按键切换模式。CMake 项目名为 `01_led`。

## 常用命令

```bash
# 首次配置目标芯片（已在 sdkconfig 中固定为 esp32s3）
idf.py set-target esp32s3

# 构建
idf.py build

# 烧录 + 串口监视（macOS 上串口通常为 /dev/cu.usbserial-* 或 /dev/cu.SLAB_USBtoUART）
idf.py -p <PORT> flash monitor

# 仅监视串口（退出 monitor：Ctrl+])
idf.py -p <PORT> monitor

# 打开 menuconfig 配置
idf.py menuconfig

# 清理构建产物
idf.py fullclean
```

注意：本仓库无单元测试框架，验证靠真机烧录后观察串口输出与 LED 行为。使用前需先 `. $IDF_PATH/export.sh` 激活 ESP-IDF 环境。

## 架构

三层结构，详见 `ARCHITECTURE.md`：

- **应用层** `main/main.c` — `app_main()` 初始化 NVS、LED、BUTTON 并注册按键回调，进入 50Hz 主循环（`vTaskDelay(20)`）。模式由静态标志 `led_mode_breathing` 控制，按键回调翻转它。
- **BSP 层** `components/BSP/` — 板级驱动，独立封装为 LED 和 BUTTON 两个模块，注册为单个 `BSP` 组件（见 `components/BSP/CMakeLists.txt`，`REQUIRES driver`）。
- **驱动层** — ESP-IDF 的 `driver/gpio`、`driver/ledc`、`nvs_flash`、`freertos`。

### 关键设计约定

- **轮询而非中断**：按键无硬件中断、无软件消抖，靠 `button_scan()` 每 20ms 检测下降沿（`RELEASED→PRESSED`）触发回调。新增定时逻辑通常以"主循环调用次数 × 20ms"计数实现（如 `led_blink_500ms()` 用 `25 次 = 500ms`）。
- **回调解耦**：BUTTON 模块通过函数指针 `button_register_callback()` 与业务逻辑解耦，不直接引用 LED。
- **LED 低电平有效**：占空比 0 = 最亮，`ledc_stop(..., 1)` = 强制高电平熄灭。呼吸灯用 `sin()` 生成平滑占空比。
- **硬件引脚**：LED = GPIO1（LEDC TIMER_0 / CHANNEL_0 / 8-bit / 1kHz，定义在 `led.h`）；BUTTON = GPIO0（输入 + 上拉，定义在 `button.h`）。

### 添加新 BSP 模块

在 `components/BSP/` 下新建子目录（如 `XXX/`），然后在 `components/BSP/CMakeLists.txt` 的 `src_dirs` 和 `include_dirs` 列表中加入该目录名即可，无需新建 CMakeLists.txt。

### 注意事项

- `components/express/` 是预留组件，仅含空函数 `func()`，未被 main 引用——修改 LED/按键功能时无需关注。
- 分区表为自定义 `partitions-16MiB.csv`（16MB flash：factory app 2MB + fat 10MB + spiffs 4MB）。
- `BSP` 组件用 `-ffast-math -O3` 编译（见其 CMakeLists.txt），改动数学相关代码时留意浮点精度。
- 硬件文档（数据手册、引脚分配表）在 `硬件资料/` 目录。
