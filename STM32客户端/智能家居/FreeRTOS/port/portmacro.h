/*
 * FreeRTOS 端口特定宏定义头文件 - 适用于 ARM Cortex-M 处理器
 * 本文件包含硬件和编译器相关的底层配置，不应直接修改
 */

/* 防止头文件重复包含 */
#ifndef PORTMACRO_H
#define PORTMACRO_H

/* 兼容 C++ 编译器 */
/* *INDENT-OFF* */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON* */

/*-----------------------------------------------------------
 * 基础类型定义 - 确保跨编译器兼容性
 *-----------------------------------------------------------*/
/* 定义与编译器匹配的基础数据类型 */
#define portCHAR          char        /* 字符类型 */
#define portFLOAT         float       /* 单精度浮点 */
#define portDOUBLE        double      /* 双精度浮点 */
#define portLONG          long        /* 长整型 */
#define portSHORT         short       /* 短整型 */
#define portSTACK_TYPE    uint32_t    /* 堆栈单元类型（32位） */
#define portBASE_TYPE     long        /* 基础整型 */

/* 任务和调度器相关类型定义 */
typedef portSTACK_TYPE   StackType_t; /* 堆栈单元类型别名 */
typedef long             BaseType_t;  /* 基础有符号整型 */
typedef unsigned long    UBaseType_t; /* 基础无符号整型 */

/* 系统节拍类型定义 */
#if ( configUSE_16_BIT_TICKS == 1 )
    typedef uint16_t     TickType_t;           /* 16位节拍计数器 */
    #define portMAX_DELAY ( TickType_t ) 0xffff/* 最大延时值 */
#else
    typedef uint32_t     TickType_t;           /* 32位节拍计数器 */
    #define portMAX_DELAY ( TickType_t ) 0xffffffffUL /* 最大延时值 */

    /* 32位架构下节拍类型原子操作标记 */
    #define portTICK_TYPE_IS_ATOMIC 1         /* 无需临界区保护 */
#endif

/*-----------------------------------------------------------
 * 架构特定配置 - ARM Cortex-M 特性
 *-----------------------------------------------------------*/
#define portSTACK_GROWTH      ( -1 )      /* 堆栈增长方向：向下增长 */
#define portTICK_PERIOD_MS    ( ( TickType_t ) 1000 / configTICK_RATE_HZ ) /* 节拍周期(ms) */
#define portBYTE_ALIGNMENT    8           /* 内存分配对齐要求（字节） */

/* 内存屏障指令参数 */
#define portSY_FULL_READ_WRITE    ( 15 )  /* 全系统内存屏障作用域 */

/*-----------------------------------------------------------
 * 调度器控制宏 - 上下文切换管理
 *-----------------------------------------------------------*/
/* 触发 PendSV 异常实现上下文切换 */
#define portYIELD()                                 \
    {                                               \
        portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT; /* 设置 PendSV 挂起位 */ \
        __dsb( portSY_FULL_READ_WRITE );            /* 数据同步屏障 */ \
        __isb( portSY_FULL_READ_WRITE );            /* 指令同步屏障 */ \
    }

/* NVIC 中断控制寄存器地址 */
#define portNVIC_INT_CTRL_REG     ( *( ( volatile uint32_t * ) 0xe000ed04 ) )
#define portNVIC_PENDSVSET_BIT    ( 1UL << 28UL )    /* PendSV 挂起位偏移 */

/* 中断服务程序中的上下文切换控制 */
#define portEND_SWITCHING_ISR( xSwitchRequired ) do { if( xSwitchRequired != pdFALSE ) portYIELD(); } while( 0 )
#define portYIELD_FROM_ISR( x )                     portEND_SWITCHING_ISR( x )

/*-----------------------------------------------------------
 * 临界区管理 - 中断屏蔽控制
 *-----------------------------------------------------------*/
/* 外部声明的临界区函数 */
extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );

/* 中断控制宏 */
#define portDISABLE_INTERRUPTS()                  vPortRaiseBASEPRI() /* 提升 BASEPRI 屏蔽中断 */
#define portENABLE_INTERRUPTS()                   vPortSetBASEPRI( 0 ) /* 恢复中断 */
#define portENTER_CRITICAL()                      vPortEnterCritical() /* 进入临界区 */
#define portEXIT_CRITICAL()                       vPortExitCritical()  /* 退出临界区 */
#define portSET_INTERRUPT_MASK_FROM_ISR()         ulPortRaiseBASEPRI() /* ISR 中设置中断掩码 */
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( x )    vPortSetBASEPRI( x ) /* ISR 中清除中断掩码 */

