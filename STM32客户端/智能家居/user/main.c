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
    // xTaskCreate(ai_cloud_control_task, "ai_cloud_control_task", 1024, NULL, 2, &cloud_task_handle); // 堆栈n字
    xTaskCreate(local_edge_control_task, "local_edge_control_task", 1024, NULL, 3, &edge_task_handle);
    // xTaskCreate(monitor_task, "monitor_task", 1024, NULL, 4, &monitor_task_handle); // 创建监控任务

    // 启动调度器
    vTaskStartScheduler();

    // 任务调度器不会到达这里, 如果到达这里，说明调度器启动失败
    while(1){
        buzzer_up();
    }
}

// 监控任务
void monitor_task(void *task_params)
{
    while(1){
        // 处理AI设备控制消息
        char *ai_response = get_ESP01S_message();
        if(ai_response != NULL && judge_ai_message_type(ai_response) == DEVICE_MESSAGE){
            process_ai_device_control_cmd(ai_response);
            clean_ESP01S_message();
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // 检查一次
    }

    // vTaskDelete(NULL);  // 自毁任务
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
    OLED_ShowString(1, 1, "Call AI...");

    // 连接服务端
    ESP01S_init();

    // 通知本地边缘任务连接云端成功
    cloud_status_msg.status = 1; // 连接云端成功
    xQueueSend(cloud_status_queue_handle, &cloud_status_msg, portMAX_DELAY);

    // 与服务端握手
    printf("21\r\n"); // 告诉服务端我的设备id
    while(1){
        char *ai_response = get_ESP01S_message();
        if(ai_response != NULL && judge_ai_message_type(ai_response) == NORMAL_MESSAGE){ // AI招呼语
            clean_ESP01S_message();
            break; // 握手成功，退出循环
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // 检查一次
    }

    printf("{\"device_type\": \"ESP01S\"}\r\n");  // 告诉服务端我的设备类型
    while(1){
        char *ai_response = get_ESP01S_message();
        if(ai_response != NULL && judge_ai_message_type(ai_response) == NORMAL_MESSAGE){
            clean_ESP01S_message();
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(3000)); // 检查一次
    }

    // 与服务端通信
    uint8_t hearbeat_count = 0; // 心跳发送时机
    while(1){
        // 发送心跳包
        if(++hearbeat_count > 6){
            hearbeat_count = 0;
            printf("heartbeat\r\n");
            vTaskDelay(pdMS_TO_TICKS(2000)); // 最少2s
            char *ai_response = get_ESP01S_message();
            if(ai_response != NULL && judge_ai_message_type(ai_response) == NORMAL_MESSAGE){
                clean_ESP01S_message();
            }
            else{
                break; // 心跳包超时，退出循环
            }
        }

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

        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    cloud_status_msg.status = 0; // 云端断开连接
    xQueueSend(cloud_status_queue_handle, &cloud_status_msg, portMAX_DELAY);

    vTaskDelete(NULL);  // 自毁任务
}

// 本地边缘控制任务函数
void local_edge_control_task(void *task_params)
{
    sensor_data_node sensor_data; // 传感器数据
    cloud_status_node cloud_status_msg; // 云端连接状态节点
    int local_control_flag = 1; // 本地控制标志

    memset(&sensor_data, 0, sizeof(sensor_data_node));
    memset(&cloud_status_msg, 0, sizeof(cloud_status_node));

    OLED_Clear(); // 清空OLED屏幕
    servo_window_off(); // 关闭窗户

    while(1){
        // 等待消息
        if(xQueueReceive(cloud_status_queue_handle, &cloud_status_msg, 0) == pdPASS){
            // 处理消息
            switch(cloud_status_msg.status){
                case 0: // 云端断开连接
                    system_status_led_control(LED_OFF); // 系统运行状态指示灯熄灭
                    local_control_flag = 1;
                    break;
                case 1: // 云端连接成功
                    system_status_led_control(LED_ON); // 系统运行状态指示灯亮起
                    local_control_flag = 0;
                    break;
            }
            OLED_Clear(); // 清空OLED屏幕
        }

        if(local_control_flag == 0){ // 云端控制+本地控制
            OLED_refresh("Cloud Control");
        }
        else if(local_control_flag == 1){  // 本地控制
            OLED_refresh("Local Control");
        }

        // 获取传感器数据
        DHT_get_temp_humi_data(&sensor_data.humidity, &sensor_data.temperature);
        sensor_data.smoke = get_MQ2_sensor_value();
        sensor_data.co = get_MQ7_sensor_value();
        sensor_data.light = get_light_sensor_value();

        // 预警
        if(sensor_data.temperature > 25 || sensor_data.smoke > 50 || sensor_data.co > 50){ // 一级预警
            servo_window_up();      // 打开窗户
            motor_front_turn();     // 打开排风扇
            close_all_alarm_led();  // 关闭所有警报灯
            led_yellow_up();        // 亮黄灯
            // buzzer_up();            // 打开警报
        }
        else if(sensor_data.temperature > 99 || sensor_data.smoke > 99 || sensor_data.co > 99){ // 二级预警
            servo_window_up();
            motor_front_turn();
            close_all_alarm_led();
            led_red_up();
            // buzzer_up();
        }

        // 语音识别
        char *asr_response = get_ASRPRO_message();
        if(asr_response != NULL){
            // 处理语音识别消息
            process_ASRPRO_message(asr_response, sensor_data.temperature, sensor_data.humidity, sensor_data.smoke, sensor_data.co);
            // 语音识别消息处理完成，清空消息
            clean_ASRPRO_message();
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
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

    // IWDG_init();
}

// 娱乐模式函数
void recreation_mode(void)
{
    room_lamp_adjust(10); // 调节卧室灯
    // 拉上窗帘
}

// 离家模式函数
void awary_mode(void)
{
    servo_window_off();
    motor_no_turn();
    room_lamp_adjust(0);
}

// 睡眠模式
void sleep_mode(void)
{
    room_lamp_adjust(0);
}

// 紧急逃生模式
void emergency_escape_mode(void)
{
    buzzer_up();
    servo_window_up();
    room_lamp_adjust(100);
    led_red_up();
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
    if(wdg_counter >= 800){   // 每 800 个节拍（0.8s）喂狗一次
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
