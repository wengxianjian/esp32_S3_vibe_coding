/**
 ****************************************************************************************************
 * @file        button.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       按键驱动代码
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
#define BUTTON_GPIO_PIN    GPIO_NUM_0  /* 按键连接的GPIO端口 */

/* 按键状态 */
enum BUTTON_STATE
{
    BUTTON_RELEASED,  /* 按键释放 */
    BUTTON_PRESSED    /* 按键按下 */
};

/* 按键回调函数类型 */
typedef void (*button_callback_t)(void);

/* 函数声明 */
void button_init(void);                           /* 初始化按键 */
uint8_t button_get_state(void);                   /* 获取按键状态 */
void button_scan(void);                           /* 扫描按键 */
void button_register_callback(button_callback_t callback);  /* 注册按键回调函数 */

#endif
