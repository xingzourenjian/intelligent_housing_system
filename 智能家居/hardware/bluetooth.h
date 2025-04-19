#ifndef __BLUETOOTH_H__
#define __BLUETOOTH_H__

#include "stm32f10x.h"
#include <stdio.h>
#include "delay.h"
#include <string.h> // 定义了NULL

#define UART2_MAX_RECV_LEN 1024
#define UART2_MAX_SEND_LEN 1024

void blue_init(void);

void send_message_to_blue_string(char *str);

void send_message_to_blue_num(uint32_t number);

char *get_blue_message(void);

#endif
