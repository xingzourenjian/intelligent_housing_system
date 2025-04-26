/*
 * FreeRTOS 内核 V10.5.1
 * 版权所有 (C) 2021 Amazon.com, Inc. 或其关联公司。保留所有权利。
 *
 * SPDX-License-Identifier: MIT
 *
 * 特此免费授予任何获得本软件及相关文档文件（"软件"）副本的人不受限制地处理软件的权限，包括但不限于使用、复制、修改、合并、发布、分发、再许可和/或销售软件副本的权利，
 * 并允许获得软件的人在接受以下条件的前提下这样做：
 *
 * 上述版权声明和本许可声明应包含在软件的所有副本或主要部分中。
 *
 * 本软件按"原样"提供，不作任何明示或暗示的担保，包括但不限于适销性、特定用途适用性和非侵权担保。在任何情况下，作者或版权持有人均不对任何索赔、损害或其他责任负责，
 * 无论是在合同诉讼、侵权行为还是其他方面，由软件或软件的使用或其他交易引起、由软件引起或与之相关的。
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 */

/*-----------------------------------------------------------
 * 用于 ARM CM3 端口的 portable.h 文件中定义函数的实现
 *----------------------------------------------------------*/

/* 调度器包含的头文件 */
#include "FreeRTOS.h"
#include "task.h"

#ifndef configKERNEL_INTERRUPT_PRIORITY
    #define configKERNEL_INTERRUPT_PRIORITY    255  /* 内核中断优先级默认值 */
#endif

#if configMAX_SYSCALL_INTERRUPT_PRIORITY == 0
    #error configMAX_SYSCALL_INTERRUPT_PRIORITY 不能设为 0。参见 http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
#endif

/* 用于向后兼容的遗留宏。该宏过去用于替换配置生成滴答中断的时钟函数（prvSetupTimerInterrupt()），
现在该函数声明为弱函数，应用层可以通过定义同名函数（vApplicationSetupTickInterrupt()）来覆盖它 */
#ifndef configOVERRIDE_DEFAULT_TICK_CONFIGURATION
    #define configOVERRIDE_DEFAULT_TICK_CONFIGURATION    0
#endif

/* 操作内核所需的常量。首先是寄存器定义... */
#define portNVIC_SYSTICK_CTRL_REG             ( *( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG             ( *( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG    ( *( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SHPR3_REG                    ( *( ( volatile uint32_t * ) 0xe000ed20 ) )
/* ...然后是寄存器中的位定义 */
#define portNVIC_SYSTICK_CLK_BIT              ( 1UL << 2UL )
#define portNVIC_SYSTICK_INT_BIT              ( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT           ( 1UL << 0UL )
#define portNVIC_SYSTICK_COUNT_FLAG_BIT       ( 1UL << 16UL )
#define portNVIC_PENDSVCLEAR_BIT              ( 1UL << 27UL )
#define portNVIC_PEND_SYSTICK_SET_BIT         ( 1UL << 26UL )
#define portNVIC_PEND_SYSTICK_CLEAR_BIT       ( 1UL << 25UL )

#define portNVIC_PENDSV_PRI                   ( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 16UL )
#define portNVIC_SYSTICK_PRI                  ( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 24UL )

/* 检查中断优先级有效性的常量 */
#define portFIRST_USER_INTERRUPT_NUMBER       ( 16 )
#define portNVIC_IP_REGISTERS_OFFSET_16       ( 0xE000E3F0 )
#define portAIRCR_REG                         ( *( ( volatile uint32_t * ) 0xE000ED0C ) )
#define portMAX_8_BIT_VALUE                   ( ( uint8_t ) 0xff )
#define portTOP_BIT_OF_BYTE                   ( ( uint8_t ) 0x80 )
#define portMAX_PRIGROUP_BITS                 ( ( uint8_t ) 7 )
#define portPRIORITY_GROUP_MASK               ( 0x07UL << 8UL )
#define portPRIGROUP_SHIFT                    ( 8UL )

/* 用于屏蔽 ICSR 寄存器中除 VECTACTIVE 位之外的所有位 */
#define portVECTACTIVE_MASK                   ( 0xFFUL )

