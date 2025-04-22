#ifndef __MQ7_H__
#define __MQ7_H__

#include "stm32f10x.h"
#include "OLED.h"
#include "delay.h"

void MQ7_sensor_init(void);

float get_MQ7_sensor_value(void);

float get_MQ7_sensor_voltage_value(void);

void show_MQ7_sensor_value_OLED(uint8_t line, uint8_t column);

#endif