/*-----------------------------------------------------------
 * 低功耗管理 - 节拍抑制功能
 *-----------------------------------------------------------*/
#ifndef portSUPPRESS_TICKS_AND_SLEEP
    extern void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime );
    #define portSUPPRESS_TICKS_AND_SLEEP( xExpectedIdleTime ) vPortSuppressTicksAndSleep( xExpectedIdleTime )
#endif

/*-----------------------------------------------------------
 * 任务选择优化 - 使用硬件特性加速调度
 *-----------------------------------------------------------*/
#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
    #define configUSE_PORT_OPTIMISED_TASK_SELECTION 1 /* 启用 CLZ 优化 */
#endif

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1
    /* 优先级数量检查 */
    #if ( configMAX_PRIORITIES > 32 )
        #error 优化任务选择仅支持优先级数 <=32
    #endif

    /* 就绪优先级位图操作 */
    #define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) ) /* 设置优先级位 */
    #define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities )  ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) ) /* 清除优先级位 */

    /* 使用 CLZ 指令查找最高优先级 */
    #define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) uxTopPriority = ( 31UL - ( uint32_t ) __clz( ( uxReadyPriorities ) ) )
#endif

/*-----------------------------------------------------------
 * 任务函数宏 - 兼容通用演示程序
 *-----------------------------------------------------------*/
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters ) void vFunction( void * pvParameters ) /* 任务函数原型 */
#define portTASK_FUNCTION( vFunction, pvParameters )       void vFunction( void * pvParameters )  /* 任务函数定义 */

/*-----------------------------------------------------------
 * 中断优先级验证（当启用 configASSERT 时）
 *-----------------------------------------------------------*/
#ifdef configASSERT
    void vPortValidateInterruptPriority( void );
    #define portASSERT_IF_INTERRUPT_PRIORITY_INVALID() vPortValidateInterruptPriority() /* 中断优先级合法性检查 */
#endif

/*-----------------------------------------------------------
 * 内联函数和汇编工具函数
 *-----------------------------------------------------------*/
#define portNOP()                       /* 空操作 */
#define portINLINE              __inline /* 内联函数指示符 */
#ifndef portFORCE_INLINE
    #define portFORCE_INLINE    __forceinline /* 强制内联 */
#endif

/* 设置 BASEPRI 寄存器值（用于中断屏蔽） */
static portFORCE_INLINE void vPortSetBASEPRI( uint32_t ulBASEPRI )
{
    __asm
    {
        /* 注意：此函数仅用于降低屏蔽级别，无需屏障 */
        msr basepri, ulBASEPRI  /* 直接写入 BASEPRI 寄存器 */
    }
}

/* 提升 BASEPRI 到系统调用最高优先级 */
static portFORCE_INLINE void vPortRaiseBASEPRI( void )
{
    uint32_t ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;
    __asm
    {
        msr basepri, ulNewBASEPRI  /* 设置中断屏蔽阈值 */
        dsb                        /* 确保操作完成 */
        isb                        /* 清空流水线 */
    }
}

/* 在 ISR 中清除中断屏蔽 */
static portFORCE_INLINE void vPortClearBASEPRIFromISR( void )
{
    __asm
    {
        msr basepri, #0  /* 解除所有中断屏蔽 */
    }
}

/* 保存并提升 BASEPRI 值 */
static portFORCE_INLINE uint32_t ulPortRaiseBASEPRI( void )
{
    uint32_t ulReturn, ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;
    __asm
    {
        mrs ulReturn, basepri     /* 读取当前 BASEPRI */
        msr basepri, ulNewBASEPRI /* 设置新的屏蔽值 */
        dsb                       /* 数据同步 */
        isb                       /* 指令同步 */
    }
    return ulReturn;
}

/* 检测当前是否在中断上下文中 */
static portFORCE_INLINE BaseType_t xPortIsInsideInterrupt( void )
{
    uint32_t ulCurrentInterrupt;
    BaseType_t xReturn;

    __asm
    {
        mrs ulCurrentInterrupt, ipsr  /* 读取 IPSR 寄存器 */
    }

    xReturn = ( ulCurrentInterrupt != 0 ) ? pdTRUE : pdFALSE;
    return xReturn;
}

/* 结束 C++ 兼容 */
/* *INDENT-OFF* */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON* */

#endif /* PORTMACRO_H */
