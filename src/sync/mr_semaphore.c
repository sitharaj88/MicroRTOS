/**
 * MicroRTOS - Semaphore Implementation
 *
 * Binary and counting semaphores.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_semaphore.h"
#include "mr_list.h"
#include "mr_port.h"
#include "mr_task.h"

#if MR_USE_SEMAPHORES

/*===========================================================================*/
/* External Declarations                                                      */
/*===========================================================================*/

extern mr_tcb_t *g_current_tcb;
extern mr_list_t g_delayed_list;
extern volatile uint32_t g_tick_count;
extern volatile mr_kernel_state_t g_kernel_state;

extern void mr_scheduler_add_ready(mr_tcb_t *tcb);
extern void mr_scheduler_remove_ready(mr_tcb_t *tcb);

/*===========================================================================*/
/* Semaphore API Implementation                                               */
/*===========================================================================*/

void mr_sem_init(mr_sem_t *sem, uint32_t initial, uint32_t max)
{
    MR_ASSERT(sem != NULL);
    MR_ASSERT(initial <= max);
    MR_ASSERT(max > 0);

    sem->count = initial;
    sem->max_count = max;
    mr_list_init(&sem->wait_list);
}

void mr_sem_init_binary(mr_sem_t *sem, uint32_t initial)
{
    MR_ASSERT(initial <= 1);
    mr_sem_init(sem, initial, 1);
}

mr_status_t mr_sem_take(mr_sem_t *sem, uint32_t timeout)
{
    mr_tcb_t *current;
    mr_status_t status = MR_OK;

    MR_ASSERT(sem != NULL);
    MR_ASSERT(!mr_port_is_in_isr());

    mr_port_enter_critical();

    current = g_current_tcb;

    /* Check if semaphore is available */
    if (sem->count > 0) {
        sem->count--;
        mr_port_exit_critical();
        return MR_OK;
    }

    /* Semaphore not available - need to wait */
    if (timeout == MR_NO_WAIT) {
        mr_port_exit_critical();
        return MR_ERR_TIMEOUT;
    }

    /* Block waiting for semaphore */
    mr_scheduler_remove_ready(current);
    current->state = MR_TASK_BLOCKED;
    current->block_reason = MR_BLOCK_SEM;
    current->blocked_on = sem;

    /* Add to wait list sorted by priority */
    current->event_node.value = current->priority;
    mr_list_insert_priority(&sem->wait_list, &current->event_node);

    /* Set up timeout if specified */
    if (timeout != MR_WAIT_FOREVER) {
        current->wake_tick = g_tick_count + timeout;
        current->state_node.value = current->wake_tick;
        mr_list_insert_sorted(&g_delayed_list, &current->state_node);
    }

    mr_port_exit_critical();

    /* Switch to another task */
    mr_port_yield();

    /* We're back - check if we got the semaphore or timed out */
    mr_port_enter_critical();

    /* If we're still in wait list, we timed out */
    if (mr_list_node_is_linked(&current->event_node)) {
        mr_list_remove(&sem->wait_list, &current->event_node);
        status = MR_ERR_TIMEOUT;
    } else {
        /* We were unblocked by sem_give */
        status = MR_OK;
    }

    current->blocked_on = NULL;
    current->block_reason = MR_BLOCK_NONE;

    mr_port_exit_critical();

    return status;
}

mr_status_t mr_sem_give(mr_sem_t *sem)
{
    mr_tcb_t *waiter;
    mr_list_node_t *node;
    bool need_switch = false;

    MR_ASSERT(sem != NULL);

    mr_port_enter_critical();

    /* Check if any tasks are waiting */
    node = mr_list_remove_head(&sem->wait_list);

    if (node != NULL) {
        /* Wake the highest priority waiter */
        waiter = MR_TCB_FROM_EVENT_NODE(node);

        /* Remove from delayed list if it was there */
        if (mr_list_node_is_linked(&waiter->state_node)) {
            mr_list_remove(&g_delayed_list, &waiter->state_node);
        }

        /* Unblock the waiter */
        waiter->state = MR_TASK_READY;
        waiter->block_reason = MR_BLOCK_NONE;
        waiter->blocked_on = NULL;
        mr_scheduler_add_ready(waiter);

        /* Check if waiter has higher priority than current */
        if (g_current_tcb != NULL && waiter->priority < g_current_tcb->priority) {
            need_switch = true;
        }
    } else {
        /* No waiters - increment count if not at max */
        if (sem->count >= sem->max_count) {
            mr_port_exit_critical();
            return MR_ERR_FULL;
        }
        sem->count++;
    }

    mr_port_exit_critical();

    if (need_switch && g_kernel_state == MR_KERNEL_RUNNING) {
        mr_port_yield();
    }

    return MR_OK;
}

mr_status_t mr_sem_give_from_isr(mr_sem_t *sem, bool *yield_required)
{
    mr_tcb_t *waiter;
    mr_list_node_t *node;

    MR_ASSERT(sem != NULL);

    if (yield_required != NULL) {
        *yield_required = false;
    }

    /* Check if any tasks are waiting */
    node = mr_list_remove_head(&sem->wait_list);

    if (node != NULL) {
        /* Wake the highest priority waiter */
        waiter = MR_TCB_FROM_EVENT_NODE(node);

        /* Remove from delayed list if it was there */
        if (mr_list_node_is_linked(&waiter->state_node)) {
            mr_list_remove(&g_delayed_list, &waiter->state_node);
        }

        /* Unblock the waiter */
        waiter->state = MR_TASK_READY;
        waiter->block_reason = MR_BLOCK_NONE;
        waiter->blocked_on = NULL;
        mr_scheduler_add_ready(waiter);

        /* Check if waiter has higher priority than current */
        if (yield_required != NULL &&
            g_current_tcb != NULL &&
            waiter->priority < g_current_tcb->priority) {
            *yield_required = true;
        }
    } else {
        /* No waiters - increment count if not at max */
        if (sem->count >= sem->max_count) {
            return MR_ERR_FULL;
        }
        sem->count++;
    }

    return MR_OK;
}

uint32_t mr_sem_get_count(mr_sem_t *sem)
{
    if (sem == NULL) {
        return 0;
    }
    return sem->count;
}

#endif /* MR_USE_SEMAPHORES */
