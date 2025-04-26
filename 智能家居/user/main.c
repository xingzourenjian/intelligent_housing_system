#include "main.h"

static uint32_t wdg_counter = 0; // 看门狗计数器

// 任务句柄
TaskHandle_t cloud_task_handle = NULL;
TaskHandle_t edge_task_handle = NULL;
TaskHandle_t monitor_task_handle = NULL; // 监控任务句柄

// 其他句柄
QueueHandle_t cloud_status_queue_handle = NULL; // 云端连接状态消息队列句柄

int main(int argc, const char *argv[])
{
    // 系统初始化
    system_init();

    // 创建消息队列
    cloud_status_queue_handle = xQueueCreate(5, sizeof(cloud_status_node));

    // 创建任务
    xTaskCreate(ai_cloud_control_task, "ai_cloud_control_task", 1024, NULL, 2, &cloud_task_handle); // 堆栈n字
    xTaskCreate(local_edge_control_task, "local_edge_control_task", 512, NULL, 3, &edge_task_handle);

    // 启动调度器
    vTaskStartScheduler();

    // 任务调度器不会到达这里, 如果到达这里，说明调度器启动失败
    while(1)
    {
        buzzer_up();
    }
}

// 监控任务
void monitor_task(void *task_params)
{
    while(1)
    {
        // 处理设备控制消息
        char *ai_response = get_ESP01S_message();
        if(ai_response != NULL && is_device_control_message(ai_response) == 1)
        {
            process_ai_response(ai_response);
            clean_ESP01S_message();
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // 检查一次
    }

    // vTaskDelete(NULL);  // 自毁任务
}

// AI云端控制任务函数
void ai_cloud_control_task(void *task_params)
{
    sensor_data_node sensor_data; // 传感器数据
    cloud_status_node cloud_status_msg; // 云端连接状态节点

    system_status_led_control(LED_ON); // 系统运行状态指示灯亮起

    // OLED屏显示当前状态
    OLED_ShowString(1, 3, "Cloud Control");
    OLED_ShowString(2, 2, "Waiting for AI");

    // 连接服务端
    ESP01S_init();

    // 刷新OLED屏幕
    OLED_refresh("Cloud Control");

    // 创建监控任务
    xTaskCreate(monitor_task, "monitor_task", 1024, NULL, 4, &monitor_task_handle);

    // 与服务端握手
    printf("21\r\n");                    // 告诉服务端我的设备id
    while(1)
    {
        char *ai_response = get_ESP01S_message();
        if(ai_response != NULL && is_device_control_message(ai_response) != 1)
        {
            clean_ESP01S_message();
            break; // 握手成功，退出循环
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // 检查一次
    }

    printf("{\"device_type\": \"ESP01S\"}\r\n");  // 告诉服务端我的设备类型
    while(1)
    {
        char *ai_response = get_ESP01S_message();
        if(ai_response != NULL && is_device_control_message(ai_response) != 1)
        {
            clean_ESP01S_message();
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // 检查一次
    }

    // 与服务端通信
    while(1)
    {
        // 刷新OLED屏幕
        OLED_refresh("Cloud Control");

        // 获取传感器数据
        DHT_get_temp_humi_data(&sensor_data.humidity, &sensor_data.temperature);
        sensor_data.smoke = get_MQ2_sensor_value();
        sensor_data.co = get_MQ7_sensor_value();
        sensor_data.light = get_light_sensor_value();

        // 发送数据到ESP01S模块
        printf("温度:%.1f, 湿度:%.1f, 烟雾:%.1f, 一氧化碳:%.1f, 光照:%.1f\r\n",
            sensor_data.temperature,
            sensor_data.humidity,
            sensor_data.smoke,
            sensor_data.co,
            sensor_data.light
        );
        vTaskDelay(pdMS_TO_TICKS(500)); // 防止TCP粘包

        // 发送心跳包
        printf("heartbeat\r\n");
        vTaskDelay(pdMS_TO_TICKS(3000)); // 服务端心跳包回复超时时间
        char *ai_response = get_ESP01S_message();
        if(ai_response != NULL && is_device_control_message(ai_response) != 1)
        {
            send_message_to_blue_string(ai_response); // 发送给蓝牙调试助手
            clean_ESP01S_message();
        }
        else
            break; // 心跳包超时，退出循环
    }

    cloud_status_msg.status = 0; // 云端断开连接
    xQueueSend(cloud_status_queue_handle, &cloud_status_msg, portMAX_DELAY);

    // 资源清理
    OLED_Clear();       // 清空OLED屏幕
    vTaskDelete(NULL);  // 自毁任务
}

// 本地边缘控制任务函数
void local_edge_control_task(void *task_params)
{
    cloud_status_node cloud_status_msg; // 云端连接状态节点
    int local_control_flag = 0; // 本地控制标志

    while(1)
    {
        // 等待消息
        if(xQueueReceive(cloud_status_queue_handle, &cloud_status_msg, 0) == pdPASS)
        {
            // 处理消息
            switch(cloud_status_msg.status)
            {
                case 0: // 云端断开连接
                    system_status_led_control(LED_OFF); // 系统运行状态指示灯熄灭
                    local_control_flag = 1;
                    break;
                case 1: // 云端连接成功
                    system_status_led_control(LED_ON); // 系统运行状态指示灯亮起
                    local_control_flag = 0;
                    break;
            }
        }

        if(local_control_flag == 1)
        {
            OLED_refresh("Local Control"); // 刷新OLED屏幕
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    // vTaskDelete(NULL);  // 自毁任务
}

// 系统初始化函数
void system_init(void)
{
    // NVIC中断分组
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    delay_init();

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

    // IWDG_init();
}

// OLED刷新函数
void OLED_refresh(char *str)
{
    // OLED屏显示当前状态
    OLED_ShowString(1, 3, str);
    show_DHT_sensor_value_OLED(2, 1);
    show_MQ2_sensor_value_OLED(3, 1);
    show_MQ7_sensor_value_OLED(3, 9);
    show_light_sensor_value_OLED(4, 1);
}

// 空闲任务钩子函数，即回调函数
void vApplicationIdleHook(void)
{
    __WFI();  // 进入低功耗模式，等待中断唤醒
}

// 系统节拍钩子函数
void vApplicationTickHook(void)
{
    wdg_counter++;
    if(wdg_counter >= 800) // 每 800 个节拍（0.8s）喂狗一次
    {
        IWDG_ReloadCounter(); // 重置独立看门狗计数器（喂狗）
        wdg_counter = 0;
    }
}

// 堆栈溢出钩子函数
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;  // 避免未使用变量警告

    buzzer_up();
}
