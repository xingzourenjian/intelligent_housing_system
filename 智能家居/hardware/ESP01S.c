#include "ESP01S.h"

static char UART3_rx_packet[UART3_MAX_RECV_LEN] = {0};
static uint8_t p_UART3_rx_packet = 0;
static char UART3_tx_packet[UART3_MAX_SEND_LEN] = {0};

// 计数器溢出频率：CK_PSC / (PSC+1) / (ARR+1)
// 定时1s
static void timer_init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); // 打开通用定时器

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

	TIM_Cmd(TIM2, DISABLE); // 关闭定时器
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
}

static void UART3_send_byte(char byte)
{
	USART_SendData(USART3, byte);
	while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
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

static void clean_UART3_rx_packet(void)
{
    memset(UART3_rx_packet, 0, sizeof(UART3_rx_packet)); // 清空接收缓存
    p_UART3_rx_packet = 0; // 重置接收指针
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

        TIM_SetCounter(TIM2, 0); // 重新计数
        TIM_Cmd(TIM2, ENABLE);
        UART3_rx_packet[p_UART3_rx_packet++] = rx_data;

    	USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    	// RXNE位也可以通过写入0来清除
    }
}

void TIM2_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
    {
	    //等待时间超过1s
	    UART3_rx_packet[p_UART3_rx_packet] = '\0'; // 添加字符串结束符
		TIM_Cmd(TIM2, DISABLE); //接收数据结束，停止计时
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}

// PB10  UART3_TX
// PB11  UART3_RX
void ESP01S_init(void)
{
    timer_init();
	UART3_init();

    // 清楚异常断开的影响
    send_cmd_to_ESP01S("+++", 1000);
    send_cmd_to_ESP01S("AT+CIPMODE=0\r\n", 500); // 关闭透传模式
    clean_UART3_rx_packet(); // 清空接收缓存

    while(!send_cmd_to_ESP01S("AT+SAVETRANSLINK=0\r\n", 500)); // 关闭透传模式自动重连

    // // 连接WiFi
    // while(!send_cmd_to_ESP01S("AT+CWMODE=3\r\n", 500)); // STA+AP模式
    // clean_UART3_rx_packet();
    // while(!send_cmd_to_ESP01S("AT+CWJAP=\"qingge\",\"yx123456\"\r\n", 2000)); // 连接热点
    // clean_UART3_rx_packet();

    // 连接服务器
    while(!send_cmd_to_ESP01S("AT+CIPMUX=0\r\n", 500)); // 单路连接模式
    clean_UART3_rx_packet();
    while(!send_cmd_to_ESP01S("AT+CIPSTART=\"TCP\",\"47.86.228.121\",8086\r\n", 1000)); // 建立TCP连接
    clean_UART3_rx_packet();

    // 进入透传模式
    while(!send_cmd_to_ESP01S("AT+CIPMODE=1\r\n", 500)); // 开启透传模式
    clean_UART3_rx_packet();
    while(!send_cmd_to_ESP01S("AT+CIPSEND\r\n", 500)); // 进入透传
    clean_UART3_rx_packet();

    printf("21\r\n"); // 告诉服务端我的设备id
    print_ESP01S_send_message(); // # 服务端回复：AI初始问候语
    clean_UART3_rx_packet();
    delay_ms(50); // 防止tcp粘包

    printf("{\"device_type\": \"ESP01S\"}\r\n");  // 告诉服务端我的设备类型
    print_ESP01S_send_message(); // # 服务端回复：设备类型更换成功！
    clean_UART3_rx_packet();
    delay_ms(50);

    // 与服务端通信
    int n1 = 39;
    int n2 = 30;
    int n3 = 30;
    int n4 = 19;
    int n5 = 1;
    while(1)
    {
        printf("温度=%d 湿度=%d 烟雾=%d 一氧化碳=%d 光照=%d\r\n", n1++, n2++, n3++, n4++, n5++); // 上传传感器数据，确定消息格式

        print_ESP01S_send_message(); // 发送AI消息到蓝牙模块

        process_ai_response(get_ESP01S_message()); // 处理ESP01S返回的消息

        clean_UART3_rx_packet(); // 清空接收缓存

        delay_ms(6000); // 每隔6秒上传一次数据
    }

    // 退出透传模式
    delay_ms(1000);
    while(!send_cmd_to_ESP01S("+++", 1000));
    clean_UART3_rx_packet();
    while(!send_cmd_to_ESP01S("AT+CIPMODE=0\r\n", 500)); // 关闭透传模式
    clean_UART3_rx_packet();

    // 关闭TCP连接
    while(!send_cmd_to_ESP01S("AT+CIPCLOSE\r\n", 500));
    clean_UART3_rx_packet();
}

