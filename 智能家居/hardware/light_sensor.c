#include "light_sensor.h"

static void ADC_init(uint16_t GPIO_Pin, uint8_t ADC_Channel)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // 开启时钟外设

    // 初始化GPIO口
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN; // 模拟功能模式，禁用上下拉
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure); // 时钟外设初始化

    // ADC
    ADC_RegularChannelConfig(ADC1, ADC_Channel, 1, ADC_SampleTime_28Cycles5);
        // ADC总转换时间为：采样时间+12.5个ADC周期 = 41
        // 12MHz进行41个周期才能转换完 (1/12M) * 41 = 3.4us （采样等待时间）

    RCC_ADCCLKConfig(RCC_PCLK2_Div6); // 72MHz / 6 = 12MHz

    ADC_InitTypeDef ADC_InitStruct;
    ADC_InitStruct.ADC_ContinuousConvMode = ENABLE; // 连续转换模式
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; // 外部触发源（软件触发）
    ADC_InitStruct.ADC_Mode = ADC_Mode_Independent; // ADC1、ADC2各转各的
    ADC_InitStruct.ADC_NbrOfChannel = 1; // 扫描模式时，扫描几个通道
    ADC_InitStruct.ADC_ScanConvMode = DISABLE; // 扫描转换模式
	ADC_Init(ADC1, &ADC_InitStruct);

	// 中断

	// 看门狗

	// 开启ADC
	ADC_Cmd(ADC1, ENABLE);

	// ADC校准
	ADC_ResetCalibration(ADC1); // 复位校准
    while(ADC_GetResetCalibrationStatus(ADC1) == SET); // 等待复位校准完成
    ADC_StartCalibration(ADC1); // 开始校准
    while(ADC_GetCalibrationStatus(ADC1) == SET); // 等待校准完成

    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

static uint16_t get_ADC_value(void)
{
    return ADC_GetConversionValue(ADC1);
}

// PA5 ADC12_IN5 光敏传感器AO口
void light_sensor_init(void)
{
    ADC_init(GPIO_Pin_5, ADC_Channel_5);
}

uint16_t get_light_sensor_value(void)
{
    return get_ADC_value();
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

