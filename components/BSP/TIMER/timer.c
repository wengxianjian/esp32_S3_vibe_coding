/**
 ****************************************************************************************************
 * @file        timer.c
 * @author      AI Assistant
 * @version     V1.0
 * @date        2026-07-28
 * @brief       高分辨率定时器驱动代码
 ****************************************************************************************************
 * @attention
 *
 ****************************************************************************************************
 */

#include "timer.h"
#include <stdio.h>

static esp_timer_handle_t s_timer_handle = NULL;  /* 定时器句柄 */

/**
 * @brief       定时器中断回调函数
 * @note        由 esp_timer 在定时到达时调用，打印提示信息
 * @param       arg: 回调参数（未使用）
 * @retval      无
 */
static void timer_callback(void *arg)
{
    printf("timer arrived!\n");
}

/**
 * @brief       初始化高分辨率定时器
 * @note        创建并启动周期定时器
 * @param       period_us: 定时器周期，单位微秒（如 1 秒 = 1000000）
 * @retval      无
 */
void timer_init(uint64_t period_us)
{
    /* 配置 esp_timer 参数，以微秒为单位 */
    const esp_timer_create_args_t timer_args = {
        .callback = &timer_callback,    /* 回调函数 */
        .name     = "high_res_timer"    /* 定时器名称（调试用） */
    };

    /* 创建定时器 */
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &s_timer_handle));

    /* 启动周期定时器，周期由参数指定 */
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_timer_handle, period_us));
}
