/**
 ****************************************************************************************************
 * @file        button.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V2.0
 * @date        2026-06-22
 * @brief       按键驱动代码 (GPTimer 硬件定时器轮询, 支持5个按键)
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 ESP32-S3 开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#ifndef __BUTTON_H_
#define __BUTTON_H_

#include "driver/gpio.h"

/* 引脚定义 */
#define BOOT_GPIO_PIN           GPIO_NUM_0  /* BOOT 按键直连的 GPIO (低电平有效) */

/* 按键轮询周期 (ms): GPTimer 每隔该时间触发一次扫描 */
#define BUTTON_SCAN_PERIOD_MS   20

/* 按键编号 */
typedef enum
{
    BUTTON_BOOT = 0,    /* BOOT 按键, 直连 IO0 */
    BUTTON_KEY0,        /* KEY0, 挂在 XL9555 I2C 扩展芯片 (暂未实现) */
    BUTTON_KEY1,        /* KEY1, 挂在 XL9555 I2C 扩展芯片 (暂未实现) */
    BUTTON_KEY2,        /* KEY2, 挂在 XL9555 I2C 扩展芯片 (暂未实现) */
    BUTTON_KEY3,        /* KEY3, 挂在 XL9555 I2C 扩展芯片 (暂未实现) */
    BUTTON_NUM          /* 按键总数 (=5) */
} button_id_t;

/* 按键状态 */
typedef enum
{
    BUTTON_RELEASED = 0,    /* 按键释放 */
    BUTTON_PRESSED          /* 按键按下 */
} button_state_t;

/* 按键回调函数类型, 参数为触发的按键编号 */
typedef void (*button_callback_t)(button_id_t id);

/* 函数声明 */
void button_init(void);                                                 /* 初始化按键 + 启动 GPTimer 轮询 */
button_state_t button_get_state(button_id_t id);                        /* 获取指定按键的当前状态 */
void button_register_callback(button_id_t id, button_callback_t cb);    /* 为指定按键注册回调函数 */

#endif
