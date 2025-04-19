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

void buzzer_pre_warn(BUZZER_PRE_WARN pre_warn_select)
{
	if(pre_warn_select == BUZZER_PRE_WARN_1) // 一级预警
	{
		led_down(LED8);	 // 红灯熄灭
		led_up(LED7); // 黄灯常亮
		for(uint8_t i = 0; i < 3; i++)
		{
			buzzer_up(); // 蜂鸣器间歇报警
			delay_ms(500);
			buzzer_off();
			delay_ms(500);
		}
	}
	else if(pre_warn_select == BUZZER_PRE_WARN_2) // 二级预警
	{
		led_down(LED7);	 // 黄灯熄灭
		led_up(LED8); // 红灯常亮
		buzzer_up(); // 蜂鸣器持续报警
	}
	else if(pre_warn_select == BUZZER_PRE_WARN_OFF) // 预警解除
	{
		led_down(LED7);	 // 黄灯熄灭
		led_down(LED8);	 // 红灯熄灭
		buzzer_off();	 // 蜂鸣器关闭
	}
}
