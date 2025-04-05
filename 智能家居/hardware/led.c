#include "led.h"

/*
         72MHz
PWM频率：CK_PSC / (PSC+1) / (ARR+1)
PWM占空比：CCR / (ARR+1)
PWM分辨率：1 / (ARR+1)
*/
static void PWM_init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);//打开通用定时器
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
 	GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 初始化时基单元
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1; // 72MHz
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = LED_ARR - 1;      // ARR，范围0~65535
	TIM_TimeBaseInitStructure.TIM_Prescaler = LED_PSC - 1;   // PSC
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0; // 重复计数器，高级定时器才有
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

    // 输出比较通道
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure); // 赋一个初始值，再更改想要的值，防止未知错误
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0; // CCR，占空比调节
	TIM_OC1Init(TIM2, &TIM_OCInitStructure);

	TIM_Cmd(TIM2, ENABLE);
}

static void PWM_set_compare1(uint16_t compare)
{
	TIM_SetCompare1(TIM2, compare);
}

void led_breath_init(void)
{
    PWM_init();
}

void led_breath(void)
{
    int i = 0;
    for(i = 0; i<=LED_ARR; i++)
    {
        PWM_set_compare1(i);
        delay_ms(10);
    }
    for(i = 0; i<=LED_ARR; i++)
    {
        PWM_set_compare1(LED_ARR - i);
        delay_ms(10);
    }
}

void led_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
 	GPIO_Init(GPIOC, &GPIO_InitStructure);

    // GPIO_SetBits(GPIOC, GPIO_Pin_13);
    GPIO_ResetBits(GPIOC, GPIO_Pin_13);
}

void led_control(LED_STATUS state)
{
	if(state == LED_ON)
	{
		GPIO_SetBits(GPIOC, GPIO_Pin_13);
	}
	else if(state == LED_OFF)
	{
		GPIO_ResetBits(GPIOC, GPIO_Pin_13);
	}
}
