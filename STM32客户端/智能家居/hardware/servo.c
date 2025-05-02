#include "servo.h"

/*
         72MHz
PWM频率：CK_PSC / (PSC+1) / (ARR+1)
PWM占空比：CCR / (ARR+1)
PWM分辨率：1 / (ARR+1)
*/
static void PWM_init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// 初始化时基单元
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1; // 72MHz
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 20000 - 1;    // ARR，范围0~65535
	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;  // PSC 时钟分频到 ​1MHz，使得每1个计数周期对应​1us
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0; // 重复计数器
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);

	// 输出比较通道
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure); // 配置部分成员，先全部初始化
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; // 输出比较模式
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; // 输出比较极性
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0; // CCR
	TIM_OC3Init(TIM4, &TIM_OCInitStructure);

	TIM_Cmd(TIM4, ENABLE);
}

// 设置通道3的CCR
static void PWM_set_compare3(uint16_t compare)
{
	TIM_SetCompare3(TIM4, compare);
}

// PB8 TIM4_CH3 舵机(PWM频率：50Hz，即20ms   角度占空比：0.5ms-1ms-1.5ms-2ms-2.5ms，即0°~180°)
void servo_init(void)
{
    PWM_init();
}

// 设置转向角度限制180°
void servo_set_angle(uint8_t angle)
{
	// 约束角度范围
	if(angle > 180){
		angle = 180;
	}

    PWM_set_compare3((2500-500) / (180-0) * angle + 500); // 500/20000*20ms = 0.5ms
}

void servo_window_on(void)
{
	servo_set_angle(150); // 打开窗户
}

void servo_window_off(void)
{
	servo_set_angle(0); // 关闭窗户
}
