#ifndef __ASRPRO_H__
#define __ASRPRO_H__

#include "stm32f10x.h"
#include <string.h>

#include "main.h"

#define UART1_MAX_RECV_LEN 512

void ASRPRO_init(void);

void ASRPRO_send_string_message(const char *str); // 发消息给语音模块

char *ASRPRO_get_message(void);

void ASRPRO_clean_message(void);

void ASRPRO_process_message(const char *asr_response, float temperature, float humidity, float smoke, float co);

#endif
