#ifndef __BLUETOOTH_H__
#define __BLUETOOTH_H__

#include "stm32f10x.h"
#include <stdio.h>
#include "serial.h"
#include "delay.h"
#include <string.h> // 定义了NULL

#define UART2_MAX_RECV_LEN 1024
#define UART2_MAX_SEND_LEN 1024

void blue_init(void);

void send_cmd_to_blue(char *str);

char *get_blue_message(void);

void print_blue_send_message(void);

#endif
