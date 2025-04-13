#include "buzzer.h"

// PA8 蜂鸣器
void buzzer_init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // 开启时钟外设
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出模式
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure); // 时钟外设初始化

	GPIO_ResetBits(GPIOA, GPIO_Pin_8); // 默认关闭蜂鸣器
}

// 控制蜂鸣器状态
void buzzer_control(BUZZER_STATUS state)
{
	if(state == BUZZER_ON) // 蜂鸣器打开
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_8);
	}
	else if(state == BUZZER_OFF) // 蜂鸣器关闭
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_8);
	}
	else
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_8); // 默认关闭蜂鸣器
	}
}

void buzzer_up(void)
{
	buzzer_control(BUZZER_ON); // 打开蜂鸣器
}

void buzzer_off(void)
{
	buzzer_control(BUZZER_OFF); // 关闭蜂鸣器
}
