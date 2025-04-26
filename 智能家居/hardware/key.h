#ifndef __KEY_H__
#define __KEY_H__

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "led.h"

void key_init(void);

void EXTI9_5_IRQHandler(void);

#endif
