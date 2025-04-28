#ifndef __MQ2_H__
#define __MQ2_H__

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "OLED.h"

void MQ2_sensor_init(void);

float get_MQ2_sensor_value(void);

float get_MQ2_sensor_voltage_value(void);

void show_MQ2_sensor_value_OLED(uint8_t line, uint8_t column);

#endif
