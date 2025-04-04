#ifndef __LIGHT_SENSOR_H__
#define __LIGHT_SENSOR_H__

#include "stm32f10x.h"
#include "OLED.h"

void light_sensor_init(void);

uint16_t get_light_sensor_value(void);

float get_light_sensor_voltage_value(void);

void show_light_sensor_voltage_value_OLED(uint8_t line, uint8_t column);

#endif
