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
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "led.h"
#include "button.h"
#include "uart_cmd.h"

/**
 * @brief       串口命令: "led blink" 处理函数
 * @note        切换LED为闪烁模式, 可通过 uart_cmd_register 注册更多命令
 * @param       argc: 参数个数
 * @param       argv: 参数数组
 * @retval      无
 */
static void cmd_led_blink(int argc, char *argv[])
{
    led_set_mode(LED_MODE_BLINK);
    printf("[CMD] LED mode switched to blink via UART command\n");
}

/**
 * @brief       串口命令: "led breathing" 处理函数
 * @param       argc: 参数个数
 * @param       argv: 参数数组
 * @retval      无
 */
static void cmd_led_breathing(int argc, char *argv[])
{
    led_set_mode(LED_MODE_BREATHING);
    printf("[CMD] LED mode switched to breathing via UART command\n");
}

/**
 * @brief       BOOT 按键按下回调函数
 * @param       id: 触发的按键编号 (此处固定为 BUTTON_BOOT)
 * @retval      无
 */
void boot_button_callback(button_id_t id)
{
    /* 切换LED模式 */
    led_mode_t new_mode = (led_get_mode() == LED_MODE_BREATHING) ?
                           LED_MODE_BLINK : LED_MODE_BREATHING;
    led_set_mode(new_mode);
    printf("[BTN] BOOT -> LED mode: %s\n",
           new_mode == LED_MODE_BREATHING ? "breathing" : "blink");
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
    
    printf("==wx add,hello world!======\n");

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
    
    button_init();          /* 初始化按键 (GPTimer 硬件定时器轮询5个按键) */
    button_register_callback(BUTTON_BOOT, boot_button_callback);  /* 注册 BOOT 按键回调: 切换灯效 */
    printf("[INFO] Button initialized (GPTimer polling, %d buttons)\n", BUTTON_NUM);

    /* 初始化UART命令模块 */
    uart_cmd_init();
    /* 注册串口命令 */
    uart_cmd_register("led blink", cmd_led_blink, "Switch LED to blink mode");
    uart_cmd_register("led breathing", cmd_led_breathing, "Switch LED to breathing mode");
    printf("[INFO] UART command module initialized\n");
    printf("[INFO] System started, press BOOT to toggle LED mode\n");
    printf("[INFO] Send 'help' via UART to see available commands\n");

    // uart cmd test fun
    //uart_cmd_execute("led blink");
    //uart_cmd_execute("led breathing");
    
    while(1)
    {
        /* 按键扫描已由 GPTimer 定时器任务完成, 主循环只负责 LED 灯效 */
        if (led_get_mode() == LED_MODE_BREATHING) {
            ledc_breathing_led();   /* 呼吸灯效果 */
        } else {
            led_blink_500ms();      /* LED 500ms闪烁 */
        }
        vTaskDelay(20);   /* 延时20ms */
    }
}
