#include "MQ2.h"

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

    // 配置通道，采样顺序1
    ADC_RegularChannelConfig(ADC1, ADC_Channel, 1, ADC_SampleTime_28Cycles5);
        // ADC总转换时间为：采样时间+12.5个ADC周期 = 41
        // 12MHz进行41个周期才能转换完 (1/12M) * 41 = 3.4us （采样等待时间）

    RCC_ADCCLKConfig(RCC_PCLK2_Div6); // 72MHz / 6 = 12MHz  ADC最大时钟14MHz

    ADC_InitTypeDef ADC_InitStruct;
    ADC_InitStruct.ADC_ContinuousConvMode = DISABLE; // 单次转换模式
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

static uint16_t get_ADC_value(uint8_t ADC_Channel)
{
    ADC_RegularChannelConfig(ADC1, ADC_Channel, 1, ADC_SampleTime_28Cycles5); // 配置通道，采样顺序1
    ADC_GetConversionValue(ADC1); // 丢掉残留数据
    ADC_SoftwareStartConvCmd(ADC1, ENABLE); // 手动触发单次转换
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET); // 等待转换完成
    return ADC_GetConversionValue(ADC1);
}

// PA4 ADC12_IN4 MQ2传感器AO口
void MQ2_sensor_init(void)
{
    ADC_init(GPIO_Pin_4, ADC_Channel_4);
}

float get_MQ2_sensor_value(void)
{
    uint16_t value = 0;

    for(int i = 0; i < 5; i++)
    {
        value += get_ADC_value(ADC_Channel_4); // 获取ADC值 0-4095
        delay_ms(5);
    }
    value /= 5; // 取平均值

    return (1.0 * value / 4095) * 100; // 转换到0-100的范围
}

float get_MQ2_sensor_voltage_value(void)
{
    return 1.0 * get_ADC_value(ADC_Channel_4) / 4095 * 3.3;
}

void show_MQ2_sensor_value_OLED(uint8_t line, uint8_t column)
{
    float value = get_MQ2_sensor_value();

    OLED_ShowString(line, column, "sm:");
    OLED_ShowNum(line, column+3, (uint32_t)value, 2);
    OLED_ShowChar(line, column+5, '.');
    OLED_ShowNum(line, column+6, (uint32_t)(value * 10) % 10, 1);
}

