/**
 * MicroRTOS - Scheduler Implementation
 *
 * This file implements the priority-based preemptive scheduler
 * with time-slicing support for equal priority tasks.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_types.h"
#include "mr_list.h"
#include "mr_port.h"

/*===========================================================================*/
/* Global Variables (defined in mr_kernel.c)                                */
/*===========================================================================*/

extern mr_tcb_t *g_current_tcb;
extern mr_list_t g_ready_list[MR_MAX_PRIORITIES];
extern mr_list_t g_delayed_list;
extern volatile uint32_t g_tick_count;
extern volatile mr_kernel_state_t g_kernel_state;
extern volatile uint8_t g_scheduler_suspended;
extern volatile bool g_yield_pending;
extern volatile uint32_t g_ready_priorities;

/*===========================================================================*/
/* Local Variables                                                            */
/*===========================================================================*/

/* Bitmap of priorities with ready tasks (for fast lookup) */
/* Bit 0 = priority 0 (highest), etc. */

/*===========================================================================*/
/* Priority Bitmap Operations                                                 */
/*===========================================================================*/

/**
 * Find the highest priority (lowest number) with a ready task.
 * Uses bit manipulation for O(1) lookup.
 */
static inline uint8_t find_highest_priority(void)
{
    uint32_t bits = g_ready_priorities;

    if (bits == 0) {
        return MR_MAX_PRIORITIES; /* No ready tasks */
    }

#if defined(__GNUC__) && (defined(__arm__) || defined(__ARM_ARCH))
    /* ARM has CLZ instruction */
    return (uint8_t)__builtin_ctz(bits);
#elif defined(__GNUC__) && defined(__AVR__)
    /* AVR: simple loop (no native CLZ) */
    uint8_t priority = 0;
    while ((bits & 1) == 0 && priority < MR_MAX_PRIORITIES) {
        bits >>= 1;
        priority++;
    }
    return priority;
#else
    /* Portable fallback */
    uint8_t priority = 0;
    while ((bits & 1) == 0 && priority < MR_MAX_PRIORITIES) {
        bits >>= 1;
        priority++;
    }
    return priority;
#endif
}

/**
 * Set a priority bit (task is ready at this priority).
 */
static inline void set_priority_bit(uint8_t priority)
{
    g_ready_priorities |= (1UL << priority);
}

/**
 * Clear a priority bit if no more tasks at this priority.
 */
static inline void clear_priority_bit(uint8_t priority)
{
    if (mr_list_is_empty(&g_ready_list[priority])) {
        g_ready_priorities &= ~(1UL << priority);
    }
}

/*===========================================================================*/
/* Scheduler Functions                                                        */
/*===========================================================================*/

/**
 * Add a task to the ready list.
 * Must be called with interrupts disabled.
 */
void mr_scheduler_add_ready(mr_tcb_t *tcb)
{
    uint8_t priority;

    MR_ASSERT(tcb != NULL);
    MR_ASSERT(tcb->priority < MR_MAX_PRIORITIES);

    priority = tcb->priority;

    /* Set node value for priority-based insertion (not really needed for
       round-robin within priority, but keeps it consistent) */
    tcb->state_node.value = priority;

    /* Add to end of priority list (FIFO within same priority) */
    mr_list_insert_end(&g_ready_list[priority], &tcb->state_node);

    /* Update priority bitmap */
    set_priority_bit(priority);

    /* Mark as ready (not running - that's set by context switch) */
    if (tcb->state != MR_TASK_RUNNING) {
        tcb->state = MR_TASK_READY;
    }

#if MR_USE_TIMESLICING
    /* Reset time slice for round-robin */
    tcb->timeslice_remaining = MR_TIMESLICE_TICKS;
#endif
}

/**
 * Remove a task from the ready list.
 * Must be called with interrupts disabled.
 */
void mr_scheduler_remove_ready(mr_tcb_t *tcb)
{
    uint8_t priority;

    MR_ASSERT(tcb != NULL);

    priority = tcb->priority;

    /* Remove from ready list */
    mr_list_remove(&g_ready_list[priority], &tcb->state_node);

    /* Update priority bitmap if list is now empty */
    clear_priority_bit(priority);
}

/**
 * Select the next task to run.
 * Must be called with interrupts disabled.
 *
 * @return TCB of next task to run
 */
mr_tcb_t *mr_scheduler_select_next(void)
{
    uint8_t priority;
    mr_list_node_t *node;

    priority = find_highest_priority();

    if (priority >= MR_MAX_PRIORITIES) {
        /* No ready tasks - this shouldn't happen if idle task exists */
        return NULL;
    }

    node = mr_list_peek_head(&g_ready_list[priority]);
    if (node == NULL) {
        return NULL;
    }

    return MR_TCB_FROM_STATE_NODE(node);
}

/**
 * Perform a context switch to the next ready task.
 * Must be called with interrupts disabled.
 */
