/**
 ****************************************************************************************************
 * @file        led.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       LED驱动代码
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

#include "led.h"
#include "math.h"

/* 当前LED模式 */
static led_mode_t current_led_mode = LED_MODE_BREATHING;

/**
 * @brief       初始化LEDC (LED PWM控制器)
 * @param       无
 * @retval      无
 */
static void ledc_init(void)
{
    /* 配置LEDC定时器 */
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,          /* 速度模式 */
        .timer_num        = LEDC_TIMER,         /* 定时器编号 */
        .duty_resolution  = LEDC_DUTY_RES,      /* 占空比分辨率 */
        .freq_hz          = LEDC_FREQUENCY,     /* PWM频率 */
        .clk_cfg          = LEDC_AUTO_CLK       /* 自动选择时钟源 */
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));  /* 应用定时器配置 */

    /* 配置LEDC通道 */
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,            /* 速度模式 */
        .channel        = LEDC_CHANNEL,         /* 通道编号 */
        .timer_sel      = LEDC_TIMER,           /* 选择定时器 */
        .intr_type      = LEDC_INTR_DISABLE,    /* 禁用中断 */
        .gpio_num       = LED_GPIO_PIN,         /* GPIO引脚 */
        .duty           = 255,                  /* 初始占空比 */
        .hpoint         = 0                     /* 高电平点 */
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));  /* 应用通道配置 */
}

/**
 * @brief       设置PWM占空比
 * @param       duty: 占空比值 (0-255)
 * @retval      无
 */
void led_set_duty(uint32_t duty)
{
    if (duty > 255) {
        duty = 255;  /* 限制最大值为255 */
    }
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty));   /* 设置占空比 */
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));      /* 更新占空比 */
}

/**
 * @brief       LED 500ms闪烁模式
 * @param       无
 * @retval      无
 */
void led_blink_500ms(void)
{
    static uint8_t blink_count = 0;  /* 闪烁计数器 */
    static uint8_t led_state = 0;    /* LED状态: 0-灭, 1-亮 */
    
    blink_count++;
    if (blink_count >= 25) {  /* 25 * 20ms = 500ms */
        blink_count = 0;
        led_state = !led_state;  /* 切换LED状态 */
        
        if (led_state) {
            /* 亮: 启动PWM输出,占空比0(低电平有效) */
            ledc_channel_config_t ledc_channel = {
                .speed_mode     = LEDC_MODE,
                .channel        = LEDC_CHANNEL,
                .timer_sel      = LEDC_TIMER,
                .intr_type      = LEDC_INTR_DISABLE,
                .gpio_num       = LED_GPIO_PIN,
                .duty           = 0,
                .hpoint         = 0
            };
            ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
        } else {
            /* 灭: 停止PWM输出,强制输出高电平 */
            ESP_ERROR_CHECK(ledc_stop(LEDC_MODE, LEDC_CHANNEL, 1));
        }
    }
}

/**
 * @brief       呼吸灯效果
 * @param       无
 * @retval      无
 */
void ledc_breathing_led(void)
{
    static float phase = 0;  /* 相位变量 */
    uint32_t duty;           /* 占空比 */
    
    phase += 0.05;  /* 相位递增 */
    if (phase > 2 * 3.14159) {
        phase = 0;  /* 重置相位 */
    }
    
    /* 使用正弦函数生成平滑的占空比变化 (0-255) */
    duty = (uint32_t)((sin(phase) + 1) * 127.5);
    led_set_duty(duty);  /* 设置占空比 */
}

/**
 * @brief       初始化LED
 * @param       无
 * @retval      无
 */
void led_init(void)
{
    ledc_init();  /* 初始化LEDC */
}

/**
 * @brief       设置LED模式
 * @param       mode: LED模式 (LED_MODE_BREATHING 或 LED_MODE_BLINK)
 * @retval      无
 */
void led_set_mode(led_mode_t mode)
{
    if (mode < LED_MODE_MAX) {
        current_led_mode = mode;
        printf("[LED] Mode set to: %s\n",
               mode == LED_MODE_BREATHING ? "breathing" : "blink");
    }
}

/**
 * @brief       获取当前LED模式
 * @param       无
 * @retval      当前LED模式
 */
led_mode_t led_get_mode(void)
{
    return current_led_mode;
}
