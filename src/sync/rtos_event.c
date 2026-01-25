/**
 * MicroRTOS - Event Groups Implementation
 *
 * Event flags for multi-task synchronization.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos_event.h"
#include "rtos_list.h"
#include "rtos_port.h"
#include "rtos_task.h"

#if RTOS_USE_EVENT_GROUPS

/*===========================================================================*/
/* External Declarations                                                      */
/*===========================================================================*/

extern rtos_tcb_t *g_current_tcb;
extern rtos_list_t g_delayed_list;
extern volatile uint32_t g_tick_count;
extern volatile rtos_kernel_state_t g_kernel_state;

extern void rtos_scheduler_add_ready(rtos_tcb_t *tcb);
extern void rtos_scheduler_remove_ready(rtos_tcb_t *tcb);

/*===========================================================================*/
/* Wait Info Storage                                                          */
/*===========================================================================*/

/*
 * We need to store wait parameters somewhere accessible during wake-up.
 * We use the event_node's container pointer for the wait info structure.
 * This is a bit of a hack but avoids adding more fields to TCB.
 */

typedef struct {
    uint32_t wait_bits;
    bool wait_all;
    bool clear_on_exit;
    uint32_t result_bits;
} event_wait_params_t;

/* Static storage for wait params (one per potential waiter) */
/* In a real system, this might be embedded in the TCB */

/*===========================================================================*/
/* Helper Functions                                                           */
/*===========================================================================*/

/**
 * Check if event condition is satisfied.
 */
static bool check_event_condition(
    uint32_t current_bits,
    uint32_t wait_bits,
    bool wait_all)
{
    if (wait_all) {
        return ((current_bits & wait_bits) == wait_bits);
    } else {
        return ((current_bits & wait_bits) != 0);
    }
}

/**
 * Wake waiting tasks whose conditions are satisfied.
 */
static bool wake_waiting_tasks(rtos_event_t *event)
{
    rtos_list_node_t *node, *next;
    rtos_tcb_t *tcb;
    bool task_woken = false;
    uint32_t wait_bits;
    bool wait_all;
    bool clear_on_exit;

    node = event->wait_list.head;
    while (node != NULL) {
        next = node->next;
        tcb = RTOS_TCB_FROM_EVENT_NODE(node);

        /* Get wait parameters stored in blocked_on field */
        /* We pack: bits in lower 24 bits, flags in upper 8 bits */
        wait_bits = (uint32_t)(uintptr_t)tcb->blocked_on & 0x00FFFFFFUL;
        wait_all = ((uint32_t)(uintptr_t)tcb->blocked_on >> 24) & 0x01;
        clear_on_exit = ((uint32_t)(uintptr_t)tcb->blocked_on >> 25) & 0x01;

        /* Check condition */
        if (check_event_condition(event->bits, wait_bits, wait_all)) {
            /* Remove from wait list */
            rtos_list_remove(&event->wait_list, &tcb->event_node);

            /* Remove from delayed list if there */
            if (rtos_list_node_is_linked(&tcb->state_node)) {
                rtos_list_remove(&g_delayed_list, &tcb->state_node);
            }

            /* Clear bits if requested */
            if (clear_on_exit) {
                event->bits &= ~(event->bits & wait_bits);
            }

            /* Store result in delay_ticks field temporarily */
            tcb->delay_ticks = event->bits;

            /* Unblock */
            tcb->state = RTOS_TASK_READY;
            tcb->block_reason = RTOS_BLOCK_NONE;
            rtos_scheduler_add_ready(tcb);

            task_woken = true;
        }

        node = next;
    }

    return task_woken;
}

/*===========================================================================*/
/* Event Groups API Implementation                                            */
/*===========================================================================*/

void rtos_event_init(rtos_event_t *event)
{
    RTOS_ASSERT(event != NULL);

    event->bits = 0;
    rtos_list_init(&event->wait_list);
}

