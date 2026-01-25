/**
 * MicroRTOS - Task Management Implementation
 *
 * This file implements task creation, deletion, suspension,
 * resume, delay, and priority management.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos_task.h"
#include "rtos_list.h"
#include "rtos_port.h"
#include <string.h>

/*===========================================================================*/
/* External Declarations                                                      */
/*===========================================================================*/

/* Defined in rtos_kernel.c */
extern rtos_tcb_t *g_current_tcb;
extern rtos_list_t g_ready_list[RTOS_MAX_PRIORITIES];
extern rtos_list_t g_delayed_list;
extern volatile uint32_t g_tick_count;
extern volatile rtos_kernel_state_t g_kernel_state;
extern volatile uint8_t g_scheduler_suspended;
extern volatile bool g_yield_pending;

/* Defined in rtos_scheduler.c */
extern void rtos_scheduler_add_ready(rtos_tcb_t *tcb);
extern void rtos_scheduler_remove_ready(rtos_tcb_t *tcb);
extern void rtos_scheduler_switch_context(void);

/*===========================================================================*/
/* Stack Initialization Pattern                                               */
/*===========================================================================*/

#if RTOS_CHECK_STACK_OVERFLOW >= 1
#define STACK_FILL_PATTERN  0xA5
#endif

/*===========================================================================*/
/* Task Creation                                                              */
/*===========================================================================*/

rtos_status_t rtos_task_create(
    rtos_tcb_t *tcb,
    const char *name,
    rtos_task_func_t entry,
    void *arg,
    uint8_t priority,
    void *stack,
    uint16_t stack_size)
{
    rtos_status_t status = RTOS_OK;

    /* Parameter validation */
    if (tcb == NULL || entry == NULL || stack == NULL) {
        return RTOS_ERR_PARAM;
    }

    if (stack_size < RTOS_MINIMAL_STACK_SIZE) {
        return RTOS_ERR_PARAM;
    }

    if (priority >= RTOS_MAX_PRIORITIES) {
        return RTOS_ERR_PARAM;
    }

    /* Initialize TCB */
    memset(tcb, 0, sizeof(rtos_tcb_t));

    tcb->priority = priority;
    tcb->base_priority = priority;
    tcb->state = RTOS_TASK_READY;
    tcb->block_reason = RTOS_BLOCK_NONE;
    tcb->stack_base = stack;
    tcb->stack_size = stack_size;

#if RTOS_USE_TASK_NAMES
    tcb->name = (name != NULL) ? name : "?";
#else
    (void)name;
#endif

#if RTOS_USE_TIMESLICING
    tcb->timeslice_remaining = RTOS_TIMESLICE_TICKS;
#endif

#if RTOS_CHECK_STACK_OVERFLOW >= 1
    /* Fill stack with pattern for overflow detection */
    memset(stack, STACK_FILL_PATTERN, stack_size);
#endif

    /* Initialize list nodes */
    rtos_list_node_init(&tcb->state_node, tcb);
    rtos_list_node_init(&tcb->event_node, tcb);

    /* Initialize the stack with initial context */
    rtos_port_init_task_stack(tcb, entry, arg);

    /* Add to ready list */
    rtos_port_enter_critical();

    rtos_scheduler_add_ready(tcb);

    /* If scheduler is running and new task is higher priority, yield */
    if (g_kernel_state == RTOS_KERNEL_RUNNING &&
        g_current_tcb != NULL &&
        priority < g_current_tcb->priority) {
        g_yield_pending = true;
    }

    rtos_port_exit_critical();

    /* Trigger context switch if needed and scheduler is running */
    if (g_kernel_state == RTOS_KERNEL_RUNNING && g_yield_pending) {
        rtos_port_yield();
    }

    return status;
}

/*===========================================================================*/
/* Task Deletion                                                              */
/*===========================================================================*/

