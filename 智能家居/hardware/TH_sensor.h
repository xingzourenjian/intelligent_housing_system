#ifndef __TH_SENSOR_H__
#define __TH_SENSOR_H__

#include "stm32f10x.h"
#include "delay.h"
#include <string.h> // 定义了NULL

void DHT_init(void);

uint8_t DHT_get_temp_humi_data(uint8_t data_buffer[]); // 获取温湿度数据

#endif
