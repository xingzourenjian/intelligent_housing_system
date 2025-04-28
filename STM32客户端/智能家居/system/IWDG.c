#include "IWDG.h"

void IWDG_init(void)
{
    // 解锁配置寄存器
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

    // 设置预分频系数 64，并等待配置完成
    IWDG_SetPrescaler(IWDG_Prescaler_64); // // 64 分频, 40kHz / 64 = 625Hz
    while(IWDG_GetFlagStatus(IWDG_FLAG_PVU) == SET);

    // 设置重载值 625（1秒超时），并等待配置完成
    IWDG_SetReload(625);
    while(IWDG_GetFlagStatus(IWDG_FLAG_RVU) == SET);

    // 启动看门狗
    IWDG_Enable();

    // 首次喂狗
    IWDG_ReloadCounter();
}
