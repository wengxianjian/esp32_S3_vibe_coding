/**
 ******************************************************************************
 * @file     main.c
 * @author   AI Assistant
 * @version  V1.0
 * @date     2023-08-26
 * @brief    LED灯实验
 ******************************************************************************
 * @attention
 *
 ******************************************************************************
 *
 * 命令接口设计说明
 * ================
 *
 * 本模块所有功能操作统一通过 uart_cmd_execute() 接口完成, 便于:
 *   - 串口远程调用 (UART RX 自动触发)
 *   - 代码内部调用 (按键回调、定时任务等)
 *   - 单元测试 (直接调用 uart_cmd_execute 验证行为)
 *
 * 命令格式:
 *   led <mode>
 *
 * 参数说明:
 *   <mode>  - LED 灯效模式, 可选值:
 *               "blink"     : 闪烁模式 (500ms 周期)
 *               "breathing" : 呼吸灯模式 (正弦波平滑过渡)
 *
 * 调用示例:
 *   uart_cmd_execute("led blink");       // 切换为闪烁模式
 *   uart_cmd_execute("led breathing");   // 切换为呼吸灯模式
 *
 * 扩展方式:
 *   新增灯效只需:
 *   1. 在 cmd_led() 中增加对应的 mode 分支
 *   2. 在 led.h/led.c 中实现新的灯效函数
 *   无需修改命令注册或按键回调逻辑。
 *
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
 * @brief       串口命令: "led <mode>" 统一处理函数
 * @note        根据参数切换 LED 灯效模式
 * @param       argc: 参数个数 (含命令名本身)
 * @param       argv: 参数数组
 *                  argv[0] = "led"         (命令名)
 *                  argv[1] = "blink" | "breathing" (灯效模式)
 * @retval      无
 *
 * @example     uart_cmd_execute("led blink");
 *              -> cmd_led(2, {"led", "blink"})
 *              -> led_set_mode(LED_MODE_BLINK)
 */
static void cmd_led(int argc, char *argv[])
{
    /* 参数校验: 至少需要 命令名 + 模式 两个参数 */
    if (argc < 2) {
        printf("[CMD] Usage: led <blink|breathing>\n");
        return;
    }

    const char *mode_str = argv[1];

    if (strcmp(mode_str, "blink") == 0) {
        led_set_mode(LED_MODE_BLINK);
        printf("[CMD] LED mode -> blink\n");
    } else if (strcmp(mode_str, "breathing") == 0) {
        led_set_mode(LED_MODE_BREATHING);
        printf("[CMD] LED mode -> breathing\n");
    } else {
        printf("[CMD] Unknown mode: '%s'\n", mode_str);
        printf("[CMD] Available modes: blink, breathing\n");
    }
}

/* 按键事件标志: 回调中设置, 主循环中消费 */
static volatile bool s_boot_pressed = false;

/**
 * @brief       BOOT 按键按下回调函数
 * @note        回调运行在高优先级任务(pri=10)中, 不可执行 printf 等阻塞操作,
 *              仅设置标志位, 由主循环(pri=1)调用 uart_cmd_execute 完成实际切换
 * @param       id: 触发的按键编号 (此处固定为 BUTTON_BOOT)
 * @retval      无
 */
void boot_button_callback(button_id_t id)
{
    s_boot_pressed = true;
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

    /*
     * 注册 "led" 命令
     * 命令格式: led <mode>
     *   mode: blink | breathing
     * 示例:
     *   串口输入 "led blink"     -> 切换为闪烁模式
     *   串口输入 "led breathing" -> 切换为呼吸灯模式
     *   代码调用 uart_cmd_execute("led blink") -> 同上
     */
    uart_cmd_register("led", cmd_led, "Control LED: led <blink|breathing>");

    printf("[INFO] UART command module initialized\n");
    printf("[INFO] System started, press BOOT to toggle LED mode\n");
    printf("[INFO] Send 'help' via UART to see available commands\n");

    while(1)
    {
        /* 检查按键事件: 主循环中执行 uart_cmd_execute, 避免高优先级回调阻塞 */
        if (s_boot_pressed) {
            s_boot_pressed = false;
            if (led_get_mode() == LED_MODE_BREATHING) {
                uart_cmd_execute("led blink");
            } else {
                uart_cmd_execute("led breathing");
            }
        }

        /* LED 灯效 */
        if (led_get_mode() == LED_MODE_BREATHING) {
            ledc_breathing_led();   /* 呼吸灯效果 */
        } else {
            led_blink_500ms();      /* LED 500ms闪烁 */
        }
        vTaskDelay(20);   /* 延时20ms */
    }
}
