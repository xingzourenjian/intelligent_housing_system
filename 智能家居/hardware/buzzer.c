#include "buzzer.h"

// PA8 蜂鸣器
void buzzer_init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_ResetBits(GPIOA, GPIO_Pin_8); // 默认关闭蜂鸣器
}

// 控制蜂鸣器状态
void buzzer_control(BUZZER_STATUS state)
{
	if(state == BUZZER_ON)
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_8);
	}
	else if(state == BUZZER_OFF)
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
	buzzer_control(BUZZER_ON);
}

void buzzer_off(void)
{
	buzzer_control(BUZZER_OFF);
}
