#include "ASRPRO.h"

static char UART1_rx_packet[UART1_MAX_RECV_LEN] = {0};
static uint8_t p_UART1_rx_packet = 0;
static char UART1_tx_packet[UART1_MAX_SEND_LEN] = {0};

// PA9   UART1_TX
// PA10  UART1_RX
static void UART1_init(void)
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

static void UART1_send_byte(char byte)
{
	USART_SendData(USART1, byte);
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

static void UART1_send_string(char *string)
{
	uint8_t i = 0;
	for(i = 0; string[i] != '\0'; i++)
    {
		UART1_send_byte(string[i]);
	}
}

static void UART1_send_number(uint32_t number)
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
		UART1_send_byte(number % 10 + '0');
		// 发送末位,即原数字的首位。加上'0'得以转化为ASCLL码，文本显示为数字
		number /= 10;	// 丢弃末位
	}while(number);
}

static void clean_UART1_rx_packet(void)
{
    memset(UART1_rx_packet, 0, sizeof(UART1_rx_packet)); // 清空接收缓存
    p_UART1_rx_packet = 0; // 重置接收指针
}

void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
		uint8_t rx_data = USART_ReceiveData(USART1);

        UART1_rx_packet[p_UART1_rx_packet++] = rx_data;

    	USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    	// RXNE位也可以通过写入0来清除
    }
}

// PA9   UART1_TX
// PA10  UART1_RX
void ASRPRO_init(void)
{
	UART1_init();
}
