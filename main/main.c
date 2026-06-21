/**
 ******************************************************************************
 * @file     main.c
 * @author   正点原子团队(ALIENTEK)
 * @version  V1.0
 * @date     2023-08-26
 * @brief    LED灯实验
 * @license  Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ******************************************************************************
 * @attention
 * 
 * 实验平台:正点原子 ESP32-S3 开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 ******************************************************************************
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "led.h"
#include "button.h"

static bool led_mode_breathing = true;  /* LED模式标志: true-呼吸灯, false-闪烁 */

/**
 * @brief       按键按下回调函数
 * @param       无
 * @retval      无
 */
void button_pressed_callback(void)
{
    led_mode_breathing = !led_mode_breathing;  /* 切换LED模式 */
}

/**
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void)
{
    esp_err_t ret;
    
    printf("\n=============================\n");
    printf("  ESP32 LED Vibe Coding\n");
    printf("  Serial Port Ready!\n");
    printf("=============================\n");
    
    ret = nvs_flash_init(); /* 初始化NVS */
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        printf("[WARN] NVS erase required\n");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    printf("[INFO] NVS init: %s\n", ret == ESP_OK ? "OK" : "FAIL");

    led_init();             /* 初始化LED */
    printf("[INFO] LED initialized\n");
    
    button_init();          /* 初始化按键 */
    button_register_callback(button_pressed_callback);  /* 注册按键回调 */
    printf("[INFO] Button initialized\n");
    printf("[INFO] System started, press button to toggle LED mode\n");

    while(1)
    {
        button_scan();      /* 扫描按键 */
        
        if (led_mode_breathing) {
            ledc_breathing_led();   /* 呼吸灯效果 */
        } else {
            led_blink_500ms();      /* LED 500ms闪烁 */
        }
        vTaskDelay(20);   /* 延时20ms */
    }
}