/* 初始化堆栈所需的常量 */
#define portINITIAL_XPSR                      ( 0x01000000 )

/* SysTick 是 24 位计数器 */
#define portMAX_24_BIT_NUMBER                 ( 0xffffffUL )

/* 用于估算在低功耗 tickless 空闲模式下 SysTick 计数器停止期间可能丢失的计数数量的修正因子 */
#define portMISSED_COUNTS_FACTOR              ( 94UL )

/* 为了严格遵循 Cortex-M 规范，任务启动地址的 bit-0 应清零，因为它会在退出 ISR 时加载到 PC 中 */
#define portSTART_ADDRESS_MASK                ( ( StackType_t ) 0xfffffffeUL )

/* 允许用户覆盖默认的 SysTick 时钟频率。如果用户定义，当配置寄存器中的 CLK 位为 0 时，
该符号必须等于 SysTick 时钟频率 */
#ifndef configSYSTICK_CLOCK_HZ
    #define configSYSTICK_CLOCK_HZ             ( configCPU_CLOCK_HZ )
    /* 确保 SysTick 时钟频率与内核相同 */
    #define portNVIC_SYSTICK_CLK_BIT_CONFIG    ( portNVIC_SYSTICK_CLK_BIT )
#else
    /* 选择不以核心频率作为 SysTick 时钟源的选项 */
    #define portNVIC_SYSTICK_CLK_BIT_CONFIG    ( 0 )
#endif

/*
 * 配置定时器生成 tick 中断。本文件中的实现为弱函数，允许应用开发者通过定义同名函数
 * (vApplicationSetupTickInterrupt()) 来更改用于生成 tick 中断的定时器
 */
void vPortSetupTimerInterrupt( void );

/*
 * 异常处理程序
 */
void xPortPendSVHandler( void );
void xPortSysTickHandler( void );
void vPortSVCHandler( void );

/*
 * 启动第一个任务作为独立函数，便于隔离测试
 */
static void prvStartFirstTask( void );

/*
 * 用于捕获试图从实现函数返回的任务
 */
static void prvTaskExitError( void );

/*-----------------------------------------------------------*/

/* 每个任务在临界区嵌套变量中维护自己的中断状态 */
static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;

/*
 * 构成一个 tick 周期的 SysTick 增量次数
 */
#if ( configUSE_TICKLESS_IDLE == 1 )
    static uint32_t ulTimerCountsForOneTick = 0;
#endif /* configUSE_TICKLESS_IDLE */

/*
 * 可抑制的最大 tick 周期数受限于 SysTick 定时器的 24 位分辨率
 */
#if ( configUSE_TICKLESS_IDLE == 1 )
    static uint32_t xMaximumPossibleSuppressedTicks = 0;
#endif /* configUSE_TICKLESS_IDLE */

/*
 * 补偿 SysTick 停止期间（仅在低功耗功能中）经过的 CPU 周期
 */
#if ( configUSE_TICKLESS_IDLE == 1 )
    static uint32_t ulStoppedTimerCompensation = 0;
#endif /* configUSE_TICKLESS_IDLE */

/*
 * 用于 portASSERT_IF_INTERRUPT_PRIORITY_INVALID() 宏，确保不会从优先级高于
 * configMAX_SYSCALL_INTERRUPT_PRIORITY 的中断调用 FreeRTOS API 函数
 */
#if ( configASSERT_DEFINED == 1 )
    static uint8_t ucMaxSysCallPriority = 0;
    static uint32_t ulMaxPRIGROUPValue = 0;
    static const volatile uint8_t * const pcInterruptPriorityRegisters = ( uint8_t * ) portNVIC_IP_REGISTERS_OFFSET_16;
#endif

/*-----------------------------------------------------------*/

