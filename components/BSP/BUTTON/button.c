/**
 ****************************************************************************************************
 * @file        button.c
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

#include "button.h"

static button_callback_t button_callback = NULL;  /* 按键回调函数指针 */
static uint8_t last_state = BUTTON_RELEASED;      /* 上次按键状态 */

/**
 * @brief       初始化按键
 * @param       无
 * @retval      无
 */
void button_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO_PIN),  /* 配置GPIO引脚 */
        .mode = GPIO_MODE_INPUT,                    /* 设置为输入模式 */
        .pull_up_en = GPIO_PULLUP_ENABLE,           /* 启用内部上拉电阻 */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,      /* 禁用内部下拉电阻 */
        .intr_type = GPIO_INTR_DISABLE              /* 禁用中断 */
    };
    gpio_config(&io_conf);  /* 应用GPIO配置 */
}

/**
 * @brief       获取按键状态
 * @param       无
 * @retval      按键状态: BUTTON_PRESSED(按下) 或 BUTTON_RELEASED(释放)
 */
uint8_t button_get_state(void)
{
    /* 按键按下时引脚为低电平,释放时为高电平 */
    return gpio_get_level(BUTTON_GPIO_PIN) == 0 ? BUTTON_PRESSED : BUTTON_RELEASED;
}

/**
 * @brief       扫描按键状态
 * @param       无
 * @retval      无
 */
void button_scan(void)
{
    uint8_t current_state = button_get_state();  /* 获取当前按键状态 */
    
    /* 检测按键按下事件(从释放变为按下) */
    if (current_state == BUTTON_PRESSED && last_state == BUTTON_RELEASED) {
        if (button_callback != NULL) {
            button_callback();  /* 执行回调函数 */
        }
    }
    
    last_state = current_state;  /* 更新上次状态 */
}

/**
 * @brief       注册按键回调函数
 * @param       callback: 回调函数指针
 * @retval      无
 */
void button_register_callback(button_callback_t callback)
{
    button_callback = callback;  /* 保存回调函数指针 */
}
