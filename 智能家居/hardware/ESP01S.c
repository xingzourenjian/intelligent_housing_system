#include "ESP01S.h"

static char UART3_rx_packet[UART3_MAX_RECV_LEN] = {0};
static uint8_t UART3_rx_flag = 0;
static uint8_t p_UART3_rx_packet = 0;

// 计数器溢出频率：CK_PSC / (PSC+1) / (ARR+1)
// 定时1s
static void timer_init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); // 打开通用定时器
	
	TIM_InternalClockConfig(TIM2); // 默认内部时钟，可不写
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 10000 - 1;    // ARR，范围0~65535 自动重装器
	TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;  // PSC 预分频器
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0; // 重复计数器，高级定时器才有
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure); 
	
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);
	// 复位后，TIM_TimeBaseInit函数会产生更新事件，而更新中断也会同步产生并置位，需手动清除中断标志位
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);
	
	TIM_Cmd(TIM2, DISABLE);
}

// PB10  UART3_TX
// PB11  UART3_RX
static void UART3_init(void)
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

static void UART3_send_byte(char byte)
{
	USART_SendData(USART3, byte);
	while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
}

static void UART3_send_array(char *array, uint16_t length)
{
	uint16_t i = 0;
	for(i = 0; i < length; i++)
    {
		UART3_send_byte(array[i]);
	}
}

static void UART3_send_string(char *string)
{
	uint8_t i = 0;
	for(i = 0; string[i] != '\0'; i++)
    {
		UART3_send_byte(string[i]);
	}
}

static void UART3_send_number(uint32_t number)
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

static void UART3_rx_packet_print(void)
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
static char *get_UART3_rx_packet(void)
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

// PB10  UART3_TX
// PB11  UART3_RX
void ESP01S_init(void)
{
	UART3_init();

    // send_cmd_to_ESP01S("AT+RST\r\n"); // 重启模块
    // print_ESP01S_send_message(3);
    // send_cmd_to_ESP01S("AT+CWMODE=1\r\n"); // STA模式
    // print_ESP01S_send_message(1);
    // send_cmd_to_ESP01S("AT+CWLAP\r\n"); // 查看当前热点
    // print_ESP01S_send_message(2);
    // send_cmd_to_ESP01S("AT+CWJAP=\"qingge\",\"yx123456\"\r\n"); // 连接热点
    // print_ESP01S_send_message(4);
    // send_cmd_to_ESP01S("AT+CIFSR\r\n"); // 查看模块IP
    // print_ESP01S_send_message(1);
    send_cmd_to_ESP01S("AT+CIPMUX=1\r\n"); // 多路连接模式
    print_ESP01S_send_message(1);
    send_cmd_to_ESP01S("AT+CIPSTART=0,\"TCP\",\"47.86.228.121\",8086\r\n"); // 建立TCP连接
    print_ESP01S_send_message(2);
    send_cmd_to_ESP01S("AT+CIPSEND=0,33\r\n"); // 向服务器发数据，id=0，字节长度：12
    print_ESP01S_send_message(1);
    send_cmd_to_ESP01S("检测一下报警器是否正常\r\n");
    print_ESP01S_send_message(5);
}

void send_cmd_to_ESP01S(char *cmd)
{
    UART3_send_string(cmd);
}

char *get_ESP01S_message(void)
{
    return get_UART3_rx_packet();
}

void print_ESP01S_send_message(uint8_t count)
{
    char *p_str = "NULL";

    for(uint8_t i = 0; i < count; i++)
    {
        while(1)
        {
            p_str = get_ESP01S_message();
            ai_response_process(p_str);
            if(strcmp(p_str, "NULL") != 0)
            {
                serial_send_string(p_str);
                serial_send_string("\r\n");
                break;
            }
        }
    }
}

/*
功能：
    处理AI响应的消息
参数：
    response: AI返回的消息
返回值：
    void */
void ai_response_process(char* ai_response)
{
    char *action_ptr = strstr(ai_response, "action = "); // 指向'a'
    char *msg_ptr = strstr(ai_response, "message = ");

    // 提取action值
    if(action_ptr)
    {
        char action_val[16] = {0};
        sscanf(action_ptr, "action = %15[^;]", action_val);

        if(strcmp(action_val, "buzzer_up") == 0)
        {
            buzzer_up();
        }
        else if(strcmp(action_val, "buzzer_off") == 0)
        {
            buzzer_off();
        }
    }

    // 提取消息内容
    if(msg_ptr)
    {
        char message[128];
        sscanf(msg_ptr, "message=%127[^\n]", message);
        serial_send_string(message);
    }
}
