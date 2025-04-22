#ifndef __TH_SENSOR_H__
#define __TH_SENSOR_H__

#include "stm32f10x.h"
#include "delay.h"
#include <string.h> // 定义了NULL
#include "OLED.h"

void DHT_init(void);

uint8_t DHT_get_temp_humi_data(float *humidity, float *temperature);

void show_DHT_sensor_value_OLED(uint8_t line, uint8_t column);

#endif
