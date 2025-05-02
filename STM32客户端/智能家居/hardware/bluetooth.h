#ifndef __BLUETOOTH_H__
#define __BLUETOOTH_H__

#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>

#define UART2_MAX_RECV_LEN 512

void blue_init(void);

void blue_send_string_message(const char *str);

void blue_send_num_message(uint32_t number);

char *blue_get_message(void);

void blue_clean_message(void);

#endif