void mr_scheduler_switch_context(void)
{
    mr_tcb_t *next_tcb;

    /* Don't switch if scheduler is suspended */
    if (g_scheduler_suspended > 0) {
        g_yield_pending = true;
        return;
    }

    /* Find next task to run */
    next_tcb = mr_scheduler_select_next();

    if (next_tcb == NULL) {
        /* No ready tasks - stay with current (shouldn't happen) */
        return;
    }

    if (next_tcb == g_current_tcb) {
        /* Already running highest priority task */
        g_yield_pending = false;
        return;
    }

    /* Update statistics for outgoing task */
#if MR_USE_RUNTIME_STATS
    if (g_current_tcb != NULL && g_current_tcb->state == MR_TASK_RUNNING) {
        g_current_tcb->runtime_ticks += g_tick_count - g_current_tcb->switch_in_tick;
    }
#endif

    /*
     * Mark outgoing task as ready if it was running, AND put it back on
     * its ready list. Without the add_ready, a preempted task gets lost
     * from the scheduler entirely: state says READY but the node sits in
     * no list. Symptom on AVR: the system blinks once then deadlocks
     * because the next yield finds no runnable task.
     *
     * If the current task is blocking (state already set to BLOCKED /
     * SUSPENDED by task_delay, mutex_wait, etc.) the caller will have
     * placed it on the appropriate wait list already, so we skip this.
     */
    if (g_current_tcb != NULL && g_current_tcb->state == MR_TASK_RUNNING) {
        g_current_tcb->state = MR_TASK_READY;
        mr_scheduler_add_ready(g_current_tcb);
    }

    /*
     * Take the incoming task off the ready list and update the priority
     * bitmap if the list is now empty. Calling mr_list_remove directly
     * (without clearing the bit) leaves a stale entry in g_ready_priorities,
     * so the next select_next picks the empty priority and returns NULL,
     * leaving the system unable to context-switch.
     */
    mr_scheduler_remove_ready(next_tcb);
    next_tcb->state = MR_TASK_RUNNING;

    /* Update statistics for incoming task */
#if MR_USE_RUNTIME_STATS
    next_tcb->switch_in_tick = g_tick_count;
#endif

#if MR_USE_TIMESLICING
    next_tcb->timeslice_remaining = MR_TIMESLICE_TICKS;
#endif

    g_yield_pending = false;

    /* Actually switch context (platform-specific) */
    g_current_tcb = next_tcb;
}

/**
 * Handle time slicing for round-robin scheduling.
 * Called from tick handler.
 */
#if MR_USE_TIMESLICING
void mr_scheduler_timeslice_tick(void)
{
    mr_tcb_t *tcb = g_current_tcb;

    if (tcb == NULL || g_scheduler_suspended > 0) {
        return;
    }

    /* Decrement time slice */
    if (tcb->timeslice_remaining > 0) {
        tcb->timeslice_remaining--;
    }

    /* Time slice expired? */
    if (tcb->timeslice_remaining == 0) {
        /* Check if another task of same priority is ready */
        if (mr_list_count(&g_ready_list[tcb->priority]) > 0) {
            /* Move current task to end of its priority list */
            mr_scheduler_add_ready(tcb);
            g_yield_pending = true;
        } else {
            /* No other task at this priority, reset slice */
            tcb->timeslice_remaining = MR_TIMESLICE_TICKS;
        }
    }
}
#endif

/**
 * Check for higher priority ready tasks.
 * Called after unblocking a task to see if we should switch.
 */
void mr_scheduler_check_preemption(void)
{
    uint8_t highest = find_highest_priority();

    if (highest < MR_MAX_PRIORITIES &&
        g_current_tcb != NULL &&
        highest < g_current_tcb->priority) {
        g_yield_pending = true;
    }
}

/**
 * Process delayed tasks and wake any that are due.
 * Called from tick handler.
 */
void mr_scheduler_process_delays(void)
{
    mr_list_node_t *node;
    mr_tcb_t *tcb;

    while (!mr_list_is_empty(&g_delayed_list)) {
        node = mr_list_peek_head(&g_delayed_list);
        tcb = MR_TCB_FROM_STATE_NODE(node);

        /* Check if this task's wake time has arrived */
        /* Handle tick wraparound with signed comparison */
        if ((int32_t)(tcb->wake_tick - g_tick_count) > 0) {
            /* Not yet time to wake - list is sorted, so we're done */
            break;
        }

        /* Remove from delayed list */
        mr_list_remove_head(&g_delayed_list);

        /* Clear blocking state */
        tcb->block_reason = MR_BLOCK_NONE;

        /* Handle based on previous block reason */
        if (tcb->state == MR_TASK_SUSPENDED) {
            /* Task was suspended while blocked, stays suspended */
            continue;
        }

        /* Move to ready list */
        tcb->state = MR_TASK_READY;
        mr_scheduler_add_ready(tcb);

        /* Check if we should preempt */
        if (g_current_tcb != NULL && tcb->priority < g_current_tcb->priority) {
            g_yield_pending = true;
        }
    }
}

/*===========================================================================*/
/* Scheduler Suspend/Resume                                                   */
/*===========================================================================*/

/**
 * Suspend the scheduler (prevent context switches).
 * Can be nested.
 */
void mr_scheduler_suspend(void)
{
    mr_port_enter_critical();
    g_scheduler_suspended++;
    mr_port_exit_critical();
}

/**
 * Resume the scheduler.
 * Will trigger a context switch if one was pending.
 */
void mr_scheduler_resume(void)
{
    mr_port_enter_critical();

    if (g_scheduler_suspended > 0) {
        g_scheduler_suspended--;

        if (g_scheduler_suspended == 0 && g_yield_pending) {
            mr_port_exit_critical();
            mr_port_yield();
            return;
        }
    }

    mr_port_exit_critical();
}
