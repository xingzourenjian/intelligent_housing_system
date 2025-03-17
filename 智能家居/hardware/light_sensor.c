#include "stm32f10x.h"
#include "light_sensor.h"

// PA5 ADC12_IN5 光敏传感器AO口
void light_sensor_init(void)
{
    ADC_init(GPIO_Pin_5, ADC_Channel_5);
}

uint16_t get_light_sensor_value(void)
{
    return ADC_GetConversionValue(ADC1);    
}

float get_light_sensor_voltage_value(void)
{
    return (float)ADC_GetConversionValue(ADC1) / 4095 * 3.3;    
}

void show_light_sensor_voltage_value_OLED(uint8_t line, uint8_t column)
{
    uint16_t value = get_light_sensor_voltage_value();
    OLED_ShowNum(line, column, value, 1);
    OLED_ShowChar(line, column+1, '.');
    OLED_ShowNum(line, column+2, (uint16_t)(value * 100) % 100, 2);
}

