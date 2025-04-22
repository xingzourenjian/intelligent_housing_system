#include "main.h"

int main(int argc, const char *argv[])
{
    char send_to_ESP01S_data[UART3_MAX_SEND_LEN]; // 存储发给ESP01S的数据
    sensor_data_node sensor_data; // 传感器数据

    // 初始化模块
    DHT_init();
    MQ2_sensor_init();
    MQ7_sensor_init();
    light_sensor_init();

    led_init();
    buzzer_init();
    motor_init();
    servo_init();

    OLED_Init();

    key_init();
    ASRPRO_init();

    blue_init(); // 蓝牙调试
    ESP01S_init();

    // 1、云端控制模式
    system_status_led_control(LED_ON); // 系统运行状态指示灯亮起

    // OLED屏显示当前状态
    OLED_ShowString(1, 3, "Cloud Control");
    OLED_ShowString(2, 2, "Waiting for AI");

    // 与服务端握手
    clean_ESP01S_message();
    printf("21\r\n");                    // 告诉服务端我的设备id
    while(get_ESP01S_message() == NULL); // 服务端回复：AI初始问候语
    clean_ESP01S_message();
    delay_ms(50);                        // 防止tcp粘包

    clean_ESP01S_message();
    printf("{\"device_type\": \"ESP01S\"}\r\n");  // 告诉服务端我的设备类型
    while(get_ESP01S_message() == NULL);          //服务端回复：设备类型更换成功！
    clean_ESP01S_message();
    delay_ms(50);

    // 与服务端通信
    while(1)
    {
        // OLED屏显示当前状态
        OLED_Clear();
        OLED_ShowString(1, 3, "Cloud Control");
        show_DHT_sensor_value_OLED(2, 1);
        show_MQ2_sensor_value_OLED(3, 1);
        show_MQ7_sensor_value_OLED(3, 9);
        show_light_sensor_value_OLED(4, 1);

        // 发送心跳包
        clean_ESP01S_message();
        printf("heartbeat\r\n");
        delay_ms(3000); // 等待服务端心跳包回复
        if(get_ESP01S_message() != NULL)
            clean_ESP01S_message();
        else
            break;

        // 获取传感器数据
        DHT_get_temp_humi_data(&sensor_data.humidity, &sensor_data.temperature);
        sensor_data.smoke = get_MQ2_sensor_value();
        sensor_data.co = get_MQ7_sensor_value();
        sensor_data.light = get_light_sensor_value();

        // 封装传感器数据
        snprintf(send_to_ESP01S_data, UART3_MAX_SEND_LEN,
            "温度:%.1f, 湿度:%.1f, 烟雾:%.1f, 一氧化碳:%.1f, 光照:%.1f\r\n",
            sensor_data.temperature,
            sensor_data.humidity,
            sensor_data.smoke,
            sensor_data.co,
            sensor_data.light
        );

        // 发送数据到ESP01S模块
        clean_ESP01S_message();
        printf("%s", send_to_ESP01S_data);

        // 处理ESP01S返回的消息
        while(get_ESP01S_message() == NULL);
        process_ai_response(get_ESP01S_message());

        // 清空ESP01S返回的消息
        clean_ESP01S_message();

        // 每隔一段时间上传一次数据，且刷新OLED屏幕
        delay_ms(3000);
    }

    // 2、本地边缘控制模式
    system_status_led_control(LED_OFF); // 系统运行状态指示灯熄灭
    while(1)
    {
        // OLED屏显示当前状态
        OLED_Clear();
        OLED_ShowString(1, 3, "Local Control");
        show_DHT_sensor_value_OLED(2, 1);
        show_MQ2_sensor_value_OLED(3, 1);
        show_MQ7_sensor_value_OLED(3, 9);
        show_light_sensor_value_OLED(4, 1);

        // 每隔一段时间上传一次数据，且刷新OLED屏幕
        delay_ms(3000);
    }

    // 系统资源清理
    close_ESP01S(); // 关闭ESP01S模块
    OLED_Clear();   // 清空OLED屏幕

    return 0;
}
