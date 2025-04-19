#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "stm32f10x.h"

#define AIN1 GPIO_Pin_4
#define AIN2 GPIO_Pin_5

void motor_init(void);

void motor_set_speed(int8_t speed);

void motor_front_turn(void);

void motor_back_turn(void);

void motor_no_turn(void);

#endif
