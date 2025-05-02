#include "bluetooth.h"

static char UART2_rx_packet[UART2_MAX_RECV_LEN] = {0};
static uint8_t UART2_rx_flag = 0;

// PA2  UART2_TX
// PA3  UART2_RX
static void UART2_init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; // TX
 	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3; // RX
 	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 115200; // 波特率
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART2, &USART_InitStructure);

	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);

	USART_Cmd(USART2, ENABLE);
}

static void UART2_send_byte(char byte)
{
	USART_SendData(USART2, byte);
	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

static void UART2_send_string(const char *string)
{
	uint8_t i = 0;
	for(i = 0; string[i] != '\0'; i++){
		UART2_send_byte((uint8_t)string[i]);
	}
}

static void UART2_send_number(uint32_t number) // 不会发送数字末尾的0
{
	uint32_t i = 0;
	while(number){
		i = i * 10 + number % 10;
		number /= 10;
	}
	number = i;			// number变为自身的回文数
	do{
		UART2_send_byte(number % 10 + '0'); // 发送末位,即原数字的首位。加上'0'得以转化为ASCLL码，文本显示为数字
		number /= 10;	// 丢弃末位
	}while(number);
}

static void UART2_clean_rx_packet(void)
{
    memset(UART2_rx_packet, 0, sizeof(UART2_rx_packet)); // 清空接收缓存
    UART2_rx_flag = 0; // 清空接收标志位
}

void USART2_IRQHandler(void)
{
    static uint8_t rx_state = 0;
	static uint8_t p_rx_packet = 0;

	if(USART_GetITStatus(USART2, USART_IT_RXNE) == SET){
		uint8_t rx_data = USART_ReceiveData(USART2);

		if(rx_state == 0){ // 等待接收
			if(rx_data == '@'){
				rx_state = 1;
			}
		}
		else if(rx_state == 1){ // 开始接收
			if(rx_data == '#'){ // 结束接收
				UART2_rx_packet[p_rx_packet] = '\0';
				UART2_rx_flag = 1;
				rx_state = 0;
				p_rx_packet = 0;
			}
			else{ // 接收中
				if(p_rx_packet < UART2_MAX_RECV_LEN - 1){ // 防止溢出
					UART2_rx_packet[p_rx_packet++] = rx_data;
				}
			}
		}

    	USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }
}

// PA2  UART2_TX
// PA3  UART2_RX
void blue_init(void)
{
	UART2_init();
}

void blue_send_string_message(const char *str)
{
    UART2_send_string(str);
}

void blue_send_num_message(uint32_t number)
{
	UART2_send_number(number);
}

char *blue_get_message(void)
{
    if(UART2_rx_flag == 1){
		return (char *)UART2_rx_packet;
	}
    return NULL;
}

void blue_clean_message(void)
{
	UART2_clean_rx_packet();
}
