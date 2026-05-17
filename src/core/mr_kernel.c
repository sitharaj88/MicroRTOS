/**
 * MicroRTOS - Kernel Implementation
 *
 * This file implements kernel initialization, tick handling,
 * the idle task, and global kernel state management.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_types.h"
#include "mr_list.h"
#include "mr_task.h"
#include "mr_port.h"
#include <string.h>

/*===========================================================================*/
/* Global Kernel State                                                        */
/*===========================================================================*/

/* Current running task */
mr_tcb_t *g_current_tcb = NULL;

/* Ready lists - one per priority level */
mr_list_t g_ready_list[MR_MAX_PRIORITIES];

/* Delayed task list (sorted by wake time) */
mr_list_t g_delayed_list;

/* System tick counter */
volatile uint32_t g_tick_count = 0;

/* Kernel state */
volatile mr_kernel_state_t g_kernel_state = MR_KERNEL_NOT_STARTED;

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

static mr_tcb_t g_idle_tcb;
static uint8_t g_idle_stack[MR_IDLE_STACK_SIZE];

#if MR_USE_RUNTIME_STATS
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
#if MR_USE_IDLE_HOOK
        /* User-defined idle hook */
        extern void mr_idle_hook(void);
        mr_idle_hook();
#endif

        /* Low power mode or just spin */
        /* On AVR: sleep mode could be entered here */
        /* On ARM: WFI instruction could be used */
    }
}

/*===========================================================================*/
/* External Scheduler Functions                                               */
/*===========================================================================*/

extern void mr_scheduler_add_ready(mr_tcb_t *tcb);
extern void mr_scheduler_process_delays(void);
extern void mr_scheduler_switch_context(void);
extern void mr_scheduler_check_preemption(void);

#if MR_USE_TIMESLICING
extern void mr_scheduler_timeslice_tick(void);
#endif

/*===========================================================================*/
/* Kernel Initialization                                                      */
/*===========================================================================*/

/**
 * Initialize the RTOS kernel.
 *
 * Must be called before any other RTOS functions.
 */
void mr_kernel_init(void)
{
    uint8_t i;

    /* Initialize ready lists */
    for (i = 0; i < MR_MAX_PRIORITIES; i++) {
        mr_list_init(&g_ready_list[i]);
    }

    /* Initialize delayed list */
    mr_list_init(&g_delayed_list);

    /* Reset kernel state */
    g_current_tcb = NULL;
    g_tick_count = 0;
    g_kernel_state = MR_KERNEL_NOT_STARTED;
    g_scheduler_suspended = 0;
    g_yield_pending = false;
    g_ready_priorities = 0;
    g_critical_nesting = 0;

    /* Create idle task at lowest priority */
    mr_task_create(
        &g_idle_tcb,
        "idle",
        idle_task,
        NULL,
        MR_MAX_PRIORITIES - 1,
        g_idle_stack,
        MR_IDLE_STACK_SIZE
    );
}

/**
 * Start the RTOS scheduler.
 *
 * This function does not return. It starts the first task and
 * begins the scheduling loop.
 */
