#include "stm32f10x.h"
#include "UART3.h"

static char UART3_rx_packet[UART3_MAX_RECV_LEN] = {0};
static uint8_t UART3_rx_flag = 0;
static uint8_t p_UART3_rx_packet = 0;

// PB10  UART3_TX
// PB11  UART3_RX
void UART3_init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; // TX
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11; // RX
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 115200; // 波特率
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART3, &USART_InitStructure);
	
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);
	
	USART_Cmd(USART3, ENABLE);

	timer_init();
}

void UART3_send_byte(char byte)
{
	USART_SendData(USART3, byte);
	while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
}

void UART3_send_array(char *array, uint16_t length)
{
	uint16_t i = 0;
	for(i = 0; i < length; i++)
    {
		UART3_send_byte(array[i]);
	}
}

void UART3_send_string(char *string)
{
	uint8_t i = 0;
	for(i = 0; string[i] != '\0'; i++)
    {
		UART3_send_byte(string[i]);
	}
}

void UART3_send_number(uint32_t number)
{
	uint32_t i = 0;
	while(number)
    {
		i = i * 10 + number % 10;
		number /= 10;
	}
	number = i;			// number变为自身的回文数
	do
    {
		UART3_send_byte(number % 10 + '0');	
		// 发送末位,即原数字的首位。加上'0'得以转化为ASCLL码，文本显示为数字
		number /= 10;	// 丢弃末位
	}while(number);
}

void UART3_rx_packet_print(void)
{
    if(UART3_rx_flag == 1)
    {
        UART3_rx_flag = 0;
        UART3_send_string(UART3_rx_packet);
    }
}

/*
功能：
    获取接收的信息
参数：
    void
返回值：
    成功：接收的信息
    失败：NULL字符串
*/
char *get_UART3_rx_packet(void)
{
    if(UART3_rx_flag == 1)
    {
        UART3_rx_flag = 0;
        return (char *)UART3_rx_packet;
    }
    return (char *)"NULL";
}

// printf移植
struct __FILE
{
    int handle;
};

FILE __stdout;

int fputc(int ch, FILE *f)
{
    USART_SendData(USART3, ch);
    while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET); // 等待上次的数据发完。
    return ch;
}

void USART3_IRQHandler(void)
{	
	if(USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
    {
		uint8_t rx_data = USART_ReceiveData(USART3);
		if(UART3_rx_flag == 0)
        {
		    TIM_SetCounter(TIM2, 0); // 重新计数
		    TIM_Cmd(TIM2, ENABLE);
			UART3_rx_packet[p_UART3_rx_packet++] = rx_data;
		}
		
    	USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    	// RXNE位也可以通过写入0来清除
    }
}

void TIM2_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
    {
	    UART3_rx_flag = 1; //等待时间超过1s，停止串口数据接收
	    UART3_rx_packet[p_UART3_rx_packet] = '\0';
		p_UART3_rx_packet = 0;
		TIM_Cmd(TIM2, DISABLE); //接收数据结束，停止计时
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	} 
}
