#ifndef __LED_H__
#define __LED_H__

#include "stm32f10x.h"
#include "delay.h"

#define LED_ARR 100 // PWM的ARR值，范围0~65535
#define LED_PSC 720 // PWM的PSC值，范围0~65535

#define LED6  GPIO_Pin_11 // PA11
#define LED7  GPIO_Pin_12 // PA12
#define LED8  GPIO_Pin_15 // PA15
#define LED9  GPIO_Pin_3  // PB3  呼吸灯
#define LED10 GPIO_Pin_13 // PC13 系统运行状态指示灯

typedef enum
{
    LED_OFF = 0,
    LED_ON,
}LED_STATUS; // LED的状态

void led9_breath_init(void);

void led9_breath(void);

void led_init(void);

void system_status_led_control(LED_STATUS state);

void system_status_led_up(void);

void system_status_led_down(void);

void led_control(uint16_t LED_num, LED_STATUS state);

void led_up(uint16_t LED_num);

void led_down(uint16_t LED_num);

void led_flip(uint16_t LED_num);

#endif
