#include "motor.h"

/*
         72MHz
PWM频率：CK_PSC / (PSC+1) / (ARR+1)
PWM占空比：CCR / (ARR+1)
PWM分辨率：1 / (ARR+1)
*/
static void PWM_init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); // 打开通用定时器
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;  // 72MHz
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;    		// ARR，范围0~65535
	TIM_TimeBaseInitStructure.TIM_Prescaler = 36 - 1;  		// PSC
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0; 	// 重复计数器，高级定时器才有
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);

	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure); // 赋一个初始值，再更改想要的值，防止未知错误
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0; // CCR，占空比调节
	TIM_OC4Init(TIM4, &TIM_OCInitStructure);

	TIM_Cmd(TIM4, ENABLE);
}

static void PWM_set_compare4(uint16_t compare)
{
	TIM_SetCompare4(TIM4, compare); // 用于更改通道4的CCR的值
}

// PB4 AIN1
// PB5 AIN2
// PB9 PWMA
void motor_init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); // 重映射，占据J-link调试端口
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

	PWM_init();
}

void motor_set_speed(int8_t speed)
{
	if(speed >= 0)
    {
		GPIO_SetBits(GPIOB, AIN1);   // AIN1=1, AIN2=0, 正转
		GPIO_ResetBits(GPIOB, AIN2);
		PWM_set_compare4(speed);
	}
	else
    {
		GPIO_ResetBits(GPIOB, AIN1);  // AIN1=0, AIN2=1, 反转
		GPIO_SetBits(GPIOB, AIN2);
		PWM_set_compare4(-speed);
	}
}

void motor_front_turn(void)
{
    GPIO_SetBits(GPIOB, AIN1);  // AIN1=1, AIN2=0, 正转
    GPIO_ResetBits(GPIOB, AIN2);
}

void motor_back_turn(void)
{
    GPIO_ResetBits(GPIOB, AIN1); // AIN1=0, AIN2=1, 反转
    GPIO_SetBits(GPIOB, AIN2);
}

void motor_no_turn(void)
{
    GPIO_SetBits(GPIOB, AIN1); // AIN1=1, AIN2=1, 制动
    GPIO_SetBits(GPIOB, AIN2);
}
