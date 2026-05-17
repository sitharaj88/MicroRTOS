/**
 * MicroRTOS - Main Public API Header
 *
 * Include this single header to access all MicroRTOS functionality.
 *
 * MicroRTOS is an enterprise-grade, lightweight RTOS for Arduino
 * platforms supporting both AVR (Uno, Mega, Nano) and ARM Cortex-M
 * (Due, Zero, MKR) microcontrollers.
 *
 * Features:
 * - Hybrid scheduling (preemptive + cooperative + time-slicing)
 * - Priority inheritance mutexes
 * - Binary and counting semaphores
 * - Thread-safe message queues
 * - Software timers (one-shot and periodic)
 * - Event groups for task synchronization
 * - Fixed-size memory pools
 * - Task notifications
 * - Runtime statistics and diagnostics
 *
 * Standards Compliance:
 * - MISRA C:2012 (Required + Mandatory Rules)
 * - IEC 61508 SIL 1-4
 * - ISO 26262 ASIL A-D
 * - CERT C Coding Standard
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef MR_H
#define MR_H

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Version Information                                                        */
/*===========================================================================*/

#define MR_VERSION_MAJOR      1
#define MR_VERSION_MINOR      0
#define MR_VERSION_PATCH      0
#define MR_VERSION_STRING     "1.0.0"

/*===========================================================================*/
/* Core Headers                                                               */
/*===========================================================================*/

#include "mr_config.h"
#include "mr_types.h"
#include "mr_port.h"
#include "mr_list.h"
#include "mr_task.h"

/*===========================================================================*/
/* Feature Headers                                                            */
/*===========================================================================*/

#if MR_USE_MUTEXES
#include "mr_mutex.h"
#endif

#if MR_USE_SEMAPHORES
#include "mr_semaphore.h"
#endif

#if MR_USE_QUEUES
#include "mr_queue.h"
#endif

#if MR_USE_TIMERS
#include "mr_timer.h"
#endif

#if MR_USE_EVENT_GROUPS
#include "mr_event.h"
#endif

#if MR_USE_MEMORY_POOLS
#include "mr_memory.h"
#endif

#if MR_USE_TICKLESS_IDLE
#include "mr_tickless.h"
#endif

#if MR_SAFETY_ENABLE
#include "mr_safety.h"
#endif

/*===========================================================================*/
/* Kernel API                                                                 */
/*===========================================================================*/

/**
 * Initialize the RTOS kernel.
 * Must be called before any other RTOS functions.
 */
void mr_kernel_init(void);

/**
 * Start the RTOS scheduler.
 * This function does not return. The first ready task will begin execution.
 */
void mr_kernel_start(void);

/**
 * Get the current system tick count.
 *
 * @return Current tick count since kernel start
 */
uint32_t mr_tick_get(void);

/**
 * Get the current kernel state.
 *
 * @return Kernel state (not started, running, or suspended)
 */
mr_kernel_state_t mr_kernel_get_state(void);

/**
 * Check if currently executing in ISR context.
 *
 * @return true if in ISR, false if in task context
 */
bool mr_is_in_isr(void);

/**
 * Get the number of tasks ready to run.
 *
 * @return Number of ready tasks
 */
uint16_t mr_get_ready_task_count(void);

/*===========================================================================*/
/* Scheduler Control                                                          */
/*===========================================================================*/

/**
 * Suspend the scheduler (prevent context switches).
 * Can be nested - scheduler resumes when resume count matches suspend count.
 */
void mr_scheduler_suspend(void);

/**
 * Resume the scheduler.
 * A pending context switch will occur if one was requested while suspended.
 */
void mr_scheduler_resume(void);

/*===========================================================================*/
/* Critical Section API                                                       */
/*===========================================================================*/

/**
 * Enter a critical section (disable interrupts).
 * Supports nesting - interrupts only re-enable when nesting count is zero.
 */
void mr_critical_enter(void);

/**
 * Exit a critical section.
 */
void mr_critical_exit(void);

/*===========================================================================*/
/* Runtime Statistics (if enabled)                                            */
/*===========================================================================*/

#if MR_USE_RUNTIME_STATS

/**
 * Get CPU idle percentage (0-100).
 *
 * @return Percentage of time spent in idle task
 */
uint8_t mr_get_idle_percent(void);

/**
 * Reset runtime statistics counters.
 */
void mr_reset_runtime_stats(void);

#endif /* MR_USE_RUNTIME_STATS */

/*===========================================================================*/
/* User Hooks (to be implemented by user if enabled)                          */
/*===========================================================================*/

#if MR_USE_IDLE_HOOK
/**
 * User idle hook - called repeatedly from idle task.
 * Can be used for power management or background processing.
 */
extern void mr_idle_hook(void);
#endif

#if MR_USE_TICK_HOOK
/**
 * User tick hook - called from tick interrupt.
 * Keep this function short as it runs in ISR context.
 */
extern void mr_tick_hook(void);
#endif

#if MR_CHECK_STACK_OVERFLOW
/**
 * Stack overflow hook - called when overflow is detected.
 * System will halt after this function is called.
 */
extern void mr_stack_overflow_hook(mr_tcb_t *tcb);
#endif

/*===========================================================================*/
/* Convenience Macros                                                         */
/*===========================================================================*/

/**
 * Define a task with static stack.
 * Usage: MR_TASK_DEFINE(my_task, 256);
 */
#define MR_TASK_DEFINE(name, stack_size) \
    static mr_tcb_t name##_tcb; \
    static uint8_t name##_stack[stack_size]

/**
 * Create a task from a definition.
 * Usage: MR_TASK_CREATE(my_task, task_func, NULL, 2);
 */
#define MR_TASK_CREATE(name, func, arg, priority) \
    mr_task_create(&name##_tcb, #name, func, arg, priority, \
                     name##_stack, sizeof(name##_stack))

/**
 * Define a message queue with static buffer.
 * Usage: MR_QUEUE_DEFINE(my_queue, sizeof(Message), 10);
 */
#define MR_QUEUE_DEFINE(name, item_size, capacity) \
    static mr_queue_t name##_queue; \
    static uint8_t name##_buffer[(item_size) * (capacity)]

/**
 * Initialize a queue from a definition.
 */
#define MR_QUEUE_INIT(name, item_size, capacity) \
    mr_queue_init(&name##_queue, name##_buffer, item_size, capacity)

/**
 * Define a memory pool with static buffer.
 * Usage: MR_MEMPOOL_DEFINE(my_pool, 32, 10);
 */
#define MR_MEMPOOL_DEFINE(name, block_size, block_count) \
    static mr_mempool_t name##_pool; \
    static uint8_t name##_buffer[(block_size) * (block_count)]

/**
 * Initialize a memory pool from a definition.
 */
#define MR_MEMPOOL_INIT(name, block_size, block_count) \
    mr_mempool_init(&name##_pool, name##_buffer, block_size, block_count)

#ifdef __cplusplus
}
#endif

#endif /* MR_H */
