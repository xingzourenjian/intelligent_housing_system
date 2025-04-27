#include "device_white_list.h"

// 全局设备命令映射表（可动态扩展）
device_cmd_node cmd_map_table[] =
{
    {"buzzer_up",  buzzer_up},              // 打开报警器
    {"buzzer_off", buzzer_off},             // 关闭报警器
    {"window_up",  servo_window_up},        // 打开窗户
    {"window_off", servo_window_off},       // 关闭窗户
    {"fans_up", motor_front_turn},          // 打开排风扇
    {"fans_off", motor_no_turn},            // 关闭排风扇
    {"room_lamp_up", room_lamp_up},          // 打开卧室灯
    {"room_lamp_off", room_lamp_off},        // 关闭卧室灯
    {"yellow_light_up", led_yellow_up},     // 打开黄灯
    {"yellow_light_off", led_yellow_off},   // 关闭黄灯
    {"red_light_up", led_red_up},           // 打开红灯
    {"red_light_off", led_red_off},         // 关闭红灯
};

int get_cmd_map_table_len(void)
{
    return sizeof(cmd_map_table) / sizeof(device_cmd_node);
}