/*
 * 初始化任务栈。模拟上下文切换中断生成的栈帧。
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    /* 模拟上下文切换中断生成的栈帧结构 */
    pxTopOfStack--;                                                      /* 调整栈顶指针以匹配MCU中断入栈方式 */
    *pxTopOfStack = portINITIAL_XPSR;                                    /* 初始xPSR寄存器值 */
    pxTopOfStack--;
    *pxTopOfStack = ( ( StackType_t ) pxCode ) & portSTART_ADDRESS_MASK; /* 任务入口地址（PC），确保bit0清零 */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) prvTaskExitError;                    /* 任务返回地址（LR），指向错误处理函数 */

    pxTopOfStack -= 5;                                                   /* 为R12, R3, R2, R1预留空间 */
    *pxTopOfStack = ( StackType_t ) pvParameters;                        /* R0寄存器存放任务参数 */
    pxTopOfStack -= 8;                                                   /* 为R11-R4寄存器预留空间 */

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

/*
 * 任务非法退出处理函数。当任务试图从函数返回时触发。
 */
static void prvTaskExitError( void )
{
    /* 任务函数禁止直接返回，应通过vTaskDelete()终止自己。
     * 若配置了configASSERT()，强制触发断言以便调试 */
    configASSERT( uxCriticalNesting == ~0UL );
    portDISABLE_INTERRUPTS(); /* 关闭中断 */

    for( ; ; ) /* 死循环阻塞 */
    {
    }
}
/*-----------------------------------------------------------*/

/* SVC中断处理函数，用于启动第一个任务 */
__asm void vPortSVCHandler( void )
{
/* *INDENT-OFF* */
    PRESERVE8          /* 保证8字节栈对齐 */

    ldr r3, = pxCurrentTCB   /* 加载当前任务控制块地址 */
    ldr r1, [ r3 ]      /* 获取pxCurrentTCB指针值 */
    ldr r0, [ r1 ]           /* 从TCB首个成员获取任务栈顶指针 */
    ldmia r0 !, { r4 - r11 } /* 恢复R4-R11寄存器及临界嵌套计数 */
    msr psp, r0        /* 设置进程栈指针PSP */
    isb                 /* 指令同步屏障 */
    mov r0, # 0         /* 清零BASEPRI寄存器 */
    msr basepri, r0
    orr r14, # 0xd      /* 设置LR第2、3位，返回后使用PSP栈且进入线程模式 */
    bx r14              /* 返回并启动任务上下文 */
/* *INDENT-ON* */
}
/*-----------------------------------------------------------*/

/* 启动第一个任务的汇编函数 */
__asm void prvStartFirstTask( void )
{
/* *INDENT-OFF* */
    PRESERVE8

    /* 通过NVIC向量表偏移寄存器定位主栈指针 */
    ldr r0, =0xE000ED08  /* VTOR寄存器地址 */
    ldr r0, [ r0 ]       /* 获取向量表起始地址 */
    ldr r0, [ r0 ]       /* 首个向量是初始MSP值 */

    msr msp, r0          /* 重置主栈指针MSP */
    cpsie i              /* 全局使能中断 */
    cpsie f
    dsb                  /* 数据同步屏障 */
    isb
    svc 0                /* 触发SVC异常启动首个任务 */
    nop                  /* 填充指令对齐 */
    nop
/* *INDENT-ON* */
}
/*-----------------------------------------------------------*/

/*
 * 启动调度器主函数。初始化系统关键资源并启动首个任务。
 */
