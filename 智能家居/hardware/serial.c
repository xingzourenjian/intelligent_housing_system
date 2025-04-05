#include "serial.h"

static char serial_rx_packet[SERIAL_MAX_RECV_LEN] = {0};
static uint8_t serial_rx_flag = 0;

// PA9   UART1_TX
// PA10  UART1_RX
void serial_init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; // TX
 	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; // RX
 	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 115200; // 波特率
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1, &USART_InitStructure);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);

	USART_Cmd(USART1, ENABLE);
}

void serial_send_byte(char byte)
{
	USART_SendData(USART1, byte);
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void serial_send_string(char *string)
{
	uint8_t i = 0;
	for(i = 0; string[i] != '\0'; i++)
    {
		serial_send_byte((uint8_t)string[i]);
	}
}

void serial_send_number(uint32_t number)
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
		serial_send_byte(number % 10 + '0');
		// 发送末位,即原数字的首位。加上'0'得以转化为ASCLL码，文本显示为数字
		number /= 10;	// 丢弃末位
	}while(number);
}

void serial_rx_packet_print(void)
{
    if(serial_rx_flag == 1)
    {
        serial_rx_flag = 0;
        serial_send_string(serial_rx_packet);
    }
}


// 获取接收的信息
char *get_serial_rx_packet(void)
{
    if(serial_rx_flag == 1)
    {
        serial_rx_flag = 0;
        return (char *)serial_rx_packet;
    }
    return NULL;
}

void USART1_IRQHandler(void)
{
    static uint8_t rx_state = 0;
	static uint8_t p_rx_packet = 0;

	if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
		uint8_t rx_data = USART_ReceiveData(USART1);

		if(rx_state == 0) // 等待接收
        {
			if(rx_data == '@' && serial_rx_flag == 0)
            {
				rx_state = 1;
			}
		}
		else if(rx_state == 1) // 开始接收
        {
			if(rx_data == '#') // 结束接收
            {
				serial_rx_packet[p_rx_packet] = '\0';
				serial_rx_flag = 1;
				rx_state = 0;
				p_rx_packet = 0;
			}
			else // 接收中
            {
				serial_rx_packet[p_rx_packet++] = rx_data;
			}
		}

    	USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    	// RXNE位也可以通过写入0来清除
    }
}
