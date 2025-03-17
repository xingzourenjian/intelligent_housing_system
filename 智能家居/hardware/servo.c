#include "stm32f10x.h"
#include "servo.h"

// PB8 TIM4_CH3 舵机
void servo_init(void)
{
    PWM_init();
}

// 设置转向角度限制180°
void servo_set_angle(float angle)
{
    PWM_set_compare3(angle / 180 * 2000 + 500);
}

