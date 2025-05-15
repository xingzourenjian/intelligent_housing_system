#ifndef __MQ2_H__
#define __MQ2_H__

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "OLED.h"

void MQ2_sensor_init(void);

float MQ2_get_sensor_value(void);

float MQ2_get_sensor_voltage_value(void);

void MQ2_sensor_show_value_to_OLED(void);

#endif
