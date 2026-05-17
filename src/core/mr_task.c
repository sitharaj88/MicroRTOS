/**
 * MicroRTOS - Task Management Implementation
 *
 * This file implements task creation, deletion, suspension,
 * resume, delay, and priority management.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_task.h"
#include "mr_list.h"
#include "mr_port.h"
#include <string.h>

/*===========================================================================*/
/* External Declarations                                                      */
/*===========================================================================*/

/* Defined in mr_kernel.c */
extern mr_tcb_t *g_current_tcb;
extern mr_list_t g_ready_list[MR_MAX_PRIORITIES];
extern mr_list_t g_delayed_list;
extern volatile uint32_t g_tick_count;
extern volatile mr_kernel_state_t g_kernel_state;
extern volatile uint8_t g_scheduler_suspended;
extern volatile bool g_yield_pending;

/* Defined in mr_scheduler.c */
extern void mr_scheduler_add_ready(mr_tcb_t *tcb);
extern void mr_scheduler_remove_ready(mr_tcb_t *tcb);
extern void mr_scheduler_switch_context(void);

/*===========================================================================*/
/* Stack Initialization Pattern                                               */
/*===========================================================================*/

#if MR_CHECK_STACK_OVERFLOW >= 1
#define STACK_FILL_PATTERN  0xA5
#endif

/*===========================================================================*/
/* Task Creation                                                              */
/*===========================================================================*/

mr_status_t mr_task_create(
    mr_tcb_t *tcb,
    const char *name,
    mr_task_func_t entry,
    void *arg,
    uint8_t priority,
    void *stack,
    uint16_t stack_size)
{
    mr_status_t status = MR_OK;

    /* Parameter validation */
    if (tcb == NULL || entry == NULL || stack == NULL) {
        return MR_ERR_PARAM;
    }

    if (stack_size < MR_MINIMAL_STACK_SIZE) {
        return MR_ERR_PARAM;
    }

    if (priority >= MR_MAX_PRIORITIES) {
        return MR_ERR_PARAM;
    }

    /* Initialize TCB */
    memset(tcb, 0, sizeof(mr_tcb_t));

    tcb->priority = priority;
    tcb->base_priority = priority;
    tcb->state = MR_TASK_READY;
    tcb->block_reason = MR_BLOCK_NONE;
    tcb->stack_base = stack;
    tcb->stack_size = stack_size;

#if MR_USE_TASK_NAMES
    tcb->name = (name != NULL) ? name : "?";
#else
    (void)name;
#endif

#if MR_USE_TIMESLICING
    tcb->timeslice_remaining = MR_TIMESLICE_TICKS;
#endif

#if MR_CHECK_STACK_OVERFLOW >= 1
    /* Fill stack with pattern for overflow detection */
    memset(stack, STACK_FILL_PATTERN, stack_size);
#endif

    /* Initialize list nodes */
    mr_list_node_init(&tcb->state_node, tcb);
    mr_list_node_init(&tcb->event_node, tcb);

    /* Initialize the stack with initial context */
    mr_port_init_task_stack(tcb, entry, arg);

    /* Add to ready list */
    mr_port_enter_critical();

    mr_scheduler_add_ready(tcb);

    /* If scheduler is running and new task is higher priority, yield */
    if (g_kernel_state == MR_KERNEL_RUNNING &&
        g_current_tcb != NULL &&
        priority < g_current_tcb->priority) {
        g_yield_pending = true;
    }

    mr_port_exit_critical();

    /* Trigger context switch if needed and scheduler is running */
    if (g_kernel_state == MR_KERNEL_RUNNING && g_yield_pending) {
        mr_port_yield();
    }

    return status;
}

/*===========================================================================*/
/* Task Deletion                                                              */
/*===========================================================================*/

