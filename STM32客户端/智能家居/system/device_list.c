#include "device_list.h"

// 设备指令映射链表, 可动态扩展
device_cmd_t device_cmd_list[] =
{
    {"buzzer_on",  .func.no_param_func = buzzer_on},              // 打开报警器
    {"buzzer_off", .func.no_param_func = buzzer_off},             // 关闭报警器
    {"window_on",  .func.no_param_func = servo_window_on},        // 打开窗户
    {"window_off", .func.no_param_func = servo_window_off},       // 关闭窗户
    {"fans_on", .func.no_param_func = motor_front_turn},          // 打开排风扇
    {"fans_off", .func.no_param_func = motor_no_turn},            // 关闭排风扇
    {"room_lamp_on", .func.no_param_func = room_lamp_on},         // 打开卧室灯
    {"room_lamp_off", .func.no_param_func = room_lamp_off},       // 关闭卧室灯
    {"yellow_light_on", .func.no_param_func = led_yellow_on},     // 打开黄灯
    {"yellow_light_off", .func.no_param_func = led_yellow_off},   // 关闭黄灯
    {"red_light_on", .func.no_param_func = led_red_on},           // 打开红灯
    {"red_light_off", .func.no_param_func = led_red_off},         // 关闭红灯

    {"emergency_escape_mode", .func.no_param_func = emergency_escape_mode}, // 紧急模式
    {"awary_mode", .func.no_param_func = awary_mode},                       // 离家模式
    {"recreation_mode", .func.no_param_func = recreation_mode},             // 娱乐模式
    {"sleep_mode", .func.no_param_func = sleep_mode},                       // 睡眠模式

    {"window_adjust", .func.one_param_func = servo_set_angle},              // 窗户调节
    {"fan_adjust", .func.one_param_func = motor_set_speed},                 // 排风扇调节
    {"room_lamp_adjust", .func.one_param_func = room_lamp_adjust},          // 卧室灯调节
};

// 获取设备指令映射链表长度
uint16_t device_cmd_list_length(void)
{
    return sizeof(device_cmd_list) / sizeof(device_cmd_t);
}
