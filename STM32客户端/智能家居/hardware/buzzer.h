#ifndef __BUZZER_H__
#define __BUZZER_H__

#include "stm32f10x.h"

typedef enum
{
    BUZZER_OFF = 0,
    BUZZER_ON,
}buzzer_status_t;   // 蜂鸣器状态枚举体

void buzzer_init(void);

void buzzer_control(buzzer_status_t BUZZER_STATUS); // 控制蜂鸣器状态

void buzzer_on(void);

void buzzer_off(void);

#endif
