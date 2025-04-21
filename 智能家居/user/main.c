#include "main.h"

int main(int argc, const char *argv[])
{
    char send_to_ESP01S_data[UART3_MAX_SEND_LEN]; // 存储发给ESP01S的数据
    sensor_data_node sensor_data; // 传感器数据

    led_init();
    buzzer_init();
    ESP01S_init();

    DHT_init();
    MQ2_sensor_init();
    MQ7_sensor_init();
    light_sensor_init();


    printf("21\r\n"); // 告诉服务端我的设备id
    while(get_ESP01S_message() == NULL); // # 服务端回复：AI初始问候语
    clean_ESP01S_message();
    delay_ms(50); // 防止tcp粘包

    printf("{\"device_type\": \"ESP01S\"}\r\n");  // 告诉服务端我的设备类型
    while(get_ESP01S_message() == NULL); // # 服务端回复：设备类型更换成功！
    clean_ESP01S_message();
    delay_ms(50);

    // 云端控制模式
    while(1)
    {
        // 发送心跳包
        printf("heartbeat\r\n");
        delay_ms(3000); // 等待1秒钟
        if(get_ESP01S_message() != NULL)
            clean_ESP01S_message();
        else
            break;

        // 获取传感器数据
        DHT_get_temp_humi_data(&sensor_data.humidity, &sensor_data.temperature); // 获取温湿度数据
        sensor_data.smoke = get_MQ2_sensor_value();
        sensor_data.co = get_MQ7_sensor_value();
        sensor_data.light = get_light_sensor_value();

        // 封装传感器数据
        // snprintf(send_to_ESP01S_data, UART3_MAX_SEND_LEN,
        //     "温度:%.2f, 湿度:%.2f, 烟雾:%.2f, 一氧化碳:%.2f, 光照:%.2f\r\n",
        //     sensor_data.temperature,
        //     sensor_data.humidity,
        //     sensor_data.smoke,
        //     sensor_data.co,
        //     sensor_data.light
        // );
        snprintf(send_to_ESP01S_data, UART3_MAX_SEND_LEN,
            "温度:%.2f, 湿度:%.2f, 烟雾:%.2f, 一氧化碳:%.2f, 光照:%.2f\r\n",
            39.0,
            68,
            66,
            20,
            3.0
        );

        // 发送数据到ESP01S模块
        printf("%s", send_to_ESP01S_data);

        // 处理ESP01S返回的消息
        while(get_ESP01S_message() == NULL); // 等待ESP01S返回消息
        process_ai_response(get_ESP01S_message());

        // 清空接收缓存
        clean_ESP01S_message();

        // 每隔6秒上传一次数据
        delay_ms(10000);
    }

    // 本地边缘控制模式
    while(1)
    {
        system_status_led_control(LED_OFF); // 关闭系统运行状态指示灯

    }

    close_ESP01S(); // 关闭ESP01S模块

    return 0;
}
