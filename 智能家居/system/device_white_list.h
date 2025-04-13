#ifndef __DEVICE_WHITE_LIST_H__
#define __DEVICE_WHITE_LIST_H__

#include "stm32f10x.h"
#include "buzzer.h"
#include "servo.h"
#include "led.h"

// 设备控制函数原型（统一接口）
typedef void (*device_control_func)(void);

// 命令-函数映射结构体
typedef struct
{
    const char *cmd;            // 命令字符串
    device_control_func func;   // 对应的控制函数
}device_cmd_node;

extern device_cmd_node cmd_map_table[]; // 全局命令映射表

int get_cmd_map_table_len(void); // 获取命令映射表大小

#endif