BaseType_t xPortStartScheduler( void )
{
    #if ( configASSERT_DEFINED == 1 )
    {
        volatile uint32_t ulOriginalPriority;
        volatile uint8_t * const pucFirstUserPriorityRegister = ( uint8_t * ) ( portNVIC_IP_REGISTERS_OFFSET_16 + portFIRST_USER_INTERRUPT_NUMBER );
        volatile uint8_t ucMaxPriorityValue;

        /* 确定允许调用FreeRTOS API的最高中断优先级 */
        ulOriginalPriority = *pucFirstUserPriorityRegister; /* 保存原优先级值 */

        /* 写入全1值检测实际有效优先级位数 */
        *pucFirstUserPriorityRegister = portMAX_8_BIT_VALUE;
        ucMaxPriorityValue = *pucFirstUserPriorityRegister; /* 读取实际有效位数 */

        /* 验证内核中断优先级为最低优先级 */
        configASSERT( ucMaxPriorityValue == ( configKERNEL_INTERRUPT_PRIORITY & ucMaxPriorityValue ) );

        /* 计算系统调用允许的最高优先级掩码 */
        ucMaxSysCallPriority = configMAX_SYSCALL_INTERRUPT_PRIORITY & ucMaxPriorityValue;

        /* 确定与优先级位数对应的优先级分组方案 */
        ulMaxPRIGROUPValue = portMAX_PRIGROUP_BITS;
        while( ( ucMaxPriorityValue & portTOP_BIT_OF_BYTE ) == portTOP_BIT_OF_BYTE )
        {
            ulMaxPRIGROUPValue--;
            ucMaxPriorityValue <<= 1; /* 左移直到最高位非1 */
        }

        /* 验证CMSIS优先级位数配置一致性 */
        #ifdef __NVIC_PRIO_BITS
            configASSERT( ( portMAX_PRIGROUP_BITS - ulMaxPRIGROUPValue ) == __NVIC_PRIO_BITS );
        #endif

        /* 恢复被修改的中断优先级寄存器 */
        *pucFirstUserPriorityRegister = ulOriginalPriority;
    }
    #endif /* configASSERT_DEFINED */

    /* 配置PendSV和SysTick为最低优先级中断 */
    portNVIC_SHPR3_REG |= portNVIC_PENDSV_PRI;
    portNVIC_SHPR3_REG |= portNVIC_SYSTICK_PRI;

    /* 初始化系统节拍定时器 */
    vPortSetupTimerInterrupt();

    /* 初始化临界区嵌套计数器 */
    uxCriticalNesting = 0;

    /* 启动首个任务（永不返回） */
    prvStartFirstTask();

    return 0; /* 理论上不应执行至此 */
}
/*-----------------------------------------------------------*/

/* 终止调度器函数（Cortex-M端口通常无需实现） */
void vPortEndScheduler( void )
{
    /* 强制触发断言，因该函数在嵌入式场景不应被调用 */
    configASSERT( uxCriticalNesting == 1000UL );
}
/*-----------------------------------------------------------*/

/* 进入临界区函数 */
void vPortEnterCritical( void )
{
    portDISABLE_INTERRUPTS();  /* 关闭中断 */
    uxCriticalNesting++;       /* 增加嵌套计数 */

    /* 非中断安全版本临界区函数禁止在中断中调用 */
    if( uxCriticalNesting == 1 )
    {
        configASSERT( ( portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK ) == 0 );
    }
}
/*-----------------------------------------------------------*/

/* 退出临界区函数 */
void vPortExitCritical( void )
{
    configASSERT( uxCriticalNesting ); /* 确保临界区嵌套计数不为零 */
    uxCriticalNesting--;               /* 减少嵌套计数 */

    if( uxCriticalNesting == 0 )
    {
        portENABLE_INTERRUPTS(); /* 恢复中断使能 */
    }
}
/*-----------------------------------------------------------*/

/* PendSV中断处理函数（上下文切换核心） */
__asm void xPortPendSVHandler( void )
{
    extern uxCriticalNesting;
    extern pxCurrentTCB;
    extern vTaskSwitchContext;

/* *INDENT-OFF* */
    PRESERVE8

    mrs r0, psp         /* 获取当前任务栈指针 */
    isb                 /* 指令同步屏障 */

    ldr r3, =pxCurrentTCB
    ldr r2, [ r3 ]      /* 加载当前TCB地址 */

    stmdb r0 !, { r4 - r11 } /* 保存R4-R11到任务栈 */
    str r0, [ r2 ]      /* 更新TCB中的栈顶指针 */

    stmdb sp !, { r3, r14 } /* 临时保存R3和LR到系统栈 */
    mov r0, #configMAX_SYSCALL_INTERRUPT_PRIORITY
    msr basepri, r0     /* 抬高BASEPRI屏蔽低优先级中断 */
    dsb
    isb
    bl vTaskSwitchContext /* 执行任务切换 */
    mov r0, #0
    msr basepri, r0     /* 恢复BASEPRI */
    ldmia sp !, { r3, r14 } /* 恢复寄存器 */

    ldr r1, [ r3 ]      /* 获取新任务TCB地址 */
    ldr r0, [ r1 ]           /* 加载新任务栈顶指针 */
    ldmia r0 !, { r4 - r11 } /* 恢复新任务的R4-R11 */
    msr psp, r0         /* 更新进程栈指针 */
    isb
    bx r14              /* 返回新任务上下文 */
    nop
/* *INDENT-ON* */
}

