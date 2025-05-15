#ifndef __LIGHT_SENSOR_H__
#define __LIGHT_SENSOR_H__

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "OLED.h"

void light_sensor_init(void);

float light_sensor_get_value(void);

float light_sensor_get_voltage_value(void);

void light_sensor_show_value_to_OLED(void);

#endif
