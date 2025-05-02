#include "main.h"

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

    if(RCC_GetFlagStatus(RCC_FLAG_IWDGRST) == SET){ // 独立看门狗复位
        buzzer_on();
        delay_ms(500);
        buzzer_off();
        RCC_ClearFlag(); // 清除复位标志位
    }

    // 创建消息队列
    cloud_status_queue_handle = xQueueCreate(5, sizeof(cloud_status_node));

    // 创建任务
    xTaskCreate(ai_cloud_control_task, "ai_cloud_control_task", 1024, NULL, 2, &cloud_task_handle); // 堆栈n字
    xTaskCreate(local_edge_control_task, "local_edge_control_task", 1024, NULL, 3, &edge_task_handle);
    xTaskCreate(monitor_task, "monitor_task", 1024, NULL, 4, &monitor_task_handle); // 创建监控任务

    // 初始化独立看门狗
    IWDG_init();

    // 启动调度器
    vTaskStartScheduler();

    // 任务调度器不会到达这里, 如果到达这里，说明调度器启动失败
    while(1){
    }
}

// 监控任务
void monitor_task(void *task_params)
{
    cloud_status_node cloud_status_msg; // 云端连接状态节点

    memset(&cloud_status_msg, 0, sizeof(cloud_status_node));

    while(1){
        // 判断云端连接状态
        if(xQueueReceive(cloud_status_queue_handle, &cloud_status_msg, 0) == pdPASS){
            if(cloud_status_msg.status == 0){
                system_status_led_control(LED_OFF); // 云端断开连接，关闭指示灯
                break; // 云端断开连接，退出循环
            }
        }

        // 处理AI设备控制消息
        char *ai_response = ESP01S_get_message();
        if(ai_response != NULL && ESP01S_judge_ai_message_type(ai_response) == DEVICE_MESSAGE){
            ESP01S_process_ai_message(ai_response);
            ESP01S_clean_message();
        }
        vTaskDelay(pdMS_TO_TICKS(300)); // 检查一次
    }

    vTaskDelete(NULL);  // 自毁任务
}

