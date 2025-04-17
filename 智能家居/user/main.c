#include "main.h"

int main(int argc, const char *argv[])
{
    MQ2_sensor_init();
    OLED_Init(); // OLED初始化
    light_sensor_init();

    while(1)
    {
        OLED_Clear();
        show_MQ2_sensor_voltage_value_OLED(1, 1);
        
        show_light_sensor_voltage_value_OLED(2, 1);
        delay_ms(1000); // 延时1秒
    }
}
