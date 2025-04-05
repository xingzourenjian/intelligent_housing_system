#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "stm32f10x.h"
#include <string.h> // 定义了NULL

#define SERIAL_MAX_RECV_LEN 400

void serial_init(void);

void serial_send_byte(char byte);

void serial_send_string(char *string);

void serial_send_number(uint32_t number);

void serial_rx_packet_print(void);

char *get_serial_rx_packet(void); // 打印或获取接收的信息后，才能接收下一条信息

#endif
