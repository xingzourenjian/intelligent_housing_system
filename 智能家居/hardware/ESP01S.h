#ifndef __ESP01S_H__
#define __ESP01S_H__

#include "stm32f10x.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h> // 定义了NULL

#include "bluetooth.h"
#include "FreeRTOS.h"
#include "task.h"
#include "device_white_list.h"

#define UART3_MAX_RECV_LEN 512
#define UART3_MAX_SEND_LEN 512

extern char UART3_rx_packet[UART3_MAX_RECV_LEN];
extern uint8_t UART3_rx_flag;

// AI返回的消息格
#define MAX_CMD_COUNT 12        // 最大device_cmd数量
#define MAX_CMD_LEN 15          // 最大device_cmd长度
#define MAX_MESSAGE_LEN 256     // 最大消息长度

void ESP01S_init(void);

uint8_t send_cmd_to_ESP01S(char *cmd, uint32_t ms);

char *get_ESP01S_message(void);

void clean_ESP01S_message(void);

int judge_ai_message_type(char *ai_response);

uint8_t execute_command(const char *device_cmd);

uint8_t process_ai_device_control_cmd(char *ai_response);

#endif
