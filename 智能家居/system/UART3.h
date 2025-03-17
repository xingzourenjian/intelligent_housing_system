#ifndef __UART3_H__
#define __UART3_H__

#include <stdio.h>
#include <stdarg.h>
#include "timer.h"
#include "serial.h"

#define UART3_MAX_RECV_LEN 1024

void UART3_init(void);

void UART3_send_byte(char byte);

void UART3_send_array(char *array, uint16_t length);

void UART3_send_string(char *string);

void UART3_send_number(uint32_t number);

void UART3_rx_packet_print(void);

char *get_UART3_rx_packet(void); // 打印或获取接收的信息后，才能接收下一条信息

#endif