rtos_status_t rtos_task_delete(rtos_tcb_t *tcb)
{
    bool is_current_task;

    rtos_port_enter_critical();

    /* NULL means delete current task */
    if (tcb == NULL) {
        tcb = g_current_tcb;
    }

    if (tcb == NULL) {
        rtos_port_exit_critical();
        return RTOS_ERR_PARAM;
    }

    is_current_task = (tcb == g_current_tcb);

    /* Remove from any list it's currently in */
    switch (tcb->state) {
        case RTOS_TASK_READY:
        case RTOS_TASK_RUNNING:
            rtos_scheduler_remove_ready(tcb);
            break;

        case RTOS_TASK_BLOCKED:
            /* Remove from delayed list if applicable */
            if (tcb->block_reason == RTOS_BLOCK_DELAY) {
                rtos_list_remove(&g_delayed_list, &tcb->state_node);
            }
            /* Also remove from event wait list if waiting on something */
            if (rtos_list_node_is_linked(&tcb->event_node)) {
                /* The blocked_on pointer contains the object */
                /* We need to remove from its wait list */
                /* This is handled generically by clearing the link */
            }
            break;

        case RTOS_TASK_SUSPENDED:
        case RTOS_TASK_DELETED:
            /* Already removed from lists */
            break;
    }

    /* Clear any event node linkage */
    tcb->event_node.next = NULL;
    tcb->event_node.prev = NULL;

#if RTOS_USE_MUTEXES
    /* Release all held mutexes */
    /* TODO: Implement mutex release on task deletion */
#endif

    tcb->state = RTOS_TASK_DELETED;

    rtos_port_exit_critical();

    /* If we deleted ourselves, we need to switch to another task */
    if (is_current_task) {
        rtos_port_yield();
    }

    return RTOS_OK;
}

/*===========================================================================*/
/* Task Suspension and Resumption                                             */
/*===========================================================================*/

rtos_status_t rtos_task_suspend(rtos_tcb_t *tcb)
{
    bool is_current_task;

    rtos_port_enter_critical();

    if (tcb == NULL) {
        tcb = g_current_tcb;
    }

    if (tcb == NULL || tcb->state == RTOS_TASK_DELETED) {
        rtos_port_exit_critical();
        return RTOS_ERR_PARAM;
    }

    is_current_task = (tcb == g_current_tcb);

    switch (tcb->state) {
        case RTOS_TASK_READY:
        case RTOS_TASK_RUNNING:
            rtos_scheduler_remove_ready(tcb);
            tcb->state = RTOS_TASK_SUSPENDED;
            break;

        case RTOS_TASK_BLOCKED:
            /* Task stays blocked but also marked as suspended */
            /* Will go to SUSPENDED when unblocked instead of READY */
            tcb->state = RTOS_TASK_SUSPENDED;
            break;

        case RTOS_TASK_SUSPENDED:
            /* Already suspended */
            break;

        case RTOS_TASK_DELETED:
            rtos_port_exit_critical();
            return RTOS_ERR_STATE;
    }

    rtos_port_exit_critical();

    if (is_current_task) {
        rtos_port_yield();
    }

    return RTOS_OK;
}

rtos_status_t rtos_task_resume(rtos_tcb_t *tcb)
{
    bool need_switch = false;

    if (tcb == NULL) {
        return RTOS_ERR_PARAM;
    }

    rtos_port_enter_critical();

    if (tcb->state != RTOS_TASK_SUSPENDED) {
        rtos_port_exit_critical();
        return RTOS_ERR_STATE;
    }

    /* Move to ready state */
    tcb->state = RTOS_TASK_READY;
    rtos_scheduler_add_ready(tcb);

    /* Check if resumed task has higher priority */
    if (g_current_tcb != NULL && tcb->priority < g_current_tcb->priority) {
        need_switch = true;
    }

    rtos_port_exit_critical();

    if (need_switch && g_kernel_state == RTOS_KERNEL_RUNNING) {
        rtos_port_yield();
    }

    return RTOS_OK;
}

/*===========================================================================*/
/* Task Yielding                                                              */
/*===========================================================================*/

