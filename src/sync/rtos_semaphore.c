/**
 * MicroRTOS - Semaphore Implementation
 *
 * Binary and counting semaphores.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos_semaphore.h"
#include "rtos_list.h"
#include "rtos_port.h"
#include "rtos_task.h"

#if RTOS_USE_SEMAPHORES

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
/* Semaphore API Implementation                                               */
/*===========================================================================*/

void rtos_sem_init(rtos_sem_t *sem, uint32_t initial, uint32_t max)
{
    RTOS_ASSERT(sem != NULL);
    RTOS_ASSERT(initial <= max);
    RTOS_ASSERT(max > 0);

    sem->count = initial;
    sem->max_count = max;
    rtos_list_init(&sem->wait_list);
}

void rtos_sem_init_binary(rtos_sem_t *sem, uint32_t initial)
{
    RTOS_ASSERT(initial <= 1);
    rtos_sem_init(sem, initial, 1);
}

rtos_status_t rtos_sem_take(rtos_sem_t *sem, uint32_t timeout)
{
    rtos_tcb_t *current;
    rtos_status_t status = RTOS_OK;

    RTOS_ASSERT(sem != NULL);
    RTOS_ASSERT(!rtos_port_is_in_isr());

    rtos_port_enter_critical();

    current = g_current_tcb;

    /* Check if semaphore is available */
    if (sem->count > 0) {
        sem->count--;
        rtos_port_exit_critical();
        return RTOS_OK;
    }

    /* Semaphore not available - need to wait */
    if (timeout == RTOS_NO_WAIT) {
        rtos_port_exit_critical();
        return RTOS_ERR_TIMEOUT;
    }

    /* Block waiting for semaphore */
    rtos_scheduler_remove_ready(current);
    current->state = RTOS_TASK_BLOCKED;
    current->block_reason = RTOS_BLOCK_SEM;
    current->blocked_on = sem;

    /* Add to wait list sorted by priority */
    current->event_node.value = current->priority;
    rtos_list_insert_priority(&sem->wait_list, &current->event_node);

    /* Set up timeout if specified */
    if (timeout != RTOS_WAIT_FOREVER) {
        current->wake_tick = g_tick_count + timeout;
        current->state_node.value = current->wake_tick;
        rtos_list_insert_sorted(&g_delayed_list, &current->state_node);
    }

    rtos_port_exit_critical();

    /* Switch to another task */
    rtos_port_yield();

    /* We're back - check if we got the semaphore or timed out */
    rtos_port_enter_critical();

    /* If we're still in wait list, we timed out */
    if (rtos_list_node_is_linked(&current->event_node)) {
        rtos_list_remove(&sem->wait_list, &current->event_node);
        status = RTOS_ERR_TIMEOUT;
    } else {
        /* We were unblocked by sem_give */
        status = RTOS_OK;
    }

    current->blocked_on = NULL;
    current->block_reason = RTOS_BLOCK_NONE;

    rtos_port_exit_critical();

    return status;
}

rtos_status_t rtos_sem_give(rtos_sem_t *sem)
{
    rtos_tcb_t *waiter;
    rtos_list_node_t *node;
    bool need_switch = false;

    RTOS_ASSERT(sem != NULL);

    rtos_port_enter_critical();

    /* Check if any tasks are waiting */
    node = rtos_list_remove_head(&sem->wait_list);

    if (node != NULL) {
        /* Wake the highest priority waiter */
        waiter = RTOS_TCB_FROM_EVENT_NODE(node);

        /* Remove from delayed list if it was there */
        if (rtos_list_node_is_linked(&waiter->state_node)) {
            rtos_list_remove(&g_delayed_list, &waiter->state_node);
        }

        /* Unblock the waiter */
        waiter->state = RTOS_TASK_READY;
        waiter->block_reason = RTOS_BLOCK_NONE;
        waiter->blocked_on = NULL;
        rtos_scheduler_add_ready(waiter);

        /* Check if waiter has higher priority than current */
        if (g_current_tcb != NULL && waiter->priority < g_current_tcb->priority) {
            need_switch = true;
        }
    } else {
        /* No waiters - increment count if not at max */
        if (sem->count >= sem->max_count) {
            rtos_port_exit_critical();
            return RTOS_ERR_FULL;
        }
        sem->count++;
    }

    rtos_port_exit_critical();

    if (need_switch && g_kernel_state == RTOS_KERNEL_RUNNING) {
        rtos_port_yield();
    }

    return RTOS_OK;
}

rtos_status_t rtos_sem_give_from_isr(rtos_sem_t *sem, bool *yield_required)
{
    rtos_tcb_t *waiter;
    rtos_list_node_t *node;

    RTOS_ASSERT(sem != NULL);

    if (yield_required != NULL) {
        *yield_required = false;
    }

    /* Check if any tasks are waiting */
    node = rtos_list_remove_head(&sem->wait_list);

    if (node != NULL) {
        /* Wake the highest priority waiter */
        waiter = RTOS_TCB_FROM_EVENT_NODE(node);

        /* Remove from delayed list if it was there */
        if (rtos_list_node_is_linked(&waiter->state_node)) {
            rtos_list_remove(&g_delayed_list, &waiter->state_node);
        }

        /* Unblock the waiter */
        waiter->state = RTOS_TASK_READY;
        waiter->block_reason = RTOS_BLOCK_NONE;
        waiter->blocked_on = NULL;
        rtos_scheduler_add_ready(waiter);

        /* Check if waiter has higher priority than current */
        if (yield_required != NULL &&
            g_current_tcb != NULL &&
            waiter->priority < g_current_tcb->priority) {
            *yield_required = true;
        }
    } else {
        /* No waiters - increment count if not at max */
        if (sem->count >= sem->max_count) {
            return RTOS_ERR_FULL;
        }
        sem->count++;
    }

    return RTOS_OK;
}

uint32_t rtos_sem_get_count(rtos_sem_t *sem)
{
    if (sem == NULL) {
        return 0;
    }
    return sem->count;
}

#endif /* RTOS_USE_SEMAPHORES */