mr_status_t mr_task_delete(mr_tcb_t *tcb)
{
    bool is_current_task;

    mr_port_enter_critical();

    /* NULL means delete current task */
    if (tcb == NULL) {
        tcb = g_current_tcb;
    }

    if (tcb == NULL) {
        mr_port_exit_critical();
        return MR_ERR_PARAM;
    }

    is_current_task = (tcb == g_current_tcb);

    /* Remove from any list it's currently in */
    switch (tcb->state) {
        case MR_TASK_READY:
        case MR_TASK_RUNNING:
            mr_scheduler_remove_ready(tcb);
            break;

        case MR_TASK_BLOCKED:
            /* Remove from delayed list if applicable */
            if (tcb->block_reason == MR_BLOCK_DELAY) {
                mr_list_remove(&g_delayed_list, &tcb->state_node);
            }
            /* Also remove from event wait list if waiting on something */
            if (mr_list_node_is_linked(&tcb->event_node)) {
                /* The blocked_on pointer contains the object */
                /* We need to remove from its wait list */
                /* This is handled generically by clearing the link */
            }
            break;

        case MR_TASK_SUSPENDED:
        case MR_TASK_DELETED:
            /* Already removed from lists */
            break;
    }

    /* Clear any event node linkage */
    tcb->event_node.next = NULL;
    tcb->event_node.prev = NULL;

#if MR_USE_MUTEXES
    /* Release all held mutexes */
    /* TODO: Implement mutex release on task deletion */
#endif

    tcb->state = MR_TASK_DELETED;

    mr_port_exit_critical();

    /* If we deleted ourselves, we need to switch to another task */
    if (is_current_task) {
        mr_port_yield();
    }

    return MR_OK;
}

/*===========================================================================*/
/* Task Suspension and Resumption                                             */
/*===========================================================================*/

mr_status_t mr_task_suspend(mr_tcb_t *tcb)
{
    bool is_current_task;

    mr_port_enter_critical();

    if (tcb == NULL) {
        tcb = g_current_tcb;
    }

    if (tcb == NULL || tcb->state == MR_TASK_DELETED) {
        mr_port_exit_critical();
        return MR_ERR_PARAM;
    }

    is_current_task = (tcb == g_current_tcb);

    switch (tcb->state) {
        case MR_TASK_READY:
        case MR_TASK_RUNNING:
            mr_scheduler_remove_ready(tcb);
            tcb->state = MR_TASK_SUSPENDED;
            break;

        case MR_TASK_BLOCKED:
            /* Task stays blocked but also marked as suspended */
            /* Will go to SUSPENDED when unblocked instead of READY */
            tcb->state = MR_TASK_SUSPENDED;
            break;

        case MR_TASK_SUSPENDED:
            /* Already suspended */
            break;

        case MR_TASK_DELETED:
            mr_port_exit_critical();
            return MR_ERR_STATE;
    }

    mr_port_exit_critical();

    if (is_current_task) {
        mr_port_yield();
    }

    return MR_OK;
}

mr_status_t mr_task_resume(mr_tcb_t *tcb)
{
    bool need_switch = false;

    if (tcb == NULL) {
        return MR_ERR_PARAM;
    }

    mr_port_enter_critical();

    if (tcb->state != MR_TASK_SUSPENDED) {
        mr_port_exit_critical();
        return MR_ERR_STATE;
    }

    /* Move to ready state */
    tcb->state = MR_TASK_READY;
    mr_scheduler_add_ready(tcb);

    /* Check if resumed task has higher priority */
    if (g_current_tcb != NULL && tcb->priority < g_current_tcb->priority) {
        need_switch = true;
    }

    mr_port_exit_critical();

    if (need_switch && g_kernel_state == MR_KERNEL_RUNNING) {
        mr_port_yield();
    }

    return MR_OK;
}

/*===========================================================================*/
/* Task Yielding                                                              */
/*===========================================================================*/

void mr_task_yield(void)
{
    mr_port_enter_critical();

    if (g_kernel_state == MR_KERNEL_RUNNING && g_scheduler_suspended == 0) {
        g_yield_pending = true;
    }

    mr_port_exit_critical();

    if (g_yield_pending) {
        mr_port_yield();
    }
}

/*===========================================================================*/
/* Task Delays                                                                */
/*===========================================================================*/

void mr_task_delay(uint32_t ticks)
{
    mr_tcb_t *tcb;

    if (ticks == 0) {
        mr_task_yield();
        return;
    }

    mr_port_enter_critical();

    tcb = g_current_tcb;
    if (tcb == NULL) {
        mr_port_exit_critical();
        return;
    }

    /* Remove from ready list */
    mr_scheduler_remove_ready(tcb);

    /* Set up delay */
    tcb->state = MR_TASK_BLOCKED;
    tcb->block_reason = MR_BLOCK_DELAY;
    tcb->wake_tick = g_tick_count + ticks;
    tcb->state_node.value = tcb->wake_tick;

    /* Add to delayed list sorted by wake time */
    mr_list_insert_sorted(&g_delayed_list, &tcb->state_node);

    mr_port_exit_critical();

    /* Switch to next task */
    mr_port_yield();
}

