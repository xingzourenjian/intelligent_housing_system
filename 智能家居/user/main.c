#include "main.h"

int main(int argc, const char *argv[])
{
    uint8_t data_buffer[5];

    blue_init();
    DHT_init();
    buzzer_init();
    ESP01S_init();

    while(1)
    {
        if(DHT_get_temp_humi_data(data_buffer) == 1)
        {
            send_message_to_blue_string("湿度: ");
            send_message_to_blue_num(data_buffer[0]); // 湿度整数
            send_message_to_blue_string(".");
            send_message_to_blue_num(data_buffer[1]); // 湿度小数
            send_message_to_blue_string("\r\n温度: ");
            send_message_to_blue_num(data_buffer[2]); // 温度整数
            send_message_to_blue_string(".");
            send_message_to_blue_num(data_buffer[3]); // 温度小数
        }
        delay_ms(2000);
    }
}
