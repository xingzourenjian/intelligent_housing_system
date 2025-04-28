#ifndef __SERVO_H__
#define __SERVO_H__

#include "stm32f10x.h"

void servo_init(void);

void servo_set_angle(float angle);

void servo_window_up(void); // 打开窗户

void servo_window_off(void); // 关闭窗户

#endif