void mr_task_delay_until(uint32_t *prev_wake, uint32_t increment)
{
    mr_tcb_t *tcb;
    uint32_t target_wake;
    int32_t time_to_wait;

    if (prev_wake == NULL || increment == 0) {
        return;
    }

    mr_port_enter_critical();

    tcb = g_current_tcb;
    if (tcb == NULL) {
        mr_port_exit_critical();
        return;
    }

    target_wake = *prev_wake + increment;
    time_to_wait = (int32_t)(target_wake - g_tick_count);

    /* Update previous wake time */
    *prev_wake = target_wake;

    /* If target time has already passed, don't delay */
    if (time_to_wait <= 0) {
        mr_port_exit_critical();
        return;
    }

    /* Remove from ready list */
    mr_scheduler_remove_ready(tcb);

    /* Set up delay */
    tcb->state = MR_TASK_BLOCKED;
    tcb->block_reason = MR_BLOCK_DELAY;
    tcb->wake_tick = target_wake;
    tcb->state_node.value = target_wake;

    /* Add to delayed list */
    mr_list_insert_sorted(&g_delayed_list, &tcb->state_node);

    mr_port_exit_critical();

    /* Switch to next task */
    mr_port_yield();
}

/*===========================================================================*/
/* Task Queries                                                               */
/*===========================================================================*/

mr_tcb_t *mr_task_get_current(void)
{
    return g_current_tcb;
}

uint8_t mr_task_get_priority(mr_tcb_t *tcb)
{
    if (tcb == NULL) {
        tcb = g_current_tcb;
    }
    return (tcb != NULL) ? tcb->priority : 0;
}

