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
    send_cmd_to_ESP01S("AT+CIPSEND=0,40\r\n"); // 向服务器发数据，id=0，字节长度：12
    print_ESP01S_send_message(1);
    send_cmd_to_ESP01S("你回答我一个英文，即：led_off那你么么么么么m");
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
            if(strcmp(p_str, "NULL") != 0)
            {
                printf("%s\r\n", p_str);
                break;
            }
        }
    }
}
