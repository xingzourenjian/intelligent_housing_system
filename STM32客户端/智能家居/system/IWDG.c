#include "IWDG.h"

// 初始化独立看门狗
void IWDG_init(void)
{
    IWDG_Enable(); // 启动看门狗，隐式使能内部低速时钟 LSI时钟

    // 解锁配置寄存器，即​预分频寄存器、重载寄存器
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

    // 设置预分频系数
    IWDG_SetPrescaler(IWDG_Prescaler_16); // 40kHz / 16 = 2500Hz，即每1个计数周期对应​0.4ms
    while(IWDG_GetFlagStatus(IWDG_FLAG_PVU) == SET);

    // 设置重载值 1秒超时
    IWDG_SetReload(2499); // 计数器从这个值开始递减计数，实际计数周期为2500
    while(IWDG_GetFlagStatus(IWDG_FLAG_RVU) == SET);

    // 启动看门狗
    IWDG_Enable();

    // 首次喂狗
    IWDG_ReloadCounter();
}