// AI云端控制任务函数
void ai_cloud_control_task(void *task_params)
{
    sensor_data_node sensor_data; // 传感器数据
    cloud_status_node cloud_status_msg; // 云端连接状态节点

    memset(&sensor_data, 0, sizeof(sensor_data_node));
    memset(&cloud_status_msg, 0, sizeof(cloud_status_node));

    // 显示状态
    system_status_led_control(LED_ON); // 系统运行状态指示灯亮起

    // 连接服务端
    ESP01S_init();

    // 与服务端握手
    printf("21\r\n"); // 告诉服务端我的设备id
    while(1){
        char *ai_response = ESP01S_get_message();
        if(ai_response != NULL && ESP01S_judge_ai_message_type(ai_response) == NORMAL_MESSAGE){ // AI招呼语
            ESP01S_clean_message();
            break; // 握手成功，退出循环
        }
        vTaskDelay(pdMS_TO_TICKS(300)); // 检查一次
    }
    printf("{\"device_type\": \"ESP01S\"}\r\n");  // 告诉服务端我的设备类型
    while(1){
        char *ai_response = ESP01S_get_message();
        if(ai_response != NULL && ESP01S_judge_ai_message_type(ai_response) == NORMAL_MESSAGE){
            ESP01S_clean_message();
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    // 与服务端通信
    uint8_t hearbeat_count = 0; // 心跳发送时机
    while(1){
        // 发送心跳包
        if(++hearbeat_count > 12){
            hearbeat_count = 0;
            printf("heartbeat\r\n");
            vTaskDelay(pdMS_TO_TICKS(2000)); // 最少2s
            char *ai_response = ESP01S_get_message();
            if(ai_response != NULL && ESP01S_judge_ai_message_type(ai_response) == NORMAL_MESSAGE){
                ESP01S_clean_message();
            }
            else{
                break; // 心跳包超时，退出循环
            }
        }

        // 获取传感器数据
        DHT_get_temp_humi_data(&sensor_data.humidity, &sensor_data.temperature);
        sensor_data.smoke = MQ2_get_sensor_value();
        sensor_data.co = MQ7_get_sensor_value();
        sensor_data.light = light_sensor_get_value();

        // 发送数据到ESP01S模块
        printf("温度:%.1f, 湿度:%.1f, 烟雾:%.1f, 一氧化碳:%.1f, 光照:%.1f\r\n",
            sensor_data.temperature,
            sensor_data.humidity,
            sensor_data.smoke,
            sensor_data.co,
            sensor_data.light
        );

        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    cloud_status_msg.status = 0; // 云端断开连接
    xQueueSend(cloud_status_queue_handle, &cloud_status_msg, portMAX_DELAY);

    vTaskDelete(NULL);  // 自毁任务
}

// 本地边缘控制任务函数
void local_edge_control_task(void *task_params)
{
    sensor_data_node sensor_data; // 传感器数据
    uint8_t alarm_count = 0; // 预警触发时机

    memset(&sensor_data, 0, sizeof(sensor_data_node));

    OLED_Clear(); // 清空OLED屏幕
    servo_window_off(); // 关闭窗户

    while(1){
        // 刷新OLED屏幕
        OLED_refresh("Smart Home");

        // 获取传感器数据
        DHT_get_temp_humi_data(&sensor_data.humidity, &sensor_data.temperature);
        sensor_data.smoke = MQ2_get_sensor_value();
        sensor_data.co = MQ7_get_sensor_value();
        sensor_data.light = light_sensor_get_value();

        // 预警
        if((sensor_data.temperature >= 38 && sensor_data.temperature < 40) ||
            (sensor_data.smoke >= 5 && sensor_data.smoke < 8) ||
            (sensor_data.co >= 3 && sensor_data.co < 6)){ // 一级预警
            if(++alarm_count > 3){ // 预警触发时机
                alarm_count = 0;
                servo_window_on();      // 打开窗户
                motor_front_turn();     // 打开排风扇
                led_close_all_alarm();  // 关闭所有警报灯
                led_yellow_on();        // 亮黄灯
                buzzer_on();            // 打开警报
                vTaskDelay(pdMS_TO_TICKS(500));
                buzzer_off();
                vTaskDelay(pdMS_TO_TICKS(500));
                buzzer_on();
                vTaskDelay(pdMS_TO_TICKS(500));
                buzzer_off();
                vTaskDelay(pdMS_TO_TICKS(500));
                buzzer_on();
                vTaskDelay(pdMS_TO_TICKS(500));
                buzzer_off();
            }
        }
        else if((sensor_data.temperature >= 40 && sensor_data.temperature < 45) || sensor_data.smoke >= 8 || sensor_data.co >= 6){ // 二级预警
            if(++alarm_count > 3){ // 预警触发时机
                alarm_count = 0;
                servo_window_on();
                motor_front_turn();
                led_close_all_alarm();
                led_red_on();
                buzzer_on();
            }
        }

        // 语音识别
        char *asr_response = ASRPRO_get_message();
        if(asr_response != NULL){
            // 处理语音识别消息
            ASRPRO_process_message(asr_response, sensor_data.temperature, sensor_data.humidity, sensor_data.smoke, sensor_data.co);
            // 语音识别消息处理完成，清空消息
            ASRPRO_clean_message();
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

    room_lamp_init();
    led_init();
    buzzer_init();
    motor_init();
    servo_init();

    OLED_Init();

    key_init();
    ASRPRO_init();

    blue_init(); // 蓝牙调试
}

// 紧急逃生模式
void emergency_escape_mode(void)
{
    buzzer_on();
    servo_window_on();
    room_lamp_adjust(100);
    led_red_on();
}

// 离家模式函数
void awary_mode(void)
{
    servo_window_off();
    motor_no_turn();
    room_lamp_adjust(0);
}

// 娱乐模式函数
void recreation_mode(void)
{
    room_lamp_adjust(10); // 调节卧室灯
    // 拉上窗帘
}

// 睡眠模式
void sleep_mode(void)
{
    room_lamp_adjust(0);
}

// OLED刷新函数
void OLED_refresh(char *str)
{
    // OLED屏显示当前状态
    OLED_ShowString(1, 3, str);
    DHT_sensor_show_value_to_OLED(2, 1);
    MQ2_sensor_show_value_to_OLED(3, 1);
    MQ7_sensor_show_value_to_OLED(3, 9);
    light_sensor_show_value_to_OLED(4, 1);
}

// 空闲任务钩子函数，即回调函数
void vApplicationIdleHook(void)
{
    __WFI();  // 进入低功耗模式，等待中断唤醒
}

// 系统节拍钩子函数
void vApplicationTickHook(void)
{
    static uint32_t wdg_counter = 0; // 看门狗计数器

    wdg_counter++;
    if(wdg_counter > 600){   // 每 600 个节拍（0.6s）喂狗一次
        IWDG_ReloadCounter(); // 重置独立看门狗计数器（喂狗）
        wdg_counter = 0;
    }
}

// 堆栈溢出钩子函数
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;  // 避免未使用变量警告
}
