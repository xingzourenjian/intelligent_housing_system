#ifndef __ESP01S_H__
#define __ESP01S_H__

#include "stm32f10x.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "serial.h"
#include "delay.h"
#include "buzzer.h"

#define UART3_MAX_RECV_LEN 1024

void ESP01S_init(void);

void send_cmd_to_ESP01S(char *cmd);

char *get_ESP01S_message(void);

void print_ESP01S_send_message(uint8_t count);

void ai_response_process(char* ai_response);

#endif
