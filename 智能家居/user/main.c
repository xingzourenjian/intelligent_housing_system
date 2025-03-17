#include "stm32f10x.h"
#include <string.h>
#include "delay.h"
#include "buzzer.h"
#include "light_sensor.h"
#include "OLED.h"
#include "serial.h"
#include "servo.h"
#include "ESP01S.h"

int main(int argc, const char *argv[])
{
    serial_init();

    ESP01S_init();
    
	while(1)
    {
        
    }
}
