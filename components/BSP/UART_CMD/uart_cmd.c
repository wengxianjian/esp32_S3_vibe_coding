/**
 ******************************************************************************
 * @file        uart_cmd.c
 * @author      AI Assistant
 * @version     V1.0
 * @date        2026-07-06
 * @brief       UART命令接收与解析模块实现
 * @license     Copyright (c) 2020-2032
 ******************************************************************************
 * @attention
 *
 * 实现说明:
 *   - 使用独立FreeRTOS任务持续监听UART RX
 *   - 按行接收命令字符串（以\n或\r\n结尾）
 *   - 支持命令注册表，动态添加/删除命令
 *   - 命令解析：按空格分割命令和参数
 *
 ******************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uart_cmd.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 命令注册表 */
#define UART_CMD_MAX_ENTRIES    16              /* 最大命令数量 */
static uart_cmd_entry_t cmd_table[UART_CMD_MAX_ENTRIES];
static int cmd_count = 0;

/* 任务句柄 */
static TaskHandle_t uart_cmd_task_handle = NULL;

/* 接收缓冲区 */
static char rx_buffer[UART_CMD_RX_BUF_SIZE];
static int rx_pos = 0;

/**
 * @brief       UART硬件初始化
 * @param       无
 * @retval      无
 */
static void uart_hw_init(void)
{
    /* UART配置 */
    uart_config_t uart_config = {
        .baud_rate = UART_CMD_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    /* 配置UART参数 */
    ESP_ERROR_CHECK(uart_param_config(UART_CMD_PORT_NUM, &uart_config));

    /* 安装UART驱动，启用RX和TX */
    ESP_ERROR_CHECK(uart_driver_install(
        UART_CMD_PORT_NUM,
        UART_CMD_RX_BUF_SIZE * 2,   /* RX缓冲区大小 */
        0,                           /* TX缓冲区大小（0=使用默认值） */
        0,                           /* 事件队列大小（0=不启用事件队列） */
        NULL,                        /* 事件队列指针 */
        0                            /* 中断标志 */
    ));

    printf("[UART] Initialized: baud=%d, RX buf=%d\n",
           UART_CMD_BAUD_RATE, UART_CMD_RX_BUF_SIZE);
}

/**
 * @brief       查找并执行命令
 * @param       cmd_str: 接收到的命令字符串
 * @retval      0: 成功执行, -1: 命令未找到
 */
static int execute_command(char *cmd_str)
{
    /* 去除首尾空白字符 */
    while (*cmd_str == ' ' || *cmd_str == '\t') {
        cmd_str++;
    }

    /* 去除末尾换行符 */
    int len = strlen(cmd_str);
    while (len > 0 && (cmd_str[len-1] == '\r' || cmd_str[len-1] == '\n' ||
                        cmd_str[len-1] == ' ' || cmd_str[len-1] == '\t')) {
        cmd_str[--len] = '\0';
    }

    /* 空命令直接返回 */
    if (len == 0) {
        return 0;
    }

    /* 打印接收到的命令 */
    printf("[RX Command] Received: %s\n", cmd_str);

    /* 特殊命令：help */
    if (strcmp(cmd_str, "help") == 0) {
        uart_cmd_print_help();
        return 0;
    }

    /* 在命令表中查找匹配的命令（支持多词命令名） */
    for (int i = 0; i < cmd_count; i++) {
        int cmd_len = strlen(cmd_table[i].cmd_name);
        
        /* 检查输入是否以命令名开头 */
        if (strncmp(cmd_str, cmd_table[i].cmd_name, cmd_len) == 0) {
            /* 命令名后必须是空格或字符串结束 */
            if (cmd_str[cmd_len] == '\0' || cmd_str[cmd_len] == ' ' || cmd_str[cmd_len] == '\t') {
                /* 找到匹配命令，解析参数 */
                int argc = 0;
                char *argv[UART_CMD_MAX_ARGS];
                
                /* 将命令名作为第一个参数 */
                argv[argc++] = (char *)cmd_table[i].cmd_name;
                
                /* 解析剩余部分作为参数 */
                if (cmd_str[cmd_len] != '\0') {
                    char *args_str = cmd_str + cmd_len;
                    char *token = strtok(args_str, " \t");
                    while (token != NULL && argc < UART_CMD_MAX_ARGS) {
                        argv[argc++] = token;
                        token = strtok(NULL, " \t");
                    }
                }
                
                /* 调用处理函数 */
                if (cmd_table[i].handler != NULL) {
                    cmd_table[i].handler(argc, argv);
                }
                return 0;
            }
        }
    }

    /* 未找到命令，提取第一个词用于错误提示 */
    char *first_word = strtok(cmd_str, " \t");
    printf("[UART] Unknown command: %s\n", first_word ? first_word : cmd_str);
    printf("[UART] Type 'help' for available commands\n");
    return -1;
}

/**
 * @brief       UART命令接收任务
 * @note        持续监听UART RX，按行接收并处理命令
 * @param       arg: 任务参数（未使用）
 * @retval      无
 */
static void uart_cmd_task(void *arg)
{
    uint8_t data;

    printf("[UART] Command task started\n");

    for (;;) {
        /* 从UART读取1字节数据 */
        int len = uart_read_bytes(UART_CMD_PORT_NUM, &data, 1, portMAX_DELAY);

        if (len > 0) {
            /* 处理接收到的数据 */
            if (data == '\n' || data == '\r') {
                /* 遇到换行符，处理完整命令 */
                if (rx_pos > 0) {
                    rx_buffer[rx_pos] = '\0';  /* 添加字符串结束符 */
                    execute_command(rx_buffer);  /* 执行命令 */
                    rx_pos = 0;                  /* 重置缓冲区位置 */
                }
            } else {
                /* 普通字符，存入缓冲区 */
                if (rx_pos < UART_CMD_MAX_CMD_LEN - 1) {
                    rx_buffer[rx_pos++] = data;
                } else {
                    /* 缓冲区溢出，丢弃当前命令 */
                    printf("[UART] Command too long, discarded\n");
                    rx_pos = 0;
                }
            }
        }
    }
}

/**
 * @brief       初始化UART命令模块
 * @param       无
 * @retval      无
 */
void uart_cmd_init(void)
{
    /* 初始化命令表 */
    memset(cmd_table, 0, sizeof(cmd_table));
    cmd_count = 0;

    /* 初始化UART硬件 */
    uart_hw_init();

    /* 创建命令接收任务 */
    xTaskCreate(
        uart_cmd_task,
        "uart_cmd",
        UART_CMD_TASK_STACK_SIZE,
        NULL,
        UART_CMD_TASK_PRIORITY,
        &uart_cmd_task_handle
    );

    printf("[UART] Command module initialized\n");
}

/**
 * @brief       注册命令处理函数
 * @param       cmd_name: 命令名称
 * @param       handler: 命令处理函数
 * @param       help: 命令帮助信息
 * @retval      0: 成功, -1: 失败
 */
int uart_cmd_register(const char *cmd_name, uart_cmd_handler_t handler, const char *help)
{
    if (cmd_name == NULL || handler == NULL) {
        return -1;
    }

    /* 检查命令是否已存在 */
    for (int i = 0; i < cmd_count; i++) {
        if (strcmp(cmd_table[i].cmd_name, cmd_name) == 0) {
            printf("[UART] Command already registered: %s\n", cmd_name);
            return -1;
        }
    }

    /* 检查注册表是否已满 */
    if (cmd_count >= UART_CMD_MAX_ENTRIES) {
        printf("[UART] Command table full\n");
        return -1;
    }

    /* 添加新命令 */
    cmd_table[cmd_count].cmd_name = cmd_name;
    cmd_table[cmd_count].handler = handler;
    cmd_table[cmd_count].help = help;
    cmd_count++;

    printf("[UART] Command registered: %s\n", cmd_name);
    return 0;
}

/**
 * @brief       注销命令
 * @param       cmd_name: 命令名称
 * @retval      0: 成功, -1: 失败
 */
int uart_cmd_unregister(const char *cmd_name)
{
    if (cmd_name == NULL) {
        return -1;
    }

    /* 查找命令 */
    for (int i = 0; i < cmd_count; i++) {
        if (strcmp(cmd_table[i].cmd_name, cmd_name) == 0) {
            /* 找到命令，将其从表中移除 */
            for (int j = i; j < cmd_count - 1; j++) {
                cmd_table[j] = cmd_table[j + 1];
            }
            cmd_count--;
            printf("[UART] Command unregistered: %s\n", cmd_name);
            return 0;
        }
    }

    printf("[UART] Command not found: %s\n", cmd_name);
    return -1;
}

/**
 * @brief       打印所有已注册命令的帮助信息
 * @param       无
 * @retval      无
 */
void uart_cmd_print_help(void)
{
    printf("\n");
    printf("=================================\n");
    printf("  Available Commands:\n");
    printf("=================================\n");

    for (int i = 0; i < cmd_count; i++) {
        printf("  %-20s", cmd_table[i].cmd_name);
        if (cmd_table[i].help != NULL) {
            printf("- %s", cmd_table[i].help);
        }
        printf("\n");
    }

    printf("  %-20s- Show this help message\n", "help");
    printf("=================================\n\n");
}

/**
 * @brief       从代码中直接执行命令（不经过UART RX）
 * @note        可在任意任务上下文中调用，命令会被复制到内部缓冲区后解析执行
 * @param       cmd_str: 命令字符串，如 "led blink"（无需换行符）
 * @retval      0: 成功执行, -1: 命令未找到, -2: 参数错误
 * @example     uart_cmd_execute("led blink");
 */
int uart_cmd_execute(const char *cmd_str)
{
    if (cmd_str == NULL) {
        printf("[UART] Error: NULL command string\n");
        return -2;
    }

    /* 检查命令长度 */
    int len = strlen(cmd_str);
    if (len == 0 || len >= UART_CMD_MAX_CMD_LEN) {
        printf("[UART] Error: Invalid command length\n");
        return -2;
    }

    /* 将命令复制到内部缓冲区（execute_command会修改字符串） */
    char cmd_buffer[UART_CMD_MAX_CMD_LEN];
    strncpy(cmd_buffer, cmd_str, UART_CMD_MAX_CMD_LEN - 1);
    cmd_buffer[UART_CMD_MAX_CMD_LEN - 1] = '\0';

    /* 执行命令 */
    return execute_command(cmd_buffer);
}
