#ifndef __ESP01S_H__
#define __ESP01S_H__

#include "stm32f10x.h"
#include <stdio.h>
#include <stdarg.h>
#include "bluetooth.h"
#include "delay.h"
#include "device_white_list.h"
#include <string.h> // 定义了NULL

#define UART3_MAX_RECV_LEN 1024
#define UART3_MAX_SEND_LEN 1024

// AI返回的消息格
#define MAX_CMD_COUNT 21    // 最大device_cmd数量
#define MAX_CMD_LEN 64      // 最大device_cmd长度
#define MAX_MESSAGE_LEN 256   // 最大message长度

void ESP01S_init(void);

void close_ESP01S(void);

uint8_t send_cmd_to_ESP01S(char *cmd, uint32_t ms);

char *get_ESP01S_message(void);

void clean_ESP01S_message(void);

void process_ai_response(char *ai_response);

#endif