void rtos_task_yield(void)
{
    rtos_port_enter_critical();

    if (g_kernel_state == RTOS_KERNEL_RUNNING && g_scheduler_suspended == 0) {
        g_yield_pending = true;
    }

    rtos_port_exit_critical();

    if (g_yield_pending) {
        rtos_port_yield();
    }
}

/*===========================================================================*/
/* Task Delays                                                                */
/*===========================================================================*/

void rtos_task_delay(uint32_t ticks)
{
    rtos_tcb_t *tcb;

    if (ticks == 0) {
        rtos_task_yield();
        return;
    }

    rtos_port_enter_critical();

    tcb = g_current_tcb;
    if (tcb == NULL) {
        rtos_port_exit_critical();
        return;
    }

    /* Remove from ready list */
    rtos_scheduler_remove_ready(tcb);

    /* Set up delay */
    tcb->state = RTOS_TASK_BLOCKED;
    tcb->block_reason = RTOS_BLOCK_DELAY;
    tcb->wake_tick = g_tick_count + ticks;
    tcb->state_node.value = tcb->wake_tick;

    /* Add to delayed list sorted by wake time */
    rtos_list_insert_sorted(&g_delayed_list, &tcb->state_node);

    rtos_port_exit_critical();

    /* Switch to next task */
    rtos_port_yield();
}

void rtos_task_delay_until(uint32_t *prev_wake, uint32_t increment)
{
    rtos_tcb_t *tcb;
    uint32_t target_wake;
    int32_t time_to_wait;

    if (prev_wake == NULL || increment == 0) {
        return;
    }

    rtos_port_enter_critical();

    tcb = g_current_tcb;
    if (tcb == NULL) {
        rtos_port_exit_critical();
        return;
    }

    target_wake = *prev_wake + increment;
    time_to_wait = (int32_t)(target_wake - g_tick_count);

    /* Update previous wake time */
    *prev_wake = target_wake;

    /* If target time has already passed, don't delay */
    if (time_to_wait <= 0) {
        rtos_port_exit_critical();
        return;
    }

    /* Remove from ready list */
    rtos_scheduler_remove_ready(tcb);

    /* Set up delay */
    tcb->state = RTOS_TASK_BLOCKED;
    tcb->block_reason = RTOS_BLOCK_DELAY;
    tcb->wake_tick = target_wake;
    tcb->state_node.value = target_wake;

    /* Add to delayed list */
    rtos_list_insert_sorted(&g_delayed_list, &tcb->state_node);

    rtos_port_exit_critical();

    /* Switch to next task */
    rtos_port_yield();
}

/*===========================================================================*/
/* Task Queries                                                               */
/*===========================================================================*/

rtos_tcb_t *rtos_task_get_current(void)
{
    return g_current_tcb;
}

uint8_t rtos_task_get_priority(rtos_tcb_t *tcb)
{
    if (tcb == NULL) {
        tcb = g_current_tcb;
    }
    return (tcb != NULL) ? tcb->priority : 0;
}

rtos_status_t rtos_task_set_priority(rtos_tcb_t *tcb, uint8_t priority)
{
    bool need_switch = false;

    if (priority >= RTOS_MAX_PRIORITIES) {
        return RTOS_ERR_PARAM;
    }

    rtos_port_enter_critical();

    if (tcb == NULL) {
        tcb = g_current_tcb;
    }

    if (tcb == NULL || tcb->state == RTOS_TASK_DELETED) {
        rtos_port_exit_critical();
        return RTOS_ERR_PARAM;
    }

    if (tcb->priority != priority) {
        /* Remove from current ready list position */
        if (tcb->state == RTOS_TASK_READY || tcb->state == RTOS_TASK_RUNNING) {
            rtos_scheduler_remove_ready(tcb);
            tcb->priority = priority;
            tcb->base_priority = priority;
            rtos_scheduler_add_ready(tcb);

            /* Check if we need to switch */
            if (tcb == g_current_tcb) {
                /* Current task lowered its priority */
                need_switch = true;
            } else if (priority < g_current_tcb->priority) {
                /* Another task now has higher priority */
                need_switch = true;
            }
        } else {
            tcb->priority = priority;
            tcb->base_priority = priority;
        }
    }

    rtos_port_exit_critical();

    if (need_switch && g_kernel_state == RTOS_KERNEL_RUNNING) {
        rtos_port_yield();
    }

    return RTOS_OK;
}

