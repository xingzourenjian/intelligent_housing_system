#include "ESP01S.h"

static uint8_t receive_mode = 1; // 接收模式

char UART3_rx_packet[UART3_MAX_RECV_LEN] = {0};
static uint8_t p_UART3_rx_packet = 0;
uint8_t UART3_rx_flag = 0; // 接收标志位

// 计数器溢出频率：CK_PSC / (PSC+1) / (ARR+1)
// 定时1s
static void timer_init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 10000 - 1;    // ARR，范围0~65535 自动重装器
	TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;  // PSC 预分频器 时钟分频到 ​10kHz，使得每1个计数周期对应​0.1ms
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
	for(i = 0; string[i] != '\0'; i++){
        UART3_send_byte(string[i]);
    }
}

// static void UART3_send_number(uint32_t number)
// {
// 	uint32_t i = 0;
// 	while(number){
// 		i = i * 10 + number % 10;
// 		number /= 10;
// 	}
// 	number = i;			// number变为自身的回文数
// 	do{
// 		UART3_send_byte(number % 10 + '0');
// 		// 发送末位,即原数字的首位。加上'0'得以转化为ASCLL码，文本显示为数字
// 		number /= 10;	// 丢弃末位
// 	}while(number);
// }

static void clean_UART3_rx_packet(void)
{
    memset(UART3_rx_packet, 0, sizeof(UART3_rx_packet)); // 清空接收缓存
    UART3_rx_flag = 0; // 清空接收标志位
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
	if(USART_GetITStatus(USART3, USART_IT_RXNE) == SET){
		uint8_t rx_data = USART_ReceiveData(USART3);

        if(receive_mode == 1){
            TIM_SetCounter(TIM2, 0); // 重新计数
            TIM_Cmd(TIM2, ENABLE);

            if(p_UART3_rx_packet < UART3_MAX_RECV_LEN - 1){ // 字符串后面是结束符，防止溢出
                UART3_rx_packet[p_UART3_rx_packet++] = rx_data;
            }
        }
        else if(receive_mode == 2){
            if(p_UART3_rx_packet < UART3_MAX_RECV_LEN - 1){
                UART3_rx_packet[p_UART3_rx_packet++] = rx_data;
                if(p_UART3_rx_packet >= 2){
                    if(UART3_rx_packet[p_UART3_rx_packet-2] == '\r' && UART3_rx_packet[p_UART3_rx_packet-1] == '\n'){
                        UART3_rx_flag = 1;
                    }
                }
            }
        }

    	USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }
}

void TIM2_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
    {
	    //等待时间超过1s
        UART3_rx_flag = 1;

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

    // 接收模式默认为1，接收ESP01S模块消息
    receive_mode = 1;

    // 清楚异常断开的影响
    send_cmd_to_ESP01S("+++", 1000);
    send_cmd_to_ESP01S("AT+CIPMODE=0\r\n", 500); // 关闭透传模式
    clean_UART3_rx_packet(); // 清空接收缓存

    while(!send_cmd_to_ESP01S("AT+SAVETRANSLINK=0\r\n", 500)); // 关闭透传模式自动重连
    clean_UART3_rx_packet();

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

    // 转换接收模式，即接收服务端消息
    receive_mode = 2;
}

uint8_t send_cmd_to_ESP01S(char *cmd, uint32_t ms)
{
    uint8_t ret_flag = 0;

    UART3_send_string(cmd);
    vTaskDelay(pdMS_TO_TICKS(ms));

    if(strstr(UART3_rx_packet, "OK")){ // ESP01S回复OK了
        ret_flag = 1;
    }
    else if(strstr(UART3_rx_packet, "ERROR")){ // ESP01S回复ERROR了
        ret_flag = 0;
    }
    else{
        ret_flag = 0;
    }

    return ret_flag;
}

char *get_ESP01S_message(void)
{
    if(UART3_rx_flag == 1){
        return UART3_rx_packet;
    }

    return NULL; // 没有接收到数据，返回NULL
}

void clean_ESP01S_message(void)
{
    clean_UART3_rx_packet(); // 清空接收缓存
}

