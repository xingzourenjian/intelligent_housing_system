#ifndef __ASRPRO_H__
#define __ASRPRO_H__

#include "stm32f10x.h"
#include <string.h> // 定义了NULL

#include "device_white_list.h"
#include "main.h"

#define UART1_MAX_RECV_LEN 512

void ASRPRO_init(void);

void send_message_to_ASRPRO_string(char *str);

char *get_ASRPRO_message(void);

void clean_ASRPRO_message(void);

void process_ASRPRO_message(char *asr_response, float temperature, float humidity, float smoke, float co);

#endif
