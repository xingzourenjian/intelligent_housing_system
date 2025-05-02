#ifndef __LED_H__
#define __LED_H__

#include "stm32f10x.h"

#define LED_ARR 100 // PWM的ARR值，范围0~65535
#define LED_PSC 720 // PWM的PSC值，范围0~65535

#define GREEN_LED    GPIO_Pin_11    // PA11
#define YELLOW_LED   GPIO_Pin_12    // PA12
#define RED_LED      GPIO_Pin_15    // PA15
#define ROOM_LED     GPIO_Pin_3     // PB3  卧室灯
#define SYSTEM_LED   GPIO_Pin_13    // PC13 系统运行状态指示灯

typedef enum
{
    LED_OFF = 0,
    LED_ON,
}led_status_t; // LED的状态

void room_lamp_init(void);

void room_lamp_adjust(uint8_t adjust_value);

void room_lamp_on(void);

void room_lamp_off(void);

void led_init(void);

void system_status_led_control(led_status_t LED_STATUS);

void system_status_led_on(void);

void system_status_led_off(void);

void led_control(uint16_t LED_num, led_status_t LED_STATUS);

void led_on(uint16_t LED_num);

void led_off(uint16_t LED_num);

void led_flip(uint16_t LED_num);

void led_close_all_alarm(void);

void led_green_on(void);

void led_green_off(void);

void led_yellow_on(void);

void led_yellow_off(void);

void led_red_on(void);

void led_red_off(void);

#endif
