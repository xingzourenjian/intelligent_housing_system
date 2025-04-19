#include "device_white_list.h"

// 全局命令映射表（可动态扩展）
device_cmd_node cmd_map_table[] =
{
    {"buzzer_up",  buzzer_up},        // 打开报警器
    {"buzzer_off", buzzer_off},       // 关闭报警器
    {"window_up",  servo_window_up},  // 打开窗户
    {"window_off", servo_window_off}, // 关闭窗户
};

int get_cmd_map_table_len(void)
{
    return sizeof(cmd_map_table) / sizeof(device_cmd_node);
}