// 判断AI消息类型
int judge_ai_message_type(char *ai_response)
{
    char *p = strstr(ai_response, "code"); // 检查 code 是否存在
    if(!p){
        return 0;
    }

    p = strchr(p, ':'); // 定位到 "code": 冒号

    // 定位到值的第一个非空白符
    int skipped_chars = 0;
    sscanf(p+1, "%*[ \t\n\r]%n", &skipped_chars); // 跳过连续的空白字符（空格、制表符、换行符、回车符） 没跳过时，skipped_chars变量不会被赋值
    p += skipped_chars;

    int code = 0;
    sscanf(p, "%d", &code); // 获取 code 的值

    return code;
}

// 解析AI返回的消息 {"code": 23, "action": {"开窗":"window_up", "打开报警器": "buzzer_up"}, "message": "消息"}\r\n
static uint8_t get_action_and_message(char *ai_response, char device_cmd[MAX_CMD_COUNT][MAX_CMD_LEN])
{
    uint8_t cmd_count = 0;
    memset(device_cmd, 0, MAX_CMD_COUNT * MAX_CMD_LEN);

    char *p = strstr(ai_response, "code"); // 检查 code 是否存在
    if(!p){
        return cmd_count;
    }

    p = strchr(p, ':'); // 定位到 "code": 冒号

    // 定位到值的第一个非空白符
    int skipped_chars = 0;
    sscanf(p++, "%*[ \t\n\r]%n", &skipped_chars); // 跳过连续的空白字符（空格、制表符、换行符、回车符）
    p += skipped_chars;

    int code = 0;
    sscanf(p, "%d", &code); // 获取 code 的值

    // 提取设备控制指令
    if(code == 23){ // 仅当code=23时解析action
        p = strstr(p, "action"); // 检查 action 是否存在
        if(p){
            p = strchr(p, '{'); // 定位到 action 对象起始位置，即指向 '{'
            char *p_end = strchr(p, '}'); // 定位到 action 对象结束位置，即指向 '}'
            while(cmd_count < MAX_CMD_COUNT){
                // 跳过键
                p = strchr(p, ':'); // 定位到键后面的冒号
                if(p > p_end){
                    break;
                }

                p = strchr(p, '\"'); // 定位到值的起始引号

                // 定位到值的第一个非空白符
                skipped_chars = 0;
                sscanf(p++, "%*[ \t\n\r]%n", &skipped_chars); // 跳过连续的空白字符（空格、制表符、换行符、回车符）
                p += skipped_chars;

                // 获取值
                sscanf(p, "%[^\"]", device_cmd[cmd_count++]);
            }
        }
    }

    // // 提取消息内容
    // p = strstr(ai_response, "message");
    // if(p){
    //     p = strchr(p, ':'); // 定位到键后面的冒号

    //     p = strchr(p, '\"'); // 定位到值的起始引号

    //     // 定位到值的第一个非空白符
    //     skipped_chars = 0;
    //     sscanf(p++, "%*[ \t\n\r]%n", &skipped_chars); // 跳过连续的空白字符（空格、制表符、换行符、回车符）
    //     p += skipped_chars;

    //     // 获取值
    //     sscanf(p, "%[^\"]", message);
    // }

    return cmd_count;
}

// 执行设备控制命令
uint8_t execute_command(const char *device_cmd)
{
    int cmd_map_table_len = get_cmd_map_table_len(); // 获取命令映射表大小

    if(cmd_map_table_len <= 0){ // 映射表为空
        return 0;
    }

    // 遍历映射表查找匹配命令
    for(int i = 0; i < cmd_map_table_len; i++){
        if(strcmp(device_cmd, cmd_map_table[i].cmd) == 0){
            cmd_map_table[i].func(); // 调用对应函数
            return 1;
        }
    }

    return 0; // 未找到匹配命令
}

// 处理AI设备控制命令
uint8_t process_ai_device_control_cmd(char *ai_response)
{
    char device_cmd[MAX_CMD_COUNT][MAX_CMD_LEN] = {0}; // 存储设备命令
    uint8_t cmd_count = 0; // 设备命令计数器
    uint8_t exec_flag = 0; // 执行命令结果

    // 解析AI返回的消息
    cmd_count = get_action_and_message(ai_response, device_cmd);

    // 执行设备控制命令
    for(uint8_t i = 0; i < cmd_count; i++){
        exec_flag = execute_command(device_cmd[i]); // 执行设备控制命令
    }

    return exec_flag; // 返回执行命令结果
}
