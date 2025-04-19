#ifndef __BUZZER_H__
#define __BUZZER_H__

#include "stm32f10x.h"
#include "led.h"
#include "delay.h"

typedef enum
{
    BUZZER_OFF = 0, // 蜂鸣器关闭
    BUZZER_ON,      // 蜂鸣器打开
}BUZZER_STATUS;

typedef enum
{
    BUZZER_PRE_WARN_OFF = 0, // 预警解除
    BUZZER_PRE_WARN_1,       // 一级预警
    BUZZER_PRE_WARN_2,       // 二级预警
}BUZZER_PRE_WARN; // 蜂鸣器状态

void buzzer_init(void); // 初始化蜂鸣器端口

void buzzer_control(BUZZER_STATUS state); // 控制蜂鸣器状态

void buzzer_up(void); // 打开蜂鸣器

void buzzer_off(void); // 关闭蜂鸣器

void buzzer_pre_warn(BUZZER_PRE_WARN pre_warn_select); // 蜂鸣器预警

#endif