rtos_task_state_t rtos_task_get_state(rtos_tcb_t *tcb)
{
    if (tcb == NULL) {
        return RTOS_TASK_DELETED;
    }
    return tcb->state;
}

const char *rtos_task_get_name(rtos_tcb_t *tcb)
{
#if RTOS_USE_TASK_NAMES
    if (tcb == NULL) {
        tcb = g_current_tcb;
    }
    return (tcb != NULL && tcb->name != NULL) ? tcb->name : "?";
#else
    (void)tcb;
    return "?";
#endif
}

uint16_t rtos_task_get_stack_high_water(rtos_tcb_t *tcb)
{
#if RTOS_CHECK_STACK_OVERFLOW >= 1
    uint8_t *stack_start;
    uint16_t unused = 0;

    if (tcb == NULL) {
        tcb = g_current_tcb;
    }

    if (tcb == NULL) {
        return 0;
    }

    stack_start = (uint8_t *)tcb->stack_base;

    /* Count bytes still at fill pattern */
    while (unused < tcb->stack_size &&
           stack_start[unused] == STACK_FILL_PATTERN) {
        unused++;
    }

    return unused;
#else
    (void)tcb;
    return 0;
#endif
}

/*===========================================================================*/
/* Task Notifications                                                         */
/*===========================================================================*/

#if RTOS_USE_TASK_NOTIFICATIONS

rtos_status_t rtos_task_notify_wait(
    bool clear_on_exit,
    uint32_t *value,
    uint32_t timeout)
{
    rtos_tcb_t *tcb;
    rtos_status_t status = RTOS_OK;

    rtos_port_enter_critical();

    tcb = g_current_tcb;
    if (tcb == NULL) {
        rtos_port_exit_critical();
        return RTOS_ERR_STATE;
    }

    /* Check if notification already pending */
    if (tcb->notify_state == RTOS_NOTIFY_RECEIVED) {
        if (value != NULL) {
            *value = tcb->notify_value;
        }
        if (clear_on_exit) {
            tcb->notify_value = 0;
        }
        tcb->notify_state = RTOS_NOTIFY_NOT_WAITING;
        rtos_port_exit_critical();
        return RTOS_OK;
    }

    /* No notification pending, need to wait */
    if (timeout == RTOS_NO_WAIT) {
        rtos_port_exit_critical();
        return RTOS_ERR_TIMEOUT;
    }

    /* Block waiting for notification */
    tcb->notify_state = RTOS_NOTIFY_WAITING;
    rtos_scheduler_remove_ready(tcb);
    tcb->state = RTOS_TASK_BLOCKED;
    tcb->block_reason = RTOS_BLOCK_NOTIFY;

    if (timeout != RTOS_WAIT_FOREVER) {
        tcb->wake_tick = g_tick_count + timeout;
        tcb->state_node.value = tcb->wake_tick;
        rtos_list_insert_sorted(&g_delayed_list, &tcb->state_node);
    }

    rtos_port_exit_critical();

    /* Switch to another task */
    rtos_port_yield();

    /* We're back - check what happened */
    rtos_port_enter_critical();

    if (tcb->notify_state == RTOS_NOTIFY_RECEIVED) {
        if (value != NULL) {
            *value = tcb->notify_value;
        }
        if (clear_on_exit) {
            tcb->notify_value = 0;
        }
        status = RTOS_OK;
    } else {
        status = RTOS_ERR_TIMEOUT;
    }

    tcb->notify_state = RTOS_NOTIFY_NOT_WAITING;

    rtos_port_exit_critical();

    return status;
}

