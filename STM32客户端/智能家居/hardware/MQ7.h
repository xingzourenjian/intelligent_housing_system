#ifndef __MQ7_H__
#define __MQ7_H__

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "OLED.h"

void MQ7_sensor_init(void);

float MQ7_get_sensor_value(void);

float MQ7_get_sensor_voltage_value(void);

void MQ7_sensor_show_value_to_OLED(uint8_t line, uint8_t column);

#endif
