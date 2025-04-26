#include "delay.h"

void delay_init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    // 配置时基参数
    TIM_TimeBaseInitTypeDef TIM_InitStruct;
    TIM_InitStruct.TIM_Prescaler = 72 - 1;                // 时钟分频到 ​1MHz，使得每1个计数周期对应​1us
    TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;  // 向上计数模式
    TIM_InitStruct.TIM_Period = 0xFFFF;                   // 自动重装载值（最大值65535）
    TIM_InitStruct.TIM_ClockDivision = TIM_CKD_DIV1;      // 输入信号采样分频器，不影响定时、PWM、输出比较
    TIM_InitStruct.TIM_RepetitionCounter = 0;             // 高级定时器专用，通用定时器忽略
    TIM_TimeBaseInit(TIM3, &TIM_InitStruct);

    TIM_Cmd(TIM3, ENABLE);
}

void delay_us(uint16_t us)
{
  TIM_SetCounter(TIM3, 0);                      // 清零计数器
  while(TIM_GetCounter(TIM3) < us);             // 等待计数值达到目标微秒数
}
