#ifndef __SERVO_H__
#define __SERVO_H__

#include "stm32f10x.h"

void servo_init(void);

void servo_set_angle(float angle);

#endif
