#include "stm32f10x.h"
#include "buzzer.h"

// PA8 蜂鸣器
void buzzer_GPIO_init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // 开启时钟外设
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出模式
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure); // 时钟外设初始化

	GPIO_ResetBits(GPIOA, GPIO_Pin_8);
}

void buzzer_up(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_8);
}

void buzzer_off(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_8);
}