mr_status_t mr_task_set_priority(mr_tcb_t *tcb, uint8_t priority)
{
    bool need_switch = false;

    if (priority >= MR_MAX_PRIORITIES) {
        return MR_ERR_PARAM;
    }

    mr_port_enter_critical();

    if (tcb == NULL) {
        tcb = g_current_tcb;
    }

    if (tcb == NULL || tcb->state == MR_TASK_DELETED) {
        mr_port_exit_critical();
        return MR_ERR_PARAM;
    }

    if (tcb->priority != priority) {
        /* Remove from current ready list position */
        if (tcb->state == MR_TASK_READY || tcb->state == MR_TASK_RUNNING) {
            mr_scheduler_remove_ready(tcb);
            tcb->priority = priority;
            tcb->base_priority = priority;
            mr_scheduler_add_ready(tcb);

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

    mr_port_exit_critical();

    if (need_switch && g_kernel_state == MR_KERNEL_RUNNING) {
        mr_port_yield();
    }

    return MR_OK;
}

mr_task_state_t mr_task_get_state(mr_tcb_t *tcb)
{
    if (tcb == NULL) {
        return MR_TASK_DELETED;
    }
    return tcb->state;
}

const char *mr_task_get_name(mr_tcb_t *tcb)
{
#if MR_USE_TASK_NAMES
    if (tcb == NULL) {
        tcb = g_current_tcb;
    }
    return (tcb != NULL && tcb->name != NULL) ? tcb->name : "?";
#else
    (void)tcb;
    return "?";
#endif
}

uint16_t mr_task_get_stack_high_water(mr_tcb_t *tcb)
{
#if MR_CHECK_STACK_OVERFLOW >= 1
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

#if MR_USE_TASK_NOTIFICATIONS

mr_status_t mr_task_notify_wait(
    bool clear_on_exit,
    uint32_t *value,
    uint32_t timeout)
{
    mr_tcb_t *tcb;
    mr_status_t status = MR_OK;

    mr_port_enter_critical();

    tcb = g_current_tcb;
    if (tcb == NULL) {
        mr_port_exit_critical();
        return MR_ERR_STATE;
    }

    /* Check if notification already pending */
    if (tcb->notify_state == MR_NOTIFY_RECEIVED) {
        if (value != NULL) {
            *value = tcb->notify_value;
        }
        if (clear_on_exit) {
            tcb->notify_value = 0;
        }
        tcb->notify_state = MR_NOTIFY_NOT_WAITING;
        mr_port_exit_critical();
        return MR_OK;
    }

    /* No notification pending, need to wait */
    if (timeout == MR_NO_WAIT) {
        mr_port_exit_critical();
        return MR_ERR_TIMEOUT;
    }

    /* Block waiting for notification */
    tcb->notify_state = MR_NOTIFY_WAITING;
    mr_scheduler_remove_ready(tcb);
    tcb->state = MR_TASK_BLOCKED;
    tcb->block_reason = MR_BLOCK_NOTIFY;

    if (timeout != MR_WAIT_FOREVER) {
        tcb->wake_tick = g_tick_count + timeout;
        tcb->state_node.value = tcb->wake_tick;
        mr_list_insert_sorted(&g_delayed_list, &tcb->state_node);
    }

    mr_port_exit_critical();

    /* Switch to another task */
    mr_port_yield();

    /* We're back - check what happened */
    mr_port_enter_critical();

    if (tcb->notify_state == MR_NOTIFY_RECEIVED) {
        if (value != NULL) {
            *value = tcb->notify_value;
        }
        if (clear_on_exit) {
            tcb->notify_value = 0;
        }
        status = MR_OK;
    } else {
        status = MR_ERR_TIMEOUT;
    }

    tcb->notify_state = MR_NOTIFY_NOT_WAITING;

    mr_port_exit_critical();

    return status;
}

mr_status_t mr_task_notify(
    mr_tcb_t *tcb,
    uint32_t value,
    mr_notify_action_t action)
{
    bool need_switch = false;

    if (tcb == NULL) {
        return MR_ERR_PARAM;
    }

    mr_port_enter_critical();

    /* Apply notification value */
    switch (action) {
        case MR_NOTIFY_SET_VALUE:
            tcb->notify_value = value;
            break;
        case MR_NOTIFY_INCREMENT:
            tcb->notify_value++;
            break;
        case MR_NOTIFY_SET_BITS:
            tcb->notify_value |= value;
            break;
        case MR_NOTIFY_NO_ACTION:
            break;
    }

    /* Wake task if waiting for notification */
    if (tcb->notify_state == MR_NOTIFY_WAITING) {
        tcb->notify_state = MR_NOTIFY_RECEIVED;

        /* Remove from delayed list if there was a timeout */
        if (tcb->block_reason == MR_BLOCK_NOTIFY &&
            mr_list_node_is_linked(&tcb->state_node)) {
            mr_list_remove(&g_delayed_list, &tcb->state_node);
        }

        tcb->state = MR_TASK_READY;
        tcb->block_reason = MR_BLOCK_NONE;
        mr_scheduler_add_ready(tcb);

        if (g_current_tcb != NULL && tcb->priority < g_current_tcb->priority) {
            need_switch = true;
        }
    } else {
        tcb->notify_state = MR_NOTIFY_RECEIVED;
    }

    mr_port_exit_critical();

    if (need_switch && g_kernel_state == MR_KERNEL_RUNNING) {
        mr_port_yield();
    }

    return MR_OK;
}

mr_status_t mr_task_notify_from_isr(
    mr_tcb_t *tcb,
    uint32_t value,
    mr_notify_action_t action,
    bool *yield_required)
{
    if (tcb == NULL) {
        return MR_ERR_PARAM;
    }

    if (yield_required != NULL) {
        *yield_required = false;
    }

    /* Apply notification value */
    switch (action) {
        case MR_NOTIFY_SET_VALUE:
            tcb->notify_value = value;
            break;
        case MR_NOTIFY_INCREMENT:
            tcb->notify_value++;
            break;
        case MR_NOTIFY_SET_BITS:
            tcb->notify_value |= value;
            break;
        case MR_NOTIFY_NO_ACTION:
            break;
    }

    /* Wake task if waiting */
    if (tcb->notify_state == MR_NOTIFY_WAITING) {
        tcb->notify_state = MR_NOTIFY_RECEIVED;

        if (tcb->block_reason == MR_BLOCK_NOTIFY &&
            mr_list_node_is_linked(&tcb->state_node)) {
            mr_list_remove(&g_delayed_list, &tcb->state_node);
        }

        tcb->state = MR_TASK_READY;
        tcb->block_reason = MR_BLOCK_NONE;
        mr_scheduler_add_ready(tcb);

        if (yield_required != NULL &&
            g_current_tcb != NULL &&
            tcb->priority < g_current_tcb->priority) {
            *yield_required = true;
        }
    } else {
        tcb->notify_state = MR_NOTIFY_RECEIVED;
    }

    return MR_OK;
}

#endif /* MR_USE_TASK_NOTIFICATIONS */

/*===========================================================================*/
/* Runtime Statistics                                                         */
/*===========================================================================*/

#if MR_USE_RUNTIME_STATS

uint32_t mr_task_get_runtime(mr_tcb_t *tcb)
{
    uint32_t runtime;

    if (tcb == NULL) {
        tcb = g_current_tcb;
    }

    if (tcb == NULL) {
        return 0;
    }

    mr_port_enter_critical();
    runtime = tcb->runtime_ticks;

    /* Add time since last switch-in for running task */
    if (tcb->state == MR_TASK_RUNNING) {
        runtime += g_tick_count - tcb->switch_in_tick;
    }
    mr_port_exit_critical();

    return runtime;
}

#endif
