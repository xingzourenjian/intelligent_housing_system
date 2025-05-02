#ifndef __DEVICE_LIST_H__
#define __DEVICE_LIST_H__

#include "stm32f10x.h"
#include "buzzer.h"
#include "servo.h"
#include "motor.h"
#include "led.h"
#include "main.h"

// 设备控制函数统一接口
typedef void (*device_control_no_param_func)(void);
typedef void (*device_control_one_param_func)(uint8_t);

// 设备指令-函数名映射结构体
typedef struct
{
    const char *cmd;    // 设备指令字符串
    union               // 对应的控制函数
    {
        device_control_no_param_func no_param_func;
        device_control_one_param_func one_param_func;
    }func;
}device_cmd_t;

extern device_cmd_t device_cmd_list[]; // 设备指令映射链表

uint16_t device_cmd_list_length(void);

#endif
