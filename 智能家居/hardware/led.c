#include "led.h"

/*
         72MHz
PWM频率：CK_PSC / (PSC+1) / (ARR+1)
PWM占空比：CCR / (ARR+1)
PWM分辨率：1 / (ARR+1)
*/
static void PWM_init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); // 打开通用定时器
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = LED9; // PB3
 	GPIO_Init(GPIOB, &GPIO_InitStructure);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); // 重映射，占据J-link调试端口
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM2, ENABLE);

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
	TIM_OC2Init(TIM2, &TIM_OCInitStructure);

	TIM_Cmd(TIM2, ENABLE);
}

static void PWM_set_compare2(uint16_t compare)
{
	TIM_SetCompare2(TIM2, compare);
}

// PB3 LED9
void led9_breath_init(void)
{
    PWM_init();
}

void led9_breath(void)
{
    int i = 0;
    for(i = 0; i<=LED_ARR; i++)
    {
        PWM_set_compare2(i);
        delay_ms(10);
    }
    for(i = 0; i<=LED_ARR; i++)
    {
        PWM_set_compare2(LED_ARR - i);
        delay_ms(10);
    }
}

// PA11 PA12 PA15   LED6 LED7 LED8
// PC13          	LED10
void led_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = LED6 | LED7 | LED8; // PA11 PA12 PA15
 	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LED10;
 	GPIO_Init(GPIOC, &GPIO_InitStructure);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); // 重映射，占据J-link调试端口
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    GPIO_SetBits(GPIOA, LED6); // 关闭LED灯
	GPIO_SetBits(GPIOA, LED7);
	GPIO_SetBits(GPIOA, LED8);

	GPIO_SetBits(GPIOC, LED10); // 系统运行状态指示灯
}

// PC13 系统运行状态指示灯
void system_status_led_control(LED_STATUS state)
{
	if(state == LED_ON)
	{
		GPIO_ResetBits(GPIOC, LED10); // 打开LED灯
	}
	else if(state == LED_OFF)
	{
		GPIO_SetBits(GPIOC, LED10); // 关闭LED灯
	}
}

void system_status_led_up(void)
{
	GPIO_ResetBits(GPIOC, LED10); // 打开LED灯
}

void system_status_led_down(void)
{
	GPIO_SetBits(GPIOC, LED10); // 关闭LED灯
}

void led_control(uint16_t LED_num, LED_STATUS state)
{
	if(state == LED_ON)
	{
		GPIO_ResetBits(GPIOA, LED_num); // 打开LED灯
	}
	else if(state == LED_OFF)
	{
		GPIO_SetBits(GPIOA, LED_num); // 关闭LED灯
	}
}

void led_up(uint16_t LED_num)
{
	led_control(LED_num, LED_ON); // 打开LED灯
}

void led_down(uint16_t LED_num)
{
	led_control(LED_num, LED_OFF); // 关闭LED灯
}

void led_flip(uint16_t LED_num)
{
    if(GPIO_ReadInputDataBit(GPIOA, LED_num) == 0)
	{
		GPIO_SetBits(GPIOA, LED_num); // 关闭LED灯
	}
	else
	{
		GPIO_ResetBits(GPIOA, LED_num); // 打开LED灯
	}
}
