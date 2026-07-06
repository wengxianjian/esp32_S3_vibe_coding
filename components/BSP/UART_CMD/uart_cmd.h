/**
 ******************************************************************************
 * @file        uart_cmd.h
 * @author      AI Assistant
 * @version     V1.0
 * @date        2026-07-06
 * @brief       UART命令接收与解析模块
 * @license     Copyright (c) 2020-2032
 ******************************************************************************
 * @attention
 *
 * 功能说明:
 *   - 独立任务监听UART RX数据
 *   - 支持命令注册机制，便于扩展新命令
 *   - 命令格式：以换行符(\n或\r\n)结尾的字符串
 *
 ******************************************************************************
 */

#ifndef __UART_CMD_H_
#define __UART_CMD_H_

#include <stdint.h>
#include <stdbool.h>

/* UART配置 */
#define UART_CMD_PORT_NUM       UART_NUM_0      /* 使用UART0（默认串口） */
#define UART_CMD_BAUD_RATE      115200          /* 波特率 */
#define UART_CMD_RX_BUF_SIZE    256             /* RX缓冲区大小(字节) */
#define UART_CMD_TASK_STACK_SIZE 2048           /* 命令处理任务栈大小 */
#define UART_CMD_TASK_PRIORITY  5               /* 任务优先级（低于按键任务） */
#define UART_CMD_MAX_CMD_LEN    64              /* 单条命令最大长度 */
#define UART_CMD_MAX_ARGS       8               /* 单条命令最大参数数量 */

/* 命令处理函数类型 */
typedef void (*uart_cmd_handler_t)(int argc, char *argv[]);

/* 命令注册表项 */
typedef struct {
    const char *cmd_name;           /* 命令名称，如 "led blink" */
    uart_cmd_handler_t handler;     /* 命令处理函数 */
    const char *help;               /* 命令帮助信息 */
} uart_cmd_entry_t;

/**
 * @brief       初始化UART命令模块
 * @note        配置UART参数并启动命令接收任务
 * @param       无
 * @retval      无
 */
void uart_cmd_init(void);

/**
 * @brief       注册命令处理函数
 * @param       cmd_name: 命令名称（如 "led blink"）
 * @param       handler: 命令处理函数指针
 * @param       help: 命令帮助信息（可为NULL）
 * @retval      0: 成功, -1: 失败（命令已存在或注册表满）
 */
int uart_cmd_register(const char *cmd_name, uart_cmd_handler_t handler, const char *help);

/**
 * @brief       注销命令
 * @param       cmd_name: 要注销的命令名称
 * @retval      0: 成功, -1: 失败（命令不存在）
 */
int uart_cmd_unregister(const char *cmd_name);

/**
 * @brief       打印所有已注册命令的帮助信息
 * @param       无
 * @retval      无
 */
void uart_cmd_print_help(void);

#endif /* __UART_CMD_H_ */