/*
 * SysTick 中断服务例程（ISR），用于驱动 FreeRTOS 的 tick 事件。
 * SysTick 运行在最低的中断优先级，因此当该中断执行时，所有中断必须处于未屏蔽状态。
 * 因此无需保存并恢复中断屏蔽值（因为其值已知），此处使用更高效的 vPortRaiseBASEPRI()
 * 替代 portSET_INTERRUPT_MASK_FROM_ISR()。
 */
void xPortSysTickHandler( void )
{
    vPortRaiseBASEPRI();
    {
        /* 递增 RTOS 节拍计数器 */
        if( xTaskIncrementTick() != pdFALSE )
        {
            /* 需要执行上下文切换。上下文切换在 PendSV 中断中处理。触发 PendSV 中断 */
            portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
        }
    }

    vPortClearBASEPRIFromISR();
}
/*-----------------------------------------------------------*/

#if ( configUSE_TICKLESS_IDLE == 1 )

    /*
     * 低功耗 tickless 空闲模式实现。该函数会暂时停止 SysTick 计数器，
     * 并根据预计的空闲时间重新配置 SysTick 以唤醒系统。
     */
    __weak void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
    {
        uint32_t ulReloadValue, ulCompleteTickPeriods, ulCompletedSysTickDecrements, ulSysTickDecrementsLeft;
        TickType_t xModifiableIdleTime;

        /* 确保 SysTick 重载值不会导致计数器溢出 */
        if( xExpectedIdleTime > xMaximumPossibleSuppressedTicks )
        {
            xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
        }

        /* 进入临界区，但不使用 taskENTER_CRITICAL() 方法，因为该操作会屏蔽应退出睡眠模式的中断 */
        __disable_irq();
        __dsb( portSY_FULL_READ_WRITE );
        __isb( portSY_FULL_READ_WRITE );

        /* 如果有上下文切换挂起或任务等待调度器恢复，则放弃低功耗进入 */
        if( eTaskConfirmSleepModeStatus() == eAbortSleep )
        {
            /* 重新启用中断 - 参见上方 __disable_irq() 的注释 */
            __enable_irq();
        }
        else
        {
            /* 暂时停止 SysTick。虽然内核维护的时间与日历时间之间会有微小偏差，
               但 tickless 模式仍能最大程度减少误差 */
            portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT );

            /* 使用 SysTick 当前值寄存器确定到下一个 tick 中断剩余的递减次数。
               如果当前值寄存器为 0，表示实际剩余 ulTimerCountsForOneTick 次递减 */
            ulSysTickDecrementsLeft = portNVIC_SYSTICK_CURRENT_VALUE_REG;

            if( ulSysTickDecrementsLeft == 0 )
            {
                ulSysTickDecrementsLeft = ulTimerCountsForOneTick;
            }

            /* 计算等待 xExpectedIdleTime 个 tick 周期所需的重载值。-1 是因为此代码通常在
               第一个 tick 周期中途执行。如果 SysTick IRQ 现在挂起，则清除 IRQ（抑制第一个 tick），
               并调整重载值以反映第二个 tick 周期已开始。预计空闲时间至少为两个 tick */
            ulReloadValue = ulSysTickDecrementsLeft + ( ulTimerCountsForOneTick * ( xExpectedIdleTime - 1UL ) );

            if( ( portNVIC_INT_CTRL_REG & portNVIC_PEND_SYSTICK_SET_BIT ) != 0 )
            {
                portNVIC_INT_CTRL_REG = portNVIC_PEND_SYSTICK_CLEAR_BIT;
                ulReloadValue -= ulTimerCountsForOneTick;
            }

            /* 应用停止期间的时间补偿 */
            if( ulReloadValue > ulStoppedTimerCompensation )
            {
                ulReloadValue -= ulStoppedTimerCompensation;
            }

            /* 设置新的重载值 */
            portNVIC_SYSTICK_LOAD_REG = ulReloadValue;

            /* 清除 SysTick 计数标志并将计数值重置为零 */
            portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

            /* 重启 SysTick */
            portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;

            /* 进入睡眠直到有事件触发。configPRE_SLEEP_PROCESSING() 可设置参数为 0，
               表示其实现包含自己的等待中断指令（WFI），因此不再执行 WFI。
               但原始预计空闲时间变量必须保持不变，故使用副本 */
            xModifiableIdleTime = xExpectedIdleTime;
            configPRE_SLEEP_PROCESSING( xModifiableIdleTime );

            if( xModifiableIdleTime > 0 )
            {
                __dsb( portSY_FULL_READ_WRITE );
                __wfi();
                __isb( portSY_FULL_READ_WRITE );
            }

            configPOST_SLEEP_PROCESSING( xExpectedIdleTime );

            /* 重新启用中断，允许将 MCU 从睡眠模式唤醒的中断立即执行 */
            __enable_irq();
            __dsb( portSY_FULL_READ_WRITE );
            __isb( portSY_FULL_READ_WRITE );

            /* 再次禁用中断，因为时钟即将停止，此时执行的中断会增加内核时间与日历时间的偏差 */
            __disable_irq();
            __dsb( portSY_FULL_READ_WRITE );
            __isb( portSY_FULL_READ_WRITE );

            /* 停止 SysTick 时钟，不读取 portNVIC_SYSTICK_CTRL_REG 以防止清除计数标志 */
            portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT );

            /* 检查 SysTick 是否已递减至零 */
            if( ( portNVIC_SYSTICK_CTRL_REG & portNVIC_SYSTICK_COUNT_FLAG_BIT ) != 0 )
            {
                uint32_t ulCalculatedLoadValue;

                /* 新的 tick 周期已开始。用剩余时间重置重载值 */
                ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL ) - ( ulReloadValue - portNVIC_SYSTICK_CURRENT_VALUE_REG );

                /* 防止因后睡眠钩子操作耗时过长或当前值寄存器为零导致的极小值或下溢 */
                if( ( ulCalculatedLoadValue <= ulStoppedTimerCompensation ) || ( ulCalculatedLoadValue > ulTimerCountsForOneTick ) )
                {
                    ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL );
                }

                portNVIC_SYSTICK_LOAD_REG = ulCalculatedLoadValue;

                /* 当此函数退出时，挂起的 tick 将被处理，因此步进已等待时间的 tick 值 */
                ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
            }
            else
            {
                /* 非 tick 中断唤醒了睡眠 */

                /* 使用当前值寄存器确定预计空闲时间结束前剩余的递减次数 */
                ulSysTickDecrementsLeft = portNVIC_SYSTICK_CURRENT_VALUE_REG;
                #if ( portNVIC_SYSTICK_CLK_BIT_CONFIG != portNVIC_SYSTICK_CLK_BIT )
                {
                    /* 如果 SysTick 未使用核心时钟，当前值寄存器可能仍为零 */
                    if( ulSysTickDecrementsLeft == 0 )
                    {
                        ulSysTickDecrementsLeft = ulReloadValue;
                    }
                }
                #endif /* portNVIC_SYSTICK_CLK_BIT_CONFIG */

                /* 计算睡眠持续时间对应的完整 tick 周期数 */
                ulCompletedSysTickDecrements = ( xExpectedIdleTime * ulTimerCountsForOneTick ) - ulSysTickDecrementsLeft;

                /* 计算经过的完整 tick 周期数 */
                ulCompleteTickPeriods = ulCompletedSysTickDecrements / ulTimerCountsForOneTick;

                /* 设置剩余部分 tick 周期的重载值 */
                portNVIC_SYSTICK_LOAD_REG = ( ( ulCompleteTickPeriods + 1UL ) * ulTimerCountsForOneTick ) - ulCompletedSysTickDecrements;
            }

            /* 重启 SysTick 并从标准值加载 */
            portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;
            portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT;
            #if ( portNVIC_SYSTICK_CLK_BIT_CONFIG == portNVIC_SYSTICK_CLK_BIT )
            {
                portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;
            }
            #else
            {
                /* 恢复使用其他时钟源 */
                portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT;

                if( ( portNVIC_SYSTICK_CTRL_REG & portNVIC_SYSTICK_COUNT_FLAG_BIT ) != 0 )
                {
                    portNVIC_SYSTICK_CURRENT_VALUE_REG = 0;
                }

                portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;
                portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT;
            }
            #endif /* portNVIC_SYSTICK_CLK_BIT_CONFIG */

            /* 步进 tick 计数器以反映经过的完整周期 */
            vTaskStepTick( ulCompleteTickPeriods );

            /* 启用中断并退出 */
            __enable_irq();
        }
    }

