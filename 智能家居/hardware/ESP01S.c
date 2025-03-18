#include "stm32f10x.h"
#include "ESP01S.h"

// PB10  UART3_TX
// PB11  UART3_RX
void ESP01S_init(void)
{
	UART3_init();
    
//    send_cmd_to_ESP01S("AT+RST\r\n"); // 重启模块
//    print_ESP01S_send_message(3);
//    send_cmd_to_ESP01S("AT+CWMODE=1\r\n"); // STA模式
//    print_ESP01S_send_message(1);
//    send_cmd_to_ESP01S("AT+CWLAP\r\n"); // 查看当前热点
//    print_ESP01S_send_message(2);
//    send_cmd_to_ESP01S("AT+CWJAP=\"qingge\",\"yx123456\"\r\n"); // 连接热点
//    print_ESP01S_send_message(4);
//    send_cmd_to_ESP01S("AT+CIFSR\r\n"); // 查看模块IP
//    print_ESP01S_send_message(1);
    send_cmd_to_ESP01S("AT+CIPMUX=1\r\n"); // 多路连接模式
    print_ESP01S_send_message(1);
    send_cmd_to_ESP01S("AT+CIPSTART=0,\"TCP\",\"47.86.228.121\",8086\r\n"); // 建立TCP连接
    print_ESP01S_send_message(2);
    send_cmd_to_ESP01S("AT+CIPSEND=0,39\r\n"); // 向服务器发数据，id=0，字节长度：12
    print_ESP01S_send_message(1);
    send_cmd_to_ESP01S("报警器有点吵，影响我睡觉了\r\n");
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
