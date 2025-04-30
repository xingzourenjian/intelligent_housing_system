#include "led.h"

/*
         72MHz
PWM频率：CK_PSC / (PSC+1) / (ARR+1)
PWM占空比：CCR / (ARR+1)
PWM分辨率：1 / (ARR+1)
*/
static void PWM_init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽输出模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = ROOM_LED; // PB3
 	GPIO_Init(GPIOB, &GPIO_InitStructure);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); // 重映射，占据J-link调试端口
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM2, ENABLE);

    // 初始化时基单元
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1; // 72MHz
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = LED_ARR - 1;      	// ARR，范围0~65535
	TIM_TimeBaseInitStructure.TIM_Prescaler = LED_PSC - 1;  	// PSC
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0; 		// 重复计数器，高级定时器才有
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

    // 输出比较通道
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure); // 赋一个初始值，再更改想要的值，防止未知错误
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; // PWM模式1，CNT < CCR时输出有效电平
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; // 有效电平为高电平
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = LED_ARR; // CCR，占空比调节 高电平，卧室灯灭
	TIM_OC2Init(TIM2, &TIM_OCInitStructure);

	TIM_Cmd(TIM2, ENABLE);
}

static void PWM_set_compare2(uint16_t compare)
{
	TIM_SetCompare2(TIM2, compare);
}

// PB3 LED9
void room_lamp_init(void)
{
    PWM_init();
}

void room_lamp_adjust(int adjust_value)
{
	if(adjust_value > 100){
		adjust_value = 100; // 限制范围0~100
	}
	if(adjust_value < 0){
		adjust_value = 0;
	}

    PWM_set_compare2(LED_ARR - adjust_value);
}

void room_lamp_up(void)
{
    room_lamp_adjust(100);
}

void room_lamp_off(void)
{
    room_lamp_adjust(0);
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
	GPIO_InitStructure.GPIO_Pin = GREEN_LED | YELLOW_LED | RED_LED; // PA11 PA12 PA15
 	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = SYSTEM_LED;
 	GPIO_Init(GPIOC, &GPIO_InitStructure);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); // 重映射，占据J-link调试端口
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    GPIO_SetBits(GPIOA, GREEN_LED); // 关闭LED灯
	GPIO_SetBits(GPIOA, YELLOW_LED);
	GPIO_SetBits(GPIOA, RED_LED);

	GPIO_SetBits(GPIOC, SYSTEM_LED); // 系统运行状态指示灯
}

// PC13 系统运行状态指示灯
void system_status_led_control(LED_STATUS state)
{
	if(state == LED_ON){
		GPIO_ResetBits(GPIOC, SYSTEM_LED);
	}
	else if(state == LED_OFF){
		GPIO_SetBits(GPIOC, SYSTEM_LED);
	}
}

void system_status_led_up(void)
{
	GPIO_ResetBits(GPIOC, SYSTEM_LED);
}

void system_status_led_down(void)
{
	GPIO_SetBits(GPIOC, SYSTEM_LED);
}

void led_control(uint16_t LED_num, LED_STATUS state)
{
	if(state == LED_ON){
		GPIO_ResetBits(GPIOA, LED_num);
	}
	else if(state == LED_OFF){
		GPIO_SetBits(GPIOA, LED_num);
	}
}

void led_up(uint16_t LED_num)
{
	led_control(LED_num, LED_ON);
}

void led_off(uint16_t LED_num)
{
	led_control(LED_num, LED_OFF);
}

void led_flip(uint16_t LED_num)
{
    if(GPIO_ReadInputDataBit(GPIOA, LED_num) == 0){
		GPIO_SetBits(GPIOA, LED_num);
	}
	else{
		GPIO_ResetBits(GPIOA, LED_num);
	}
}

void close_all_alarm_led(void)
{
	led_off(YELLOW_LED);
	led_off(RED_LED);
}

void led_green_up(void)
{
	led_control(GREEN_LED, LED_ON);
}

void led_green_off(void)
{
	led_control(GREEN_LED, LED_OFF);
}

void led_yellow_up(void)
{
	led_control(YELLOW_LED, LED_ON);
}

void led_yellow_off(void)
{
	led_control(YELLOW_LED, LED_OFF);
}

void led_red_up(void)
{
	led_control(RED_LED, LED_ON);
}

void led_red_off(void)
{
	led_control(RED_LED, LED_OFF);
}
