#ifndef __ASRPRO_H__
#define __ASRPRO_H__

#include "stm32f10x.h"
#include <string.h> // 定义了NULL
#include "device_white_list.h"

#define UART1_MAX_RECV_LEN 1024
#define UART1_MAX_SEND_LEN 1024

void ASRPRO_init(void);

void send_message_to_ASRPRO_string(char *str);

void send_message_to_ASRPRO_num(uint32_t number);

char *get_ASRPRO_message(void);

void clean_ASRPRO_message(void);

#endif
