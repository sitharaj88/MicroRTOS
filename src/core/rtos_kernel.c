/**
 * MicroRTOS - Kernel Implementation
 *
 * This file implements kernel initialization, tick handling,
 * the idle task, and global kernel state management.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos_types.h"
#include "rtos_list.h"
#include "rtos_task.h"
#include "rtos_port.h"
#include <string.h>

/*===========================================================================*/
/* Global Kernel State                                                        */
/*===========================================================================*/

/* Current running task */
rtos_tcb_t *g_current_tcb = NULL;

/* Ready lists - one per priority level */
rtos_list_t g_ready_list[RTOS_MAX_PRIORITIES];

/* Delayed task list (sorted by wake time) */
rtos_list_t g_delayed_list;

/* System tick counter */
volatile uint32_t g_tick_count = 0;

/* Kernel state */
volatile rtos_kernel_state_t g_kernel_state = RTOS_KERNEL_NOT_STARTED;

/* Scheduler suspension counter */
volatile uint8_t g_scheduler_suspended = 0;

/* Pending yield flag */
volatile bool g_yield_pending = false;

/* Bitmap of priorities with ready tasks */
volatile uint32_t g_ready_priorities = 0;

/* Critical section nesting counter */
volatile uint8_t g_critical_nesting = 0;

/*===========================================================================*/
/* Idle Task                                                                  */
/*===========================================================================*/

static rtos_tcb_t g_idle_tcb;
static uint8_t g_idle_stack[RTOS_IDLE_STACK_SIZE];

#if RTOS_USE_RUNTIME_STATS
static volatile uint32_t g_idle_runtime = 0;
static volatile uint32_t g_total_runtime = 0;
#endif

/**
 * Idle task function.
 * Runs when no other task is ready. Can be used for power management.
 */
static void idle_task(void *arg)
{
    (void)arg;

    while (1) {
#if RTOS_USE_IDLE_HOOK
        /* User-defined idle hook */
        extern void rtos_idle_hook(void);
        rtos_idle_hook();
#endif

        /* Low power mode or just spin */
        /* On AVR: sleep mode could be entered here */
        /* On ARM: WFI instruction could be used */
    }
}

/*===========================================================================*/
/* External Scheduler Functions                                               */
/*===========================================================================*/

extern void rtos_scheduler_add_ready(rtos_tcb_t *tcb);
extern void rtos_scheduler_process_delays(void);
extern void rtos_scheduler_switch_context(void);
extern void rtos_scheduler_check_preemption(void);

#if RTOS_USE_TIMESLICING
extern void rtos_scheduler_timeslice_tick(void);
#endif

/*===========================================================================*/
/* Kernel Initialization                                                      */
/*===========================================================================*/

/**
 * Initialize the RTOS kernel.
 *
 * Must be called before any other RTOS functions.
 */
void rtos_kernel_init(void)
{
    uint8_t i;

    /* Initialize ready lists */
    for (i = 0; i < RTOS_MAX_PRIORITIES; i++) {
        rtos_list_init(&g_ready_list[i]);
    }

    /* Initialize delayed list */
    rtos_list_init(&g_delayed_list);

    /* Reset kernel state */
    g_current_tcb = NULL;
    g_tick_count = 0;
    g_kernel_state = RTOS_KERNEL_NOT_STARTED;
    g_scheduler_suspended = 0;
    g_yield_pending = false;
    g_ready_priorities = 0;
    g_critical_nesting = 0;

    /* Create idle task at lowest priority */
    rtos_task_create(
        &g_idle_tcb,
        "idle",
        idle_task,
        NULL,
        RTOS_MAX_PRIORITIES - 1,
        g_idle_stack,
        RTOS_IDLE_STACK_SIZE
    );
}

/**
 * Start the RTOS scheduler.
 *
 * This function does not return. It starts the first task and
 * begins the scheduling loop.
 */
void rtos_kernel_start(void)
{
    /* Initialize the hardware timer and interrupts */
    rtos_port_init();

    /* Select first task to run */
    g_current_tcb = NULL;
    rtos_scheduler_switch_context();

    if (g_current_tcb == NULL) {
        /* No tasks created - shouldn't happen with idle task */
        while (1) { }
    }

    /* Mark kernel as running */
    g_kernel_state = RTOS_KERNEL_RUNNING;

    /* Start the first task (platform-specific, doesn't return) */
    rtos_port_start_scheduler();

    /* Should never reach here */
    while (1) { }
}

/*===========================================================================*/
/* Tick Handler                                                               */
/*===========================================================================*/

/**
 * System tick handler.
 *
 * Called from the hardware timer interrupt. Increments the tick
 * counter, processes delayed tasks, and handles time slicing.
 */
