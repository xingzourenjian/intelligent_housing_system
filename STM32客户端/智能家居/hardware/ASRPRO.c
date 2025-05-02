#include "ASRPRO.h"

static char UART1_rx_packet[UART1_MAX_RECV_LEN] = {0};
static uint8_t UART1_rx_flag = 0;

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

static void UART1_send_string(const char *string)
{
	uint8_t i = 0;
	for(i = 0; string[i] != '\0'; i++){
		UART1_send_byte(string[i]);
	}
}

// static void UART1_send_number(uint32_t number)  // 不会发送数字末尾的0
// {
// 	uint32_t i = 0;
// 	while(number){
// 		i = i * 10 + number % 10;
// 		number /= 10;
// 	}
// 	number = i;			// number变为自身的回文数
// 	do{
// 		UART1_send_byte(number % 10 + '0'); // 发送末位,即原数字的首位。加上'0'得以转化为ASCLL码，文本显示为数字
// 		number /= 10;	// 丢弃末位
// 	}while(number);
// }

static void UART1_clean_rx_packet(void)
{
    memset(UART1_rx_packet, 0, sizeof(UART1_rx_packet)); // 清空接收缓存
    UART1_rx_flag = 0; // 清空接收标志位
}

void USART1_IRQHandler(void)
{
    static uint8_t rx_state = 0;
	static uint8_t p_rx_packet = 0;

	if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET){
		uint8_t rx_data = USART_ReceiveData(USART1);

		if(rx_state == 0){ // 等待接收
			if(rx_data == '@'){
				rx_state = 1;
			}
		}
		else if(rx_state == 1){ // 开始接收
			if(rx_data == '#'){ // 结束接收
				UART1_rx_packet[p_rx_packet] = '\0';
				UART1_rx_flag = 1;
				rx_state = 0;
				p_rx_packet = 0;
			}
			else{ // 接收中
				if(p_rx_packet < UART1_MAX_RECV_LEN - 1){ // 防止溢出
					UART1_rx_packet[p_rx_packet++] = rx_data;
				}
			}
		}

    	USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

// PA9   UART1_TX
// PA10  UART1_RX
void ASRPRO_init(void)
{
	UART1_init();
}

void ASRPRO_send_string_message(const char *str)
{
    UART1_send_string(str);
}

char *ASRPRO_get_message(void)
{
    if(UART1_rx_flag == 1){
		return (char *)UART1_rx_packet;
	}
    return NULL;
}

void ASRPRO_clean_message(void)
{
	UART1_clean_rx_packet();
}

// 获取语音识别码
static uint8_t ASRPRO_get_voice_code(const char *asr_response)
{
	uint8_t asr_code = 0; // 语音识别码

	while(*asr_response != '\0'){
		if(*asr_response >= '0' && *asr_response <= '9'){
			asr_code = (asr_code * 10) + (*asr_response - '0'); // 语音识别码
		}
		asr_response++;
	}

	return asr_code;
}

void ASRPRO_process_message(const char *asr_response, float temperature, float humidity, float smoke, float co)
{
	if(asr_response == NULL){
		return;
	}

	char send_buffer[9] = {0};
	uint8_t asr_code = 0; // 语音识别码

	asr_code = ASRPRO_get_voice_code(asr_response); // 获取语音识别码
	switch(asr_code){
		case 1: // 获取温湿度
			sprintf(send_buffer, "%.1f\n", temperature);
			ASRPRO_send_string_message(send_buffer);
			sprintf(send_buffer, "%.1f\n", humidity);
			ASRPRO_send_string_message(send_buffer);
			break;
		case 2: // 获取烟雾浓度
			sprintf(send_buffer, "%.1f\n", smoke);
			ASRPRO_send_string_message(send_buffer);
			break;
		case 3: // 获取一氧化碳浓度
			sprintf(send_buffer, "%.1f\n", co);
			ASRPRO_send_string_message(send_buffer);
			break;
		case 4: // 打开窗户
			ASRPRO_send_string_message("4\n");
			servo_window_on();
			break;
		case 5: // 关闭窗户
			ASRPRO_send_string_message("5\n");
			servo_window_off();
			break;
		case 6: // 打开排风扇
			ASRPRO_send_string_message("6\n");
			motor_front_turn();
			break;
		case 7: // 关闭排风扇
			ASRPRO_send_string_message("7\n");
			motor_no_turn();
			break;
		case 8: // 打开卧室灯
			ASRPRO_send_string_message("8\n");
			room_lamp_on();
			break;
		case 9: // 关闭卧室灯
			ASRPRO_send_string_message("9\n");
			room_lamp_off();
			break;
		case 10: // 启动娱乐模式
			ASRPRO_send_string_message("10\n");
			recreation_mode();
			break;
		case 11: // 启动睡眠模式
			ASRPRO_send_string_message("11\n");
			sleep_mode();
			break;
		case 12: // 启动离家模式
			ASRPRO_send_string_message("12\n");
			awary_mode();
			break;
		default:
			break;
	}
}