void mr_kernel_start(void)
{
    /* Initialize the hardware timer and interrupts */
    mr_port_init();

    /* Select first task to run */
    g_current_tcb = NULL;
    mr_scheduler_switch_context();

    if (g_current_tcb == NULL) {
        /* No tasks created - shouldn't happen with idle task */
        while (1) { }
    }

    /* Mark kernel as running */
    g_kernel_state = MR_KERNEL_RUNNING;

    /* Start the first task (platform-specific, doesn't return) */
    mr_port_start_scheduler();

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
void mr_tick_handler(void)
{
    /* Increment tick counter */
    g_tick_count++;

#if MR_USE_RUNTIME_STATS
    g_total_runtime++;
    if (g_current_tcb == &g_idle_tcb) {
        g_idle_runtime++;
    }
#endif

    /* Process delayed tasks */
    mr_scheduler_process_delays();

#if MR_USE_TIMERS
    /* Process software timers */
    extern void mr_timer_process(void);
    mr_timer_process();
#endif

#if MR_USE_TIMESLICING
    /* Handle time slicing */
    mr_scheduler_timeslice_tick();
#endif

#if MR_USE_TICK_HOOK
    /* User-defined tick hook */
    extern void mr_tick_hook(void);
    mr_tick_hook();
#endif

    /*
     * Trigger a context switch only when this handler is invoked from
     * a non-ISR context. When called from the tick ISR the port has
     * already saved context and will call mr_scheduler_switch_context
     * after we return; calling mr_port_yield() here would re-enter
     * SAVE_CONTEXT on top of the ISR's frame, overwriting the saved
     * stack_ptr and corrupting the interrupted task's resume state.
     */
    if (g_yield_pending && g_scheduler_suspended == 0 &&
        !mr_port_is_in_isr()) {
        mr_port_yield();
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
uint32_t mr_tick_get(void)
{
    uint32_t ticks;

    mr_port_enter_critical();
    ticks = g_tick_count;
    mr_port_exit_critical();

    return ticks;
}

/**
 * Get the current kernel state.
 *
 * @return Kernel state
 */
mr_kernel_state_t mr_kernel_get_state(void)
{
    return g_kernel_state;
}

/**
 * Check if we're currently in an ISR context.
 *
 * @return true if in ISR, false otherwise
 */
bool mr_is_in_isr(void)
{
    return mr_port_is_in_isr();
}

/**
 * Get the number of tasks currently ready to run.
 *
 * @return Number of ready tasks
 */
uint16_t mr_get_ready_task_count(void)
{
    uint16_t count = 0;
    uint8_t i;

    mr_port_enter_critical();
    for (i = 0; i < MR_MAX_PRIORITIES; i++) {
        count += mr_list_count(&g_ready_list[i]);
    }
    /* Add current running task if any */
    if (g_current_tcb != NULL && g_current_tcb->state == MR_TASK_RUNNING) {
        count++;
    }
    mr_port_exit_critical();

    return count;
}

/*===========================================================================*/
/* Runtime Statistics                                                         */
/*===========================================================================*/

#if MR_USE_RUNTIME_STATS

/**
 * Get CPU idle percentage (0-100).
 *
 * @return Idle percentage
 */
uint8_t mr_get_idle_percent(void)
{
    uint32_t idle, total;
    uint8_t percent;

    mr_port_enter_critical();
    idle = g_idle_runtime;
    total = g_total_runtime;
    mr_port_exit_critical();

    if (total == 0) {
        return 100;
    }

    percent = (uint8_t)((idle * 100UL) / total);
    return percent;
}

/**
 * Reset runtime statistics.
 */
void mr_reset_runtime_stats(void)
{
    mr_port_enter_critical();
    g_idle_runtime = 0;
    g_total_runtime = 0;
    mr_port_exit_critical();
}

#endif /* MR_USE_RUNTIME_STATS */

/*===========================================================================*/
/* Critical Section Support                                                   */
/*===========================================================================*/

/**
 * Enter a critical section (disable interrupts with nesting).
 */
void mr_critical_enter(void)
{
    mr_port_enter_critical();
    g_critical_nesting++;
}

/**
 * Exit a critical section.
 */
void mr_critical_exit(void)
{
    if (g_critical_nesting > 0) {
        g_critical_nesting--;
        if (g_critical_nesting == 0) {
            mr_port_exit_critical();
        }
    }
}

/*===========================================================================*/
/* Assertion Handler                                                          */
/*===========================================================================*/

#if MR_USE_ASSERT

/**
 * Assertion failure handler.
 * Called when MR_ASSERT fails.
 */
void mr_assert_failed(const char *file, int line)
{
    (void)file;
    (void)line;

    /* Disable interrupts */
    mr_port_disable_interrupts();

    /* Halt - in a real system, you might log this or blink an LED */
    while (1) {
        /* Trap */
    }
}

#endif

/*===========================================================================*/
/* Stack Overflow Hook                                                        */
/*===========================================================================*/

#if MR_CHECK_STACK_OVERFLOW

/**
 * Stack overflow handler.
 * Called when stack overflow is detected during context switch.
 */
void mr_stack_overflow_hook(mr_tcb_t *tcb)
{
    (void)tcb;

    /* Disable interrupts */
    mr_port_disable_interrupts();

    /* In a real system, log the task name and halt or reset */
    while (1) {
        /* Trap */
    }
}

#endif
