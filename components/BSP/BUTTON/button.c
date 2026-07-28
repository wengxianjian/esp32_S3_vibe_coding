/**
 ****************************************************************************************************
 * @file        button.c
 * @author      AI Assistant
 * @version     V2.0
 * @date        2026-06-22
 * @brief       按键驱动代码 (GPTimer 硬件定时器轮询, 支持5个按键)
 ****************************************************************************************************
 * @attention
 *
 * 设计说明:
 *   - 使用 GPTimer 硬件定时器, 每 BUTTON_SCAN_PERIOD_MS(20ms) 产生一次中断;
 *   - 中断回调中只通过任务通知唤醒扫描任务(中断里不可调用 printf 等非 ISR-safe 函数);
 *   - 扫描任务 button_task() 调用 button_scan() 完成边沿检测、信息打印与回调分发;
 *   - BOOT 按键直连 IO0, 可真实读取; KEY0~KEY3 挂在 XL9555 I2C 扩展芯片, 暂留占位。
 *
 ****************************************************************************************************
 */

#include <stdio.h>
#include "button.h"
#include "driver/gptimer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 各按键名称, 用于打印信息 */
static const char *button_names[BUTTON_NUM] = {"BOOT", "KEY0", "KEY1", "KEY2", "KEY3"};

static button_callback_t button_callbacks[BUTTON_NUM] = {0};            /* 各按键回调函数指针 */
static button_state_t last_state[BUTTON_NUM];                          /* 各按键上次状态 */

static gptimer_handle_t button_gptimer = NULL;                         /* GPTimer 句柄 */
static TaskHandle_t button_task_handle = NULL;                         /* 扫描任务句柄 */

/**
 * @brief       获取指定按键的当前状态
 * @param       id: 按键编号 (button_id_t)
 * @retval      BUTTON_PRESSED(按下) 或 BUTTON_RELEASED(释放)
 */
button_state_t button_get_state(button_id_t id)
{
    switch (id)
    {
        case BUTTON_BOOT:
            /* BOOT 直连 IO0, 低电平有效(按下=低电平) */
            return gpio_get_level(BOOT_GPIO_PIN) == 0 ? BUTTON_PRESSED : BUTTON_RELEASED;

        case BUTTON_KEY0:
        case BUTTON_KEY1:
        case BUTTON_KEY2:
        case BUTTON_KEY3:
            /* TODO: KEY0~KEY3 挂在 XL9555 I2C 扩展芯片上,
             *       待实现 XL9555 驱动后, 在此通过 I2C 读取对应引脚电平 */
            return BUTTON_RELEASED;

        default:
            return BUTTON_RELEASED;
    }
}

/**
 * @brief       扫描所有按键 (检测按下边沿)
 * @note        由扫描任务周期性调用; 检测到"释放->按下"时打印信息并执行回调
 * @param       无
 * @retval      无
 */
static void button_scan(void)
{
    for (button_id_t id = 0; id < BUTTON_NUM; id++)
    {
        button_state_t current = button_get_state(id);

        /* 检测按下事件(从释放变为按下) */
        if (current == BUTTON_PRESSED && last_state[id] == BUTTON_RELEASED)
        {
            printf("[BTN] %s pressed\n", button_names[id]);  /* 先打印信息, 具体处理由回调完成 */

            if (button_callbacks[id] != NULL)
            {
                button_callbacks[id](id);  /* 执行该按键的回调函数 */
            }
        }

        last_state[id] = current;  /* 更新该按键的上次状态 */
    }
}

/**
 * @brief       GPTimer 报警中断回调 (运行在 ISR 上下文)
 * @note        中断里不做耗时/非 ISR-safe 操作, 仅通知扫描任务执行扫描
 * @param       timer: GPTimer 句柄
 * @param       edata: 报警事件数据
 * @param       user_ctx: 用户参数(未使用)
 * @retval      是否需要唤醒高优先级任务
 */
static bool IRAM_ATTR button_timer_isr_callback(gptimer_handle_t timer,
                                                const gptimer_alarm_event_data_t *edata,
                                                void *user_ctx)
{
    BaseType_t high_task_awoken = pdFALSE;
    vTaskNotifyGiveFromISR(button_task_handle, &high_task_awoken);  /* 唤醒扫描任务 */
    return high_task_awoken == pdTRUE;  /* 若唤醒了更高优先级任务, 请求切换 */
}

/**
 * @brief       按键扫描任务
 * @param       arg: 任务参数(未使用)
 * @retval      无
 */
static void button_task(void *arg)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  /* 等待 GPTimer 中断通知 */
        button_scan();                            /* 执行一次按键扫描 */
    }
}

/**
 * @brief       初始化按键 GPIO
 * @param       无
 * @retval      无
 */
static void button_gpio_init(void)
{
    /* BOOT 按键: IO0 输入 + 上拉, 无中断(采用定时器轮询) */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOOT_GPIO_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    /* TODO: 此处后续可初始化 XL9555 (I2C) 以读取 KEY0~KEY3 */
}

/**
 * @brief       初始化 GPTimer 硬件定时器并启动周期轮询
 * @param       无
 * @retval      无
 */
static void button_gptimer_init(void)
{
    /* 1. 创建定时器: 1MHz 分辨率 (1 tick = 1us) */
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,  /* 1MHz, 1 tick = 1us */
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &button_gptimer));

    /* 2. 注册报警回调 */
    gptimer_event_callbacks_t cbs = {
        .on_alarm = button_timer_isr_callback,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(button_gptimer, &cbs, NULL));

    /* 3. 使能定时器 */
    ESP_ERROR_CHECK(gptimer_enable(button_gptimer));

    /* 4. 配置报警: 每 BUTTON_SCAN_PERIOD_MS 触发一次, 自动重载 */
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = BUTTON_SCAN_PERIOD_MS * 1000,  /* ms -> us */
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(button_gptimer, &alarm_config));

    /* 5. 启动定时器 */
    ESP_ERROR_CHECK(gptimer_start(button_gptimer));
}

/**
 * @brief       初始化按键 (GPIO + 扫描任务 + GPTimer 轮询)
 * @param       无
 * @retval      无
 */
void button_init(void)
{
    /* 初始化各按键状态 */
    for (button_id_t id = 0; id < BUTTON_NUM; id++)
    {
        last_state[id] = BUTTON_RELEASED;
        button_callbacks[id] = NULL;
    }

    button_gpio_init();  /* 初始化按键 GPIO */

    /* 创建按键扫描任务 (优先级需高于普通任务, 以便及时响应定时器通知) */
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, &button_task_handle);

    button_gptimer_init();  /* 启动 GPTimer 周期轮询 */
}

/**
 * @brief       为指定按键注册回调函数
 * @param       id: 按键编号 (button_id_t)
 * @param       cb: 回调函数指针
 * @retval      无
 */
void button_register_callback(button_id_t id, button_callback_t cb)
{
    if (id < BUTTON_NUM)
    {
        button_callbacks[id] = cb;  /* 保存该按键的回调函数指针 */
    }
}