rtos_status_t rtos_task_notify(
    rtos_tcb_t *tcb,
    uint32_t value,
    rtos_notify_action_t action)
{
    bool need_switch = false;

    if (tcb == NULL) {
        return RTOS_ERR_PARAM;
    }

    rtos_port_enter_critical();

    /* Apply notification value */
    switch (action) {
        case RTOS_NOTIFY_SET_VALUE:
            tcb->notify_value = value;
            break;
        case RTOS_NOTIFY_INCREMENT:
            tcb->notify_value++;
            break;
        case RTOS_NOTIFY_SET_BITS:
            tcb->notify_value |= value;
            break;
        case RTOS_NOTIFY_NO_ACTION:
            break;
    }

    /* Wake task if waiting for notification */
    if (tcb->notify_state == RTOS_NOTIFY_WAITING) {
        tcb->notify_state = RTOS_NOTIFY_RECEIVED;

        /* Remove from delayed list if there was a timeout */
        if (tcb->block_reason == RTOS_BLOCK_NOTIFY &&
            rtos_list_node_is_linked(&tcb->state_node)) {
            rtos_list_remove(&g_delayed_list, &tcb->state_node);
        }

        tcb->state = RTOS_TASK_READY;
        tcb->block_reason = RTOS_BLOCK_NONE;
        rtos_scheduler_add_ready(tcb);

        if (g_current_tcb != NULL && tcb->priority < g_current_tcb->priority) {
            need_switch = true;
        }
    } else {
        tcb->notify_state = RTOS_NOTIFY_RECEIVED;
    }

    rtos_port_exit_critical();

    if (need_switch && g_kernel_state == RTOS_KERNEL_RUNNING) {
        rtos_port_yield();
    }

    return RTOS_OK;
}

rtos_status_t rtos_task_notify_from_isr(
    rtos_tcb_t *tcb,
    uint32_t value,
    rtos_notify_action_t action,
    bool *yield_required)
{
    if (tcb == NULL) {
        return RTOS_ERR_PARAM;
    }

    if (yield_required != NULL) {
        *yield_required = false;
    }

    /* Apply notification value */
    switch (action) {
        case RTOS_NOTIFY_SET_VALUE:
            tcb->notify_value = value;
            break;
        case RTOS_NOTIFY_INCREMENT:
            tcb->notify_value++;
            break;
        case RTOS_NOTIFY_SET_BITS:
            tcb->notify_value |= value;
            break;
        case RTOS_NOTIFY_NO_ACTION:
            break;
    }

    /* Wake task if waiting */
    if (tcb->notify_state == RTOS_NOTIFY_WAITING) {
        tcb->notify_state = RTOS_NOTIFY_RECEIVED;

        if (tcb->block_reason == RTOS_BLOCK_NOTIFY &&
            rtos_list_node_is_linked(&tcb->state_node)) {
            rtos_list_remove(&g_delayed_list, &tcb->state_node);
        }

        tcb->state = RTOS_TASK_READY;
        tcb->block_reason = RTOS_BLOCK_NONE;
        rtos_scheduler_add_ready(tcb);

        if (yield_required != NULL &&
            g_current_tcb != NULL &&
            tcb->priority < g_current_tcb->priority) {
            *yield_required = true;
        }
    } else {
        tcb->notify_state = RTOS_NOTIFY_RECEIVED;
    }

    return RTOS_OK;
}

#endif /* RTOS_USE_TASK_NOTIFICATIONS */

/*===========================================================================*/
/* Runtime Statistics                                                         */
/*===========================================================================*/

#if RTOS_USE_RUNTIME_STATS

uint32_t rtos_task_get_runtime(rtos_tcb_t *tcb)
{
    uint32_t runtime;

    if (tcb == NULL) {
        tcb = g_current_tcb;
    }

    if (tcb == NULL) {
        return 0;
    }

    rtos_port_enter_critical();
    runtime = tcb->runtime_ticks;

    /* Add time since last switch-in for running task */
    if (tcb->state == RTOS_TASK_RUNNING) {
        runtime += g_tick_count - tcb->switch_in_tick;
    }
    rtos_port_exit_critical();

    return runtime;
}

#endif
