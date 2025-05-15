/*
* FreeRTOS V202212.01
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_TIMEOUTS                      1   // 必须为1才能使用portMAX_DELAY
#define configUSE_STATS_FORMATTING_FUNCTIONS    0   // 禁用统计格式化函数
#define configUSE_CO_ROUTINES                   0   // 禁用协程
#define configUSE_RECURSIVE_MUTEXES             0   // 禁用递归互斥锁

/*================================= 系统配置 ===================================*/
#define configUSE_PREEMPTION            1   // 1: 使用抢占式调度，0: 协作式调度
#define configCPU_CLOCK_HZ              ( 72000000UL ) // CPU时钟频率(72MHz)
#define configTICK_RATE_HZ              ( 1000 )       // 系统节拍频率(1kHz)
#define configMAX_PRIORITIES            ( 5 )          // 最大任务优先级数0~configMAX_PRIORITIES-1（硬件优化时最大32）
#define configMINIMAL_STACK_SIZE        ( 128 )        // 空闲任务堆栈大小（字）
#define configTOTAL_HEAP_SIZE           ( 15 * 1024 )  // 动态内存池大小(默认10KB)
#define configMAX_TASK_NAME_LEN         ( 16 )         // 任务名最大长度

/*=============================== 任务调度配置 ================================*/
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  1 // 1: 硬件任务选择(需CLZ指令),0: 通用算法
#define configUSE_TIME_SLICING          1  // 1: 启用时间片调度（需抢占式调度）
#define configIDLE_SHOULD_YIELD         1  // 1: 空闲任务让出同优先级任务的CPU时间

/*============================== 内存管理配置 =================================*/
#define configSUPPORT_STATIC_ALLOCATION 0  // 1: 启用静态内存分配
#define configSUPPORT_DYNAMIC_ALLOCATION 1 // 1: 启用动态内存分配

/*============================= 内核功能开关 ==================================*/
#define configUSE_TASK_NOTIFICATIONS    1  // 1: 启用任务通知（每个任务+8字节）
#define configUSE_MUTEXES               1  // 1: 启用互斥信号量
#define configUSE_TRACE_FACILITY        0  // 1: 启用可视化跟踪调试
#define configUSE_16_BIT_TICKS          0  // 1: 使用16位Tick计数器，0: 32位

/*============================= 钩子函数配置 ==================================*/
#define configUSE_IDLE_HOOK             1  // 1: 启用空闲任务钩子函数
#define configUSE_TICK_HOOK             1  // 1: 启用系统节拍钩子函数

/*============================== 软件定时器 ===================================*/
#define configUSE_TIMERS                0  // 1: 启用软件定时器
#define configTIMER_TASK_PRIORITY       ( configMAX_PRIORITIES - 1 ) // 定时器任务优先级
#define configTIMER_QUEUE_LENGTH        5  // 定时器命令队列长度
#define configTIMER_TASK_STACK_DEPTH    ( 128 ) // 定时器任务堆栈深度（字）

/*============================ 调试/诊断配置 ==================================*/
#define configCHECK_FOR_STACK_OVERFLOW  2  // 0:关闭,1:方法1检测,2:方法2检测

/*============================= 中断优先级配置 ================================*/
// Cortex-M中断优先级设置（数值越小优先级越高） 中断优先级寄存器是8位的，但通常只使用高四位

// 内核相关中断，如 SysTick 和 PendSV 配置为 ​最低优先级，以确保系统调度不会阻塞其他高优先级外设中断
#define configKERNEL_INTERRUPT_PRIORITY        255 // 内核中断最低优先级(15)
// 数值小于 11，如 0~10的中断，不会被 FreeRTOS 的临界区屏蔽，可以抢占内核中断
#define configMAX_SYSCALL_INTERRUPT_PRIORITY   191  // 系统调用最高优先级(11)
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY 15  // 与CMSIS库对应的优先级值

/*============================= 包含的API函数 ================================*/
#define INCLUDE_vTaskPrioritySet        1  // 包含任务优先级设置函数
#define INCLUDE_uxTaskPriorityGet       1  // 包含任务优先级获取函数
#define INCLUDE_vTaskDelete             1  // 包含任务删除函数
#define INCLUDE_vTaskDelay               1  // 包含相对延时函数
#define INCLUDE_vTaskDelayUntil          1  // 包含绝对延时函数
#define INCLUDE_vTaskSuspend            1  // 包含任务挂起函数
#define INCLUDE_xTaskGetSchedulerState   1  // 包含调度器状态查询函数

/*============================= 处理器适配配置 ===============================*/
#define xPortPendSVHandler    PendSV_Handler  // 挂起服务例程别名
#define vPortSVCHandler       SVC_Handler     // 系统调用处理程序别名

/*============================== 数据类型定义 ==================================*/
#define configSTACK_DEPTH_TYPE          uint16_t  // 任务堆栈深度数据类型

#endif /* FREERTOS_CONFIG_H */
