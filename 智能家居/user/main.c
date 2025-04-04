#include "main.h"

int main(int argc, const char *argv[])
{
    serial_init();
    buzzer_GPIO_init();
    //buzzer_up();
    ESP01S_init();

    while(1)
    {

    }
}
