#ifndef __BUZZER_H__
#define __BUZZER_H__

#include "stm32f10x.h"

void buzzer_GPIO_init(void); // 初始化蜂鸣器端口

void buzzer_up(void);

void buzzer_off(void);

#endif
