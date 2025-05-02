#ifndef __ESP01S_H__
#define __ESP01S_H__

#include "stm32f10x.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "device_list.h"

#define UART3_MAX_RECV_LEN 512
#define UART3_MAX_SEND_LEN 512

// AI返回的消息格
#define MAX_CMD_COUNT 12        // 最大device_cmd数量
#define MAX_CMD_LEN 15          // 最大device_cmd长度
#define MAX_MESSAGE_LEN 256     // 最大消息长度

void ESP01S_init(void);

uint8_t ESP01S_send_cmd(const char *cmd, uint32_t ms); // 发送指令给ESP01S模块

char *ESP01S_get_message(void);

void ESP01S_clean_message(void);

uint8_t ESP01S_judge_ai_message_type(const char *ai_response); // 判断AI消息类型

uint8_t ESP01S_process_ai_message(const char *ai_response); // 处理AI消息

#endif
