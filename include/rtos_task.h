/**
 * MicroRTOS - Task Management API
 *
 * This file provides the task management interface including
 * task creation, deletion, suspension, and delay functions.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef RTOS_TASK_H
#define RTOS_TASK_H

#include "rtos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Task Function Type                                                         */
/*===========================================================================*/

/**
 * Task entry function signature.
 *
 * @param arg User-provided argument passed to the task
 */
typedef void (*rtos_task_func_t)(void *arg);

/*===========================================================================*/
/* Task Creation and Deletion                                                 */
/*===========================================================================*/

/**
 * Create and start a new task.
 *
 * The task is added to the ready list and will run when it becomes
 * the highest priority ready task.
 *
 * @param tcb        Pointer to pre-allocated Task Control Block
 * @param name       Task name for debugging (can be NULL if RTOS_USE_TASK_NAMES=0)
 * @param entry      Task entry function
 * @param arg        Argument passed to task function
 * @param priority   Task priority (0 = highest, RTOS_MAX_PRIORITIES-1 = lowest)
 * @param stack      Pointer to pre-allocated stack memory
 * @param stack_size Size of stack in bytes
 *
 * @return RTOS_OK on success, error code otherwise
 */
rtos_status_t rtos_task_create(
    rtos_tcb_t *tcb,
    const char *name,
    rtos_task_func_t entry,
    void *arg,
    uint8_t priority,
    void *stack,
    uint16_t stack_size
);

/**
 * Delete a task and release its resources.
 *
 * If the task holds any mutexes, they are released with priority
 * restoration. The task's TCB and stack can be reused after deletion.
 *
 * @param tcb Pointer to TCB, or NULL to delete current task
 *
 * @return RTOS_OK on success, error code otherwise
 */
rtos_status_t rtos_task_delete(rtos_tcb_t *tcb);

/*===========================================================================*/
/* Task Control                                                               */
/*===========================================================================*/

/**
 * Suspend a task.
 *
 * A suspended task will not run until resumed. If the task is
 * blocked on a resource, it remains blocked and suspended.
 *
 * @param tcb Pointer to TCB, or NULL to suspend current task
 *
 * @return RTOS_OK on success, error code otherwise
 */
rtos_status_t rtos_task_suspend(rtos_tcb_t *tcb);

/**
 * Resume a suspended task.
 *
 * The task is moved back to the ready state (or blocked state
 * if it was blocked when suspended).
 *
 * @param tcb Pointer to TCB
 *
 * @return RTOS_OK on success, error code otherwise
 */
rtos_status_t rtos_task_resume(rtos_tcb_t *tcb);

/**
 * Voluntarily yield the CPU to another ready task.
 *
 * If another task of equal or higher priority is ready, it will
 * run. Otherwise, the current task continues.
 */
void rtos_task_yield(void);

/*===========================================================================*/
/* Task Delays                                                                */
/*===========================================================================*/

/**
 * Delay the current task for a specified number of ticks.
 *
 * The task will not run for at least the specified number of
 * system ticks. Actual delay may be slightly longer.
 *
 * @param ticks Number of ticks to delay (use RTOS_MS_TO_TICKS)
 */
void rtos_task_delay(uint32_t ticks);

/**
 * Delay until an absolute tick count.
 *
 * This provides more precise periodic timing than rtos_task_delay
 * by accounting for the task's execution time.
 *
 * Usage:
 *   uint32_t last_wake = rtos_tick_get();
 *   while (1) {
 *       rtos_task_delay_until(&last_wake, RTOS_MS_TO_TICKS(100));
 *       // Execute periodic code
 *   }
 *
 * @param prev_wake Pointer to variable holding previous wake tick
 * @param increment Number of ticks between wake times
 */
void rtos_task_delay_until(uint32_t *prev_wake, uint32_t increment);

/*===========================================================================*/
/* Task Queries                                                               */
/*===========================================================================*/

/**
 * Get the current running task's TCB.
 *
 * @return Pointer to current task's TCB
 */
rtos_tcb_t *rtos_task_get_current(void);

/**
 * Get a task's current priority.
 *
 * @param tcb Pointer to TCB, or NULL for current task
 *
 * @return Task's current priority
 */
uint8_t rtos_task_get_priority(rtos_tcb_t *tcb);

/**
 * Set a task's priority.
 *
 * If the task is running and the new priority is lower than
 * another ready task, a context switch may occur.
 *
 * @param tcb      Pointer to TCB, or NULL for current task
 * @param priority New priority (0 = highest)
 *
 * @return RTOS_OK on success, error code otherwise
 */
rtos_status_t rtos_task_set_priority(rtos_tcb_t *tcb, uint8_t priority);

/**
 * Get a task's current state.
 *
 * @param tcb Pointer to TCB
 *
 * @return Task state
 */
rtos_task_state_t rtos_task_get_state(rtos_tcb_t *tcb);

/**
 * Get a task's name.
 *
 * @param tcb Pointer to TCB, or NULL for current task
 *
 * @return Task name, or "?" if names are disabled
 */
const char *rtos_task_get_name(rtos_tcb_t *tcb);

/**
 * Get a task's stack high water mark.
 *
 * Returns the minimum free stack space that has ever existed for
 * the task. Useful for tuning stack sizes.
 *
 * @param tcb Pointer to TCB, or NULL for current task
 *
 * @return Minimum free stack bytes
 */
uint16_t rtos_task_get_stack_high_water(rtos_tcb_t *tcb);

/*===========================================================================*/
/* Task Notifications                                                         */
/*===========================================================================*/

#if RTOS_USE_TASK_NOTIFICATIONS

/**
 * Wait for a task notification.
 *
 * @param clear_on_exit If true, clear notification value on exit
 * @param value         Pointer to store notification value (can be NULL)
 * @param timeout       Maximum ticks to wait (RTOS_WAIT_FOREVER, RTOS_NO_WAIT)
 *
 * @return RTOS_OK if notified, RTOS_ERR_TIMEOUT if timeout occurred
 */
rtos_status_t rtos_task_notify_wait(
    bool clear_on_exit,
    uint32_t *value,
    uint32_t timeout
);

/**
 * Send a notification to a task.
 *
 * @param tcb    Target task TCB
 * @param value  Notification value
 * @param action How to apply the value (set, increment, set bits)
 *
 * @return RTOS_OK on success
 */
rtos_status_t rtos_task_notify(
    rtos_tcb_t *tcb,
    uint32_t value,
    rtos_notify_action_t action
);

/**
 * Send a notification from an ISR.
 *
 * @param tcb             Target task TCB
 * @param value           Notification value
 * @param action          How to apply the value
 * @param yield_required  Set to true if a context switch should occur
 *
 * @return RTOS_OK on success
 */
rtos_status_t rtos_task_notify_from_isr(
    rtos_tcb_t *tcb,
    uint32_t value,
    rtos_notify_action_t action,
    bool *yield_required
);

#endif /* RTOS_USE_TASK_NOTIFICATIONS */

/*===========================================================================*/
/* Runtime Statistics                                                         */
/*===========================================================================*/

#if RTOS_USE_RUNTIME_STATS

/**
 * Get a task's accumulated runtime in ticks.
 *
 * @param tcb Pointer to TCB, or NULL for current task
 *
 * @return Runtime in system ticks
 */
uint32_t rtos_task_get_runtime(rtos_tcb_t *tcb);

#endif

#ifdef __cplusplus
}
#endif

#endif /* RTOS_TASK_H */
