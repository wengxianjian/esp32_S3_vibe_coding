/**
 ****************************************************************************************************
 * @file        led.h
 * @author      AI Assistant
 * @version     V1.0
 * @date        2023-08-26
 * @brief       LED驱动代码
 ****************************************************************************************************
 * @attention
 *
 ****************************************************************************************************
 */

#ifndef __LED_H_
#define __LED_H_

#include "driver/gpio.h"
#include "driver/ledc.h"


/* 引脚定义 */
#define LED_GPIO_PIN    GPIO_NUM_1  /* LED连接的GPIO端口 */

/* 引脚的输出的电平状态 */
enum GPIO_OUTPUT_STATE
{
    PIN_RESET,
    PIN_SET
};

/* LED端口定义 */
#define LED(x)          do { x ?                                      \
                             gpio_set_level(LED_GPIO_PIN, PIN_SET) :  \
                             gpio_set_level(LED_GPIO_PIN, PIN_RESET); \
                        } while(0)  /* LED翻转 */

/* LED取反定义 */
#define LED_TOGGLE()    do { gpio_set_level(LED_GPIO_PIN, !gpio_get_level(LED_GPIO_PIN)); } while(0)  /* LED翻转 */

/* LEDC配置 */
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT
#define LEDC_FREQUENCY          1000  /* PWM频率 1kHz */

/* LED模式枚举 */
typedef enum {
    LED_MODE_BREATHING = 0,     /* 呼吸灯模式 */
    LED_MODE_BLINK,             /* 闪烁模式 */
    LED_MODE_MAX
} led_mode_t;

/* 函数声明 */
void led_init(void);                    /* 初始化LED */
void ledc_breathing_led(void);          /* 呼吸灯效果 */
void led_set_duty(uint32_t duty);       /* 设置占空比 */
void led_blink_500ms(void);             /* LED 500ms闪烁 */
void led_set_mode(led_mode_t mode);     /* 设置LED模式 */
led_mode_t led_get_mode(void);          /* 获取当前LED模式 */

#endif