uint8_t send_cmd_to_ESP01S(char *cmd, uint32_t ms)
{
    uint8_t ret_flag = 0;

    UART3_send_string(cmd);
    delay_ms(ms);

    if(strstr(UART3_rx_packet, "OK")) // ESP01S回复OK了
        ret_flag = 1;
    else if(strstr(UART3_rx_packet, "ERROR")) // ESP01S回复ERROR了
        ret_flag = 0;
    else
        ret_flag = 0;
}

char *get_ESP01S_message(void)
{
    return UART3_rx_packet; // 返回接收缓存
}

void print_ESP01S_send_message(void)
{
    char *ESP01S_message = NULL;
    while(1)
    {
        ESP01S_message = get_ESP01S_message();
        if(*ESP01S_message != '\0') // ESP01S开始返回的消息
        {
            delay_ms(1000); // 等待数据接收完成，定时器添加结束符
            ESP01S_message = get_ESP01S_message(); // 获取接收缓存

            send_message_to_blue_string("AI:\r"); // 发送消息到蓝牙模块
            send_message_to_blue_string(ESP01S_message);
            break;
        }
    }
}

// 解析AI返回的消息
static void get_action_and_message(char *ai_response, char device_cmd[MAX_CMD_COUNT][MAX_CMD_LEN], int *cmd_count, char message[MAX_MESSAGE_LEN])
{
    const char *ptr = ai_response;

    memset(device_cmd, 0, sizeof(char) * MAX_CMD_COUNT * MAX_CMD_LEN); // 清空命令数组
    memset(message, 0, sizeof(char) * MAX_MESSAGE_LEN); // 清空消息内容
    *cmd_count = 0; // 初始化命令计数器

    /*
        AI两种类型的消息：
            "action = 0; message = ......\r\n"
            "action = 开窗: window_up, 打开报警器: buzzer_up; message = ......\r\n"
    */

    // 提取设备控制命令到 device_cmd 字符串数组
    while(*cmd_count < MAX_CMD_COUNT)
    {
        // 找到冒号字符
        ptr = strchr(ptr, ':');
        if(ptr == NULL) // 找不到冒号，说明没有设备命令
            break;

        // 定位值的起始位置（冒号后的第一个非空格字符）
        ptr++;
        while(*ptr == ' ')
            ptr++;

        // 提取一个设备命令到 device_cmd 字符串数组
        char temp[MAX_CMD_LEN] = {0};
        sscanf(ptr, "%64[^,;]", temp); // 读取到逗号或分号为止
        strcpy(device_cmd[*cmd_count], temp);

        (*cmd_count)++; // 设备命令计数器加1

        // 移动到下一个分隔符
        ptr = strpbrk(ptr, ",;");
        if(ptr == NULL) // 找不到分隔符，说明没有更多的设备命令
            break;
    }

    // 提取消息内容到 message 字符串
    ptr = strstr(ai_response, "message"); // 定位到消息内容
    ptr = strchr(ptr, '=');

    // 定位消息的起始位置（第一个非空格字符）
    ptr++;
    while(*ptr == ' ')
        ptr++;

    char temp[MAX_MESSAGE_LEN] = {0};
    sscanf(ptr, "%256[^\r\n]", temp); // 自动包含\0终止符，即匹配到\r\n或\0结束
    strcpy(message, temp);
}

// 执行设备控制命令
static int execute_command(const char *device_cmd)
{
    int cmd_map_table_len = get_cmd_map_table_len(); // 获取命令映射表大小

    if(cmd_map_table_len <= 0) // 映射表为空，返回错误
        return 0;

    // 遍历映射表查找匹配命令
    for(int i = 0; i < cmd_map_table_len; i++)
    {
        if(strcmp(device_cmd, cmd_map_table[i].cmd) == 0)
        {
            cmd_map_table[i].func(); // 调用对应函数
            return 1;
        }
    }
    return 0; // 未找到匹配命令
}

// 处理AI响应的消息
void process_ai_response(char *ai_response)
{
    char device_cmd[MAX_CMD_COUNT][MAX_CMD_LEN] = {0}; // 存储设备命令
    char message[MAX_MESSAGE_LEN] = {0}; // 存储消息内容
    int cmd_count = 0; // 设备命令计数器

    // 解析AI返回的消息
    get_action_and_message(ai_response, device_cmd, &cmd_count, message);

    // 执行设备控制命令
    for(int i = 0; i < cmd_count; i++)
    {
        if(execute_command(device_cmd[i]) == 1) // 执行设备控制命令
        {
            send_message_to_blue_string("执行设备指令 OK\r\n"); // 发送执行成功的消息到蓝牙模块
        }
    }
}
