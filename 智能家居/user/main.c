#include "main.h"

int main(int argc, const char *argv[])
{
    serial_init();
    buzzer_init();
    ESP01S_init();
    
    while(1)
    {
        
    }
}
