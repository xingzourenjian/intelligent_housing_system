#ifndef __MAIN_H__
#define __MAIN_H__

#include "stm32f10x.h"

#include <string.h> // 定义了NULL
#include <stdio.h>

#include "FreeRTOS.h"        // FreeRTOS 核心配置和类型定义
#include "FreeRTOSConfig.h"  // FreeRTOS 用户配置文件
#include "semphr.h"          // 信号量操作

#include "croutine.h"        // 协程
#include "event_groups.h"    // 事件组
#include "list.h"            // 任务列表操作
#include "queue.h"           // 队列操作
#include "stream_buffer.h"   // 流缓冲区
#include "task.h"            // 任务管理相关 API, 如 xTaskCreate, vTaskStartScheduler 等
#include "timers.h"          // 软件定时器

#include "delay.h"
#include "IWDG.h"

#include "buzzer.h"
#include "light_sensor.h"
#include "OLED.h"
#include "servo.h"
#include "ESP01S.h"
#include "led.h"
#include "bluetooth.h"
#include "TH_sensor.h"
#include "MQ2.h"
#include "MQ7.h"
#include "motor.h"
#include "key.h"
#include "ASRPRO.h"

// 传感器数据节点
typedef struct sensor_data_node
{
    float temperature;
    float humidity;
    float smoke;
    float co;
    float light;
}sensor_data_node;

// 云端连接状态节点
typedef struct
{
    char status; // 云端连接状态 0: 断开连接 1: 已连接
}cloud_status_node;

typedef enum
{
    NORMAL_MESSAGE = 21, // 普通消息
    SENSOR_MESSAGE = 22, // 传感器消息
    DEVICE_MESSAGE = 23, // 设备控制消息
    IGNORE_MESSAGE = 24, // 忽略消息
}ai_message_type_t;

typedef enum
{
    BUZZER_PRE_WARN_OFF = 0, // 预警解除
    BUZZER_PRE_WARN_1,       // 一级预警
    BUZZER_PRE_WARN_2,       // 二级预警
}alarm_status_t;

extern uint8_t OLED_interface_flag;

void monitor_task(void *task_params);

void ai_cloud_control_task(void *task_params);

void local_edge_control_task(void *task_params);

void system_init(void);

void emergency_escape_mode(void); // 紧急逃生模式

void awary_mode(void); // 离家模式函数

void recreation_mode(void); // 娱乐模式函数

void sleep_mode(void); // 睡眠模式

void OLED_refresh(uint8_t refresh_flag);

#endif
