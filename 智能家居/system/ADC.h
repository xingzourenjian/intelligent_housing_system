#ifndef __ADC_H__
#define __ADC_H__

void ADC_init(uint16_t GPIO_Pin, uint8_t ADC_Channel);

uint16_t get_ADC_value(void);

#endif
