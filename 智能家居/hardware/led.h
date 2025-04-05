#ifndef __LED_H__
#define __LED_H__

#include "stm32f10x.h"
#include "delay.h"

#define LED_ARR 100 // PWM的ARR值，范围0~65535
#define LED_PSC 720 // PWM的PSC值，范围0~65535

typedef enum
{
    LED_OFF = 0,
    LED_ON,
}LED_STATUS; // LED的状态

void led_breath_init(void);

void led_breath(void);

void led_init(void);

void led_control(LED_STATUS state);

#endif