void rtos_tick_handler(void)
{
    /* Increment tick counter */
    g_tick_count++;

#if RTOS_USE_RUNTIME_STATS
    g_total_runtime++;
    if (g_current_tcb == &g_idle_tcb) {
        g_idle_runtime++;
    }
#endif

    /* Process delayed tasks */
    rtos_scheduler_process_delays();

#if RTOS_USE_TIMERS
    /* Process software timers */
    extern void rtos_timer_process(void);
    rtos_timer_process();
#endif

#if RTOS_USE_TIMESLICING
    /* Handle time slicing */
    rtos_scheduler_timeslice_tick();
#endif

#if RTOS_USE_TICK_HOOK
    /* User-defined tick hook */
    extern void rtos_tick_hook(void);
    rtos_tick_hook();
#endif

    /*
     * Trigger a context switch only when this handler is invoked from
     * a non-ISR context. When called from the tick ISR the port has
     * already saved context and will call rtos_scheduler_switch_context
     * after we return; calling rtos_port_yield() here would re-enter
     * SAVE_CONTEXT on top of the ISR's frame, overwriting the saved
     * stack_ptr and corrupting the interrupted task's resume state.
     */
    if (g_yield_pending && g_scheduler_suspended == 0 &&
        !rtos_port_is_in_isr()) {
        rtos_port_yield();
    }
}

/*===========================================================================*/
/* Kernel Queries                                                             */
/*===========================================================================*/

/**
 * Get the current system tick count.
 *
 * @return Current tick count
 */
uint32_t rtos_tick_get(void)
{
    uint32_t ticks;

    rtos_port_enter_critical();
    ticks = g_tick_count;
    rtos_port_exit_critical();

    return ticks;
}

/**
 * Get the current kernel state.
 *
 * @return Kernel state
 */
rtos_kernel_state_t rtos_kernel_get_state(void)
{
    return g_kernel_state;
}

/**
 * Check if we're currently in an ISR context.
 *
 * @return true if in ISR, false otherwise
 */
bool rtos_is_in_isr(void)
{
    return rtos_port_is_in_isr();
}

/**
 * Get the number of tasks currently ready to run.
 *
 * @return Number of ready tasks
 */
uint16_t rtos_get_ready_task_count(void)
{
    uint16_t count = 0;
    uint8_t i;

    rtos_port_enter_critical();
    for (i = 0; i < RTOS_MAX_PRIORITIES; i++) {
        count += rtos_list_count(&g_ready_list[i]);
    }
    /* Add current running task if any */
    if (g_current_tcb != NULL && g_current_tcb->state == RTOS_TASK_RUNNING) {
        count++;
    }
    rtos_port_exit_critical();

    return count;
}

/*===========================================================================*/
/* Runtime Statistics                                                         */
/*===========================================================================*/

#if RTOS_USE_RUNTIME_STATS

/**
 * Get CPU idle percentage (0-100).
 *
 * @return Idle percentage
 */
uint8_t rtos_get_idle_percent(void)
{
    uint32_t idle, total;
    uint8_t percent;

    rtos_port_enter_critical();
    idle = g_idle_runtime;
    total = g_total_runtime;
    rtos_port_exit_critical();

    if (total == 0) {
        return 100;
    }

    percent = (uint8_t)((idle * 100UL) / total);
    return percent;
}

/**
 * Reset runtime statistics.
 */
void rtos_reset_runtime_stats(void)
{
    rtos_port_enter_critical();
    g_idle_runtime = 0;
    g_total_runtime = 0;
    rtos_port_exit_critical();
}

#endif /* RTOS_USE_RUNTIME_STATS */

/*===========================================================================*/
/* Critical Section Support                                                   */
/*===========================================================================*/

/**
 * Enter a critical section (disable interrupts with nesting).
 */
void rtos_critical_enter(void)
{
    rtos_port_enter_critical();
    g_critical_nesting++;
}

/**
 * Exit a critical section.
 */
void rtos_critical_exit(void)
{
    if (g_critical_nesting > 0) {
        g_critical_nesting--;
        if (g_critical_nesting == 0) {
            rtos_port_exit_critical();
        }
    }
}

/*===========================================================================*/
/* Assertion Handler                                                          */
/*===========================================================================*/

#if RTOS_USE_ASSERT

/**
 * Assertion failure handler.
 * Called when RTOS_ASSERT fails.
 */
void rtos_assert_failed(const char *file, int line)
{
    (void)file;
    (void)line;

    /* Disable interrupts */
    rtos_port_disable_interrupts();

    /* Halt - in a real system, you might log this or blink an LED */
    while (1) {
        /* Trap */
    }
}

#endif

/*===========================================================================*/
/* Stack Overflow Hook                                                        */
/*===========================================================================*/

#if RTOS_CHECK_STACK_OVERFLOW

/**
 * Stack overflow handler.
 * Called when stack overflow is detected during context switch.
 */
void rtos_stack_overflow_hook(rtos_tcb_t *tcb)
{
    (void)tcb;

    /* Disable interrupts */
    rtos_port_disable_interrupts();

    /* In a real system, log the task name and halt or reset */
    while (1) {
        /* Trap */
    }
}

#endif