uint32_t rtos_event_wait(
    rtos_event_t *event,
    uint32_t bits,
    bool wait_all,
    bool clear_on_exit,
    uint32_t timeout)
{
    rtos_tcb_t *current;
    uint32_t result = 0;
    uintptr_t packed_params;

    RTOS_ASSERT(event != NULL);
    RTOS_ASSERT(bits != 0);
    RTOS_ASSERT(!rtos_port_is_in_isr());

    rtos_port_enter_critical();

    current = g_current_tcb;

    /* Check if condition already satisfied */
    if (check_event_condition(event->bits, bits, wait_all)) {
        result = event->bits;

        if (clear_on_exit) {
            event->bits &= ~(event->bits & bits);
        }

        rtos_port_exit_critical();
        return result;
    }

    /* Need to wait */
    if (timeout == RTOS_NO_WAIT) {
        rtos_port_exit_critical();
        return 0;
    }

    /* Block waiting */
    rtos_scheduler_remove_ready(current);
    current->state = RTOS_TASK_BLOCKED;
    current->block_reason = RTOS_BLOCK_EVENT;

    /* Pack wait parameters into blocked_on pointer */
    /* Bits (24 bits) | wait_all (1 bit) | clear_on_exit (1 bit) */
    packed_params = (bits & 0x00FFFFFFUL);
    if (wait_all) packed_params |= (1UL << 24);
    if (clear_on_exit) packed_params |= (1UL << 25);
    current->blocked_on = (void *)packed_params;

    /* Add to wait list */
    current->event_node.value = current->priority;
    rtos_list_insert_priority(&event->wait_list, &current->event_node);

    /* Set timeout */
    if (timeout != RTOS_WAIT_FOREVER) {
        current->wake_tick = g_tick_count + timeout;
        current->state_node.value = current->wake_tick;
        rtos_list_insert_sorted(&g_delayed_list, &current->state_node);
    }

    rtos_port_exit_critical();

    /* Switch to another task */
    rtos_port_yield();

    /* We're back */
    rtos_port_enter_critical();

    /* Check if we're still in wait list (timeout) */
    if (rtos_list_node_is_linked(&current->event_node)) {
        rtos_list_remove(&event->wait_list, &current->event_node);
        result = 0;  /* Timeout */
    } else {
        /* Condition was satisfied - result stored in delay_ticks */
        result = current->delay_ticks;
    }

    current->blocked_on = NULL;
    current->block_reason = RTOS_BLOCK_NONE;

    rtos_port_exit_critical();

    return result;
}

uint32_t rtos_event_set(rtos_event_t *event, uint32_t bits)
{
    bool need_switch = false;

    RTOS_ASSERT(event != NULL);

    rtos_port_enter_critical();

    /* Set the bits */
    event->bits |= bits;

    /* Wake any tasks whose conditions are now satisfied */
    if (wake_waiting_tasks(event)) {
        need_switch = true;
    }

    rtos_port_exit_critical();

    if (need_switch && g_kernel_state == RTOS_KERNEL_RUNNING) {
        rtos_port_yield();
    }

    return event->bits;
}

uint32_t rtos_event_clear(rtos_event_t *event, uint32_t bits)
{
    uint32_t old_bits;

    RTOS_ASSERT(event != NULL);

    rtos_port_enter_critical();
    old_bits = event->bits;
    event->bits &= ~bits;
    rtos_port_exit_critical();

    return old_bits;
}

uint32_t rtos_event_get(rtos_event_t *event)
{
    if (event == NULL) {
        return 0;
    }
    return event->bits;
}

uint32_t rtos_event_set_from_isr(
    rtos_event_t *event,
    uint32_t bits,
    bool *yield_required)
{
    rtos_list_node_t *node, *next;
    rtos_tcb_t *tcb;
    uint32_t wait_bits;
    bool wait_all;
    bool clear_on_exit;

    RTOS_ASSERT(event != NULL);

    if (yield_required != NULL) {
        *yield_required = false;
    }

    /* Set the bits */
    event->bits |= bits;

    /* Check waiting tasks */
    node = event->wait_list.head;
    while (node != NULL) {
        next = node->next;
        tcb = RTOS_TCB_FROM_EVENT_NODE(node);

        wait_bits = (uint32_t)(uintptr_t)tcb->blocked_on & 0x00FFFFFFUL;
        wait_all = ((uint32_t)(uintptr_t)tcb->blocked_on >> 24) & 0x01;
        clear_on_exit = ((uint32_t)(uintptr_t)tcb->blocked_on >> 25) & 0x01;

        if (check_event_condition(event->bits, wait_bits, wait_all)) {
            rtos_list_remove(&event->wait_list, &tcb->event_node);

            if (rtos_list_node_is_linked(&tcb->state_node)) {
                rtos_list_remove(&g_delayed_list, &tcb->state_node);
            }

            if (clear_on_exit) {
                event->bits &= ~(event->bits & wait_bits);
            }

            tcb->delay_ticks = event->bits;
            tcb->state = RTOS_TASK_READY;
            tcb->block_reason = RTOS_BLOCK_NONE;
            rtos_scheduler_add_ready(tcb);

            if (yield_required != NULL &&
                g_current_tcb != NULL &&
                tcb->priority < g_current_tcb->priority) {
                *yield_required = true;
            }
        }

        node = next;
    }

    return event->bits;
}

uint32_t rtos_event_sync(
    rtos_event_t *event,
    uint32_t set_bits,
    uint32_t wait_bits,
    uint32_t timeout)
{
    uint32_t result;

    RTOS_ASSERT(event != NULL);

    /* Set our bits */
    rtos_event_set(event, set_bits);

    /* Wait for all required bits */
    result = rtos_event_wait(event, wait_bits, true, true, timeout);

    return result;
}

#endif /* RTOS_USE_EVENT_GROUPS */
