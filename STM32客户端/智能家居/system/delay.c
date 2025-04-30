#include "delay.h"

void delay_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    TIM_TimeBaseInitTypeDef TIM_InitStruct;
    TIM_InitStruct.TIM_Prescaler = 72 - 1; // 时钟分频到 ​1MHz，使得每1个计数周期对应​1us
    TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_InitStruct.TIM_Period = 65535;
    TIM_InitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_InitStruct.TIM_RepetitionCounter = 0;  // 重复计数器设为0
    TIM_TimeBaseInit(TIM1, &TIM_InitStruct);

    TIM_CtrlPWMOutputs(TIM1, ENABLE);  // 启用TIM1主输出
    TIM_Cmd(TIM1, DISABLE);
}

void delay_us(uint16_t us)
{
    TIM_Cmd(TIM1, ENABLE);
    TIM_SetCounter(TIM1, 0);                      // 清零计数器
    while(TIM_GetCounter(TIM1) < us);             // 等待计数值达到目标微秒数
    TIM_Cmd(TIM1, DISABLE);
}

void delay_ms(uint16_t ms)
{
    while(ms--){
        delay_us(1000);  // 分段延时避免溢出 65535 / 1000 = 65.535ms
    }
}