#endif /* #if configUSE_TICKLESS_IDLE */

/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

/*
 * 配置 SysTick 定时器以生成指定频率的节拍中断
 */
#if ( configOVERRIDE_DEFAULT_TICK_CONFIGURATION == 0 )

    /* 弱定义函数，允许应用层覆盖默认的定时器配置 */
    __weak void vPortSetupTimerInterrupt( void )
    {
        /* 计算配置节拍中断所需的常量 */
        #if ( configUSE_TICKLESS_IDLE == 1 )
        {
            /* 计算单个 tick 周期对应的定时器计数值 */
            ulTimerCountsForOneTick = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ );
            /* 计算允许抑制的最大 tick 数（基于24位计数器） */
            xMaximumPossibleSuppressedTicks = portMAX_24_BIT_NUMBER / ulTimerCountsForOneTick;
            /* 计算定时器停止期间的补偿因子 */
            ulStoppedTimerCompensation = portMISSED_COUNTS_FACTOR / ( configCPU_CLOCK_HZ / configSYSTICK_CLOCK_HZ );
        }
        #endif /* configUSE_TICKLESS_IDLE */

        /* 停止并复位 SysTick 定时器 */
        portNVIC_SYSTICK_CTRL_REG = 0UL;
        portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

        /* 按指定频率配置 SysTick 中断 */
        portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
        /* 启用 SysTick（配置时钟源、中断使能、计数器启动） */
        portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT );
    }

