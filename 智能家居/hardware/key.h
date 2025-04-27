#ifndef __KEY_H__
#define __KEY_H__

#include "stm32f10x.h"
#include "delay.h"
#include "led.h"
#include "buzzer.h"

void key_init(void);

void EXTI9_5_IRQHandler(void);

#endif
