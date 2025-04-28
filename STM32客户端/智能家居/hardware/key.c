#include "key.h"

// PA7 KEY1
// PA6 KEY2
// PA1 KEY3
// PA0 KEY4
void key_init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	// GPIO初始化
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_6 | GPIO_Pin_1 | GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// AFIO选择中断引脚
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource7 | GPIO_PinSource6 | GPIO_PinSource1 | GPIO_PinSource0); // 将外部中断的n号线映射到GPIO

	// EXTI初始化
	EXTI_InitTypeDef EXTI_InitStructure;
	EXTI_InitStructure.EXTI_Line = EXTI_Line7 | EXTI_Line6 | EXTI_Line1 | EXTI_Line0; // 选择配置外部中断的n号线
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;			// 指定外部中断线为中断模式
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;		// 指定外部中断线为下降沿触发
	EXTI_Init(&EXTI_InitStructure);

	// NVIC配置
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
	NVIC_Init(&NVIC_InitStructure);
}

// 按键1和按键2
void EXTI9_5_IRQHandler(void)
{
	// 按键1
	if(EXTI_GetITStatus(EXTI_Line7) == SET){
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7) == 0){
			delay_ms(20);											//延时消抖
            while(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7) == 0);	//等待按键松手
            delay_ms(20);

			close_all_alarm_led();  // 关闭所有警报灯
			buzzer_off();           // 关闭警报
		}
		EXTI_ClearITPendingBit(EXTI_Line7);		//清除外部中断n号线的中断标志位
	}

	// 按键2
	if(EXTI_GetITStatus(EXTI_Line6) == SET){
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6) == 0){
			delay_ms(20);											//延时消抖
            while(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6) == 0);	//等待按键松手
            delay_ms(20);

			emergency_escape_mode(); // 紧急逃生模式
		}
		EXTI_ClearITPendingBit(EXTI_Line6);		//清除外部中断n号线的中断标志位
	}
}

// 按键3
void EXTI1_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line1) == SET){
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == 0){
			delay_ms(20);											//延时消抖
            while(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == 0);	//等待按键松手
            delay_ms(20);

			awary_mode(); // 离家模式函数
		}
		EXTI_ClearITPendingBit(EXTI_Line1);		//清除外部中断n号线的中断标志位
	}
}

// 按键4
void EXTI0_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line0) == SET){
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0){
			delay_ms(20);											//延时消抖
            while(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0);	//等待按键松手
            delay_ms(20);

			sleep_mode(); // 睡眠模式
		}
		EXTI_ClearITPendingBit(EXTI_Line0);		//清除外部中断n号线的中断标志位
	}
}