#endif /* configOVERRIDE_DEFAULT_TICK_CONFIGURATION */
/*-----------------------------------------------------------*/

/* 汇编函数：获取当前中断状态寄存器(IPSR)值 */
__asm uint32_t vPortGetIPSR( void )
{
/* *INDENT-OFF* */
    PRESERVE8

    mrs r0, ipsr
    bx r14
/* *INDENT-ON* */
}
/*-----------------------------------------------------------*/

#if ( configASSERT_DEFINED == 1 )

    /* 中断优先级验证函数（断言检查） */
    void vPortValidateInterruptPriority( void )
    {
        uint32_t ulCurrentInterrupt;
        uint8_t ucCurrentPriority;

        /* 获取当前执行的中断号 */
        ulCurrentInterrupt = vPortGetIPSR();

        /* 仅验证用户定义的中断 */
        if( ulCurrentInterrupt >= portFIRST_USER_INTERRUPT_NUMBER )
        {
            /* 从优先级寄存器获取当前中断优先级 */
            ucCurrentPriority = pcInterruptPriorityRegisters[ ulCurrentInterrupt ];

            /* 验证中断优先级是否符合规范：
               1. 使用 FreeRTOS API 的中断优先级必须 <= configMAX_SYSCALL_INTERRUPT_PRIORITY
               2. 数值型优先级越低表示逻辑优先级越高
               3. 禁止使用默认优先级 0（必须显式设置有效优先级） */
            configASSERT( ucCurrentPriority >= ucMaxSysCallPriority );
        }

        /* 验证优先级分组设置：
           所有优先级位必须用于抢占优先级（无子优先级位）
           使用 CMSIS 库时需调用 NVIC_SetPriorityGrouping(0) */
        configASSERT( ( portAIRCR_REG & portPRIORITY_GROUP_MASK ) <= ulMaxPRIGROUPValue );
    }

#endif /* configASSERT_DEFINED */
