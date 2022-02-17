/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-07-24     Tanek        the first version
 * 2018-11-12     Ernest Chen  modify copyright
 */
 
#include <rthw.h>
#include <rtthread.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h> /* for timeofday */
#include "compiler.h"
#include "driver/chip/system_chip.h"
#include "driver/chip/hal_global.h"
#include "driver/chip/hal_gpio.h"
// #include "common/board/board.h"
#include "sys/sram_heap.h"

#ifndef PRJCONF_GPIO_PORT_IRQ_USED
#define PRJCONF_GPIO_PORT_IRQ_USED      0xff
#endif
/* gpio port bitmask for pm backup */
#ifndef PRJCONF_GPIO_PORT_PM_BACKUP
#define PRJCONF_GPIO_PORT_PM_BACKUP     0xff
#endif

#define _SCB_BASE       (0xE000E010UL)
#define _SYSTICK_CTRL   (*(rt_uint32_t *)(_SCB_BASE + 0x0))
#define _SYSTICK_LOAD   (*(rt_uint32_t *)(_SCB_BASE + 0x4))
#define _SYSTICK_VAL    (*(rt_uint32_t *)(_SCB_BASE + 0x8))
#define _SYSTICK_CALIB  (*(rt_uint32_t *)(_SCB_BASE + 0xC))
#define _SYSTICK_PRI    (*(rt_uint8_t  *)(0xE000ED23UL))

// Updates the variable SystemCoreClock and must be called 
// whenever the core clock is changed during program execution.
extern void SystemCoreClockUpdate(void);

// Holds the system core clock, which is the system clock 
// frequency supplied to the SysTick timer and the processor 
// core clock.
extern uint32_t SystemCoreClock;

static uint32_t _SysTick_Config(rt_uint32_t ticks)
{
    if ((ticks - 1) > 0xFFFFFF)
    {
        return 1;
    }
    
    _SYSTICK_LOAD = ticks - 1; 
    _SYSTICK_PRI = 0xFF;
    _SYSTICK_VAL  = 0;
    _SYSTICK_CTRL = 0x07;  
    
    return 0;
}

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
#define RT_HEAP_SIZE 1024
// static uint32_t rt_heap[RT_HEAP_SIZE];     // heap default size: 4K(1024 * 4)
extern uint8_t __heap_start__[];
extern uint8_t __heap_end__[];
RT_WEAK void *rt_heap_begin_get(void)
{
    // return rt_heap;
    return __heap_start__;
}

RT_WEAK void *rt_heap_end_get(void)
{
    // return rt_heap + RT_HEAP_SIZE;
    return __heap_end__;
}
#endif

/**
 * This function will initial your board.
 */
void rt_hw_board_init()
{
#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
    rt_system_heap_init(rt_heap_begin_get(), rt_heap_end_get());
#endif
    extern void RT_systeminit(void);
    RT_systeminit();
    /* System Tick Configuration */
    _SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND);
}

void SysTick_Handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();
}

void OsTickHandler()
{
    SysTick_Handler();
}
