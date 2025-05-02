#ifndef __TH_SENSOR_H__
#define __TH_SENSOR_H__

#include "stm32f10x.h"
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "delay.h"
#include "OLED.h"

void DHT_init(void);

uint8_t DHT_get_temp_humi_data(float *humidity, float *temperature);

void DHT_sensor_show_value_to_OLED(uint8_t line, uint8_t column);

#endif
