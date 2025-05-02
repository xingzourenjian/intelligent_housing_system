#ifndef __SERVO_H__
#define __SERVO_H__

#include "stm32f10x.h"

void servo_init(void);

void servo_set_angle(uint8_t angle);

void servo_window_on(void);  // 打开窗户

void servo_window_off(void); // 关闭窗户

#endif
