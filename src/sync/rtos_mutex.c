/**
 * MicroRTOS - Mutex Implementation
 *
 * Mutual exclusion with priority inheritance support.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos_mutex.h"
#include "rtos_list.h"
#include "rtos_port.h"
#include "rtos_task.h"

#if RTOS_USE_MUTEXES

/*===========================================================================*/
/* External Declarations                                                      */
/*===========================================================================*/

extern rtos_tcb_t *g_current_tcb;
extern rtos_list_t g_ready_list[RTOS_MAX_PRIORITIES];
extern rtos_list_t g_delayed_list;
extern volatile uint32_t g_tick_count;
extern volatile rtos_kernel_state_t g_kernel_state;

extern void rtos_scheduler_add_ready(rtos_tcb_t *tcb);
extern void rtos_scheduler_remove_ready(rtos_tcb_t *tcb);

/*===========================================================================*/
/* Priority Inheritance Helpers                                               */
/*===========================================================================*/

/**
 * Boost the priority of a task holding a mutex.
 */
static void boost_owner_priority(rtos_tcb_t *owner, uint8_t new_priority)
{
    if (owner == NULL || new_priority >= owner->priority) {
        return;
    }

    /* Remove from current ready list position */
    if (owner->state == RTOS_TASK_READY || owner->state == RTOS_TASK_RUNNING) {
        rtos_scheduler_remove_ready(owner);
        owner->priority = new_priority;
        rtos_scheduler_add_ready(owner);
    } else {
        /* Just update priority, will take effect when unblocked */
        owner->priority = new_priority;
    }
}

/**
 * Restore a task's priority after releasing a mutex.
 */
static void restore_owner_priority(rtos_tcb_t *owner)
{
    uint8_t highest_needed = owner->base_priority;
    rtos_mutex_t *mutex;
    rtos_list_node_t *node;
    rtos_tcb_t *waiter;

    if (owner == NULL) {
        return;
    }

    /*
     * Check all mutexes still held by this task.
     * The new priority should be the highest of:
     * - Base priority
     * - Highest priority of any waiter on held mutexes
     */
    mutex = owner->mutex_held;
    while (mutex != NULL) {
        node = rtos_list_peek_head(&mutex->wait_list);
        if (node != NULL) {
            waiter = RTOS_TCB_FROM_EVENT_NODE(node);
            if (waiter->priority < highest_needed) {
                highest_needed = waiter->priority;
            }
        }
        mutex = mutex->next;
    }

    /* Apply new priority if different */
    if (owner->priority != highest_needed) {
        if (owner->state == RTOS_TASK_READY || owner->state == RTOS_TASK_RUNNING) {
            rtos_scheduler_remove_ready(owner);
            owner->priority = highest_needed;
            rtos_scheduler_add_ready(owner);
        } else {
            owner->priority = highest_needed;
        }
    }
}

/*===========================================================================*/
/* Mutex API Implementation                                                   */
/*===========================================================================*/

void rtos_mutex_init(rtos_mutex_t *mutex)
{
    RTOS_ASSERT(mutex != NULL);

    mutex->owner = NULL;
    mutex->lock_count = 0;
    mutex->next = NULL;
    rtos_list_init(&mutex->wait_list);
}

rtos_status_t rtos_mutex_lock(rtos_mutex_t *mutex, uint32_t timeout)
{
    rtos_tcb_t *current;
    rtos_status_t status = RTOS_OK;

    RTOS_ASSERT(mutex != NULL);
    RTOS_ASSERT(!rtos_port_is_in_isr());

    rtos_port_enter_critical();

    current = g_current_tcb;

    /* Check if mutex is free */
    if (mutex->owner == NULL) {
        /* Acquire the mutex */
        mutex->owner = current;
        mutex->lock_count = 1;

        /* Add to list of mutexes held by this task */
        mutex->next = current->mutex_held;
        current->mutex_held = mutex;
        current->mutexes_held_count++;

        rtos_port_exit_critical();
        return RTOS_OK;
    }

    /* Check for recursive lock by same owner */
    if (mutex->owner == current) {
        mutex->lock_count++;
        rtos_port_exit_critical();
        return RTOS_OK;
    }

    /* Mutex is held by another task - need to wait */
    if (timeout == RTOS_NO_WAIT) {
        rtos_port_exit_critical();
        return RTOS_ERR_TIMEOUT;
    }

    /* Priority inheritance: boost owner if we have higher priority */
    if (current->priority < mutex->owner->priority) {
        boost_owner_priority(mutex->owner, current->priority);
    }

    /* Block waiting for mutex */
    rtos_scheduler_remove_ready(current);
    current->state = RTOS_TASK_BLOCKED;
    current->block_reason = RTOS_BLOCK_MUTEX;
    current->blocked_on = mutex;

    /* Add to wait list sorted by priority */
    current->event_node.value = current->priority;
    rtos_list_insert_priority(&mutex->wait_list, &current->event_node);

    /* Set up timeout if specified */
    if (timeout != RTOS_WAIT_FOREVER) {
        current->wake_tick = g_tick_count + timeout;
        current->state_node.value = current->wake_tick;
        rtos_list_insert_sorted(&g_delayed_list, &current->state_node);
    }

    rtos_port_exit_critical();

    /* Switch to another task */
    rtos_port_yield();

    /* We're back - check if we got the mutex or timed out */
    rtos_port_enter_critical();

    if (mutex->owner == current) {
        status = RTOS_OK;
    } else {
        /* Timed out - remove from wait list if still there */
        if (rtos_list_node_is_linked(&current->event_node)) {
            rtos_list_remove(&mutex->wait_list, &current->event_node);
        }
        status = RTOS_ERR_TIMEOUT;
    }

    current->blocked_on = NULL;
    current->block_reason = RTOS_BLOCK_NONE;

    rtos_port_exit_critical();

    return status;
}

rtos_status_t rtos_mutex_trylock(rtos_mutex_t *mutex)
{
    return rtos_mutex_lock(mutex, RTOS_NO_WAIT);
}

rtos_status_t rtos_mutex_unlock(rtos_mutex_t *mutex)
{
    rtos_tcb_t *current;
    rtos_tcb_t *next_owner;
    rtos_list_node_t *node;
    rtos_mutex_t **mutex_ptr;
    bool need_switch = false;

    RTOS_ASSERT(mutex != NULL);
    RTOS_ASSERT(!rtos_port_is_in_isr());

    rtos_port_enter_critical();

    current = g_current_tcb;

    /* Verify we own this mutex */
    if (mutex->owner != current) {
        rtos_port_exit_critical();
        return RTOS_ERR_NOT_OWNER;
    }

    /* Handle recursive unlock */
    if (mutex->lock_count > 1) {
        mutex->lock_count--;
        rtos_port_exit_critical();
        return RTOS_OK;
    }

    /* Remove mutex from owner's held list */
    mutex_ptr = &current->mutex_held;
    while (*mutex_ptr != NULL && *mutex_ptr != mutex) {
        mutex_ptr = &(*mutex_ptr)->next;
    }
    if (*mutex_ptr == mutex) {
        *mutex_ptr = mutex->next;
        current->mutexes_held_count--;
    }

    /* Check if any tasks are waiting */
    node = rtos_list_remove_head(&mutex->wait_list);

    if (node != NULL) {
        /* Give mutex to highest priority waiter */
        next_owner = RTOS_TCB_FROM_EVENT_NODE(node);

        /* Remove from delayed list if it was there */
        if (rtos_list_node_is_linked(&next_owner->state_node)) {
            rtos_list_remove(&g_delayed_list, &next_owner->state_node);
        }

        /* Transfer ownership */
        mutex->owner = next_owner;
        mutex->lock_count = 1;

        /* Add to new owner's held list */
        mutex->next = next_owner->mutex_held;
        next_owner->mutex_held = mutex;
        next_owner->mutexes_held_count++;

        /* Unblock the new owner */
        next_owner->state = RTOS_TASK_READY;
        next_owner->block_reason = RTOS_BLOCK_NONE;
        next_owner->blocked_on = NULL;
        rtos_scheduler_add_ready(next_owner);

        /* Check if new owner has higher priority */
        if (next_owner->priority < current->priority) {
            need_switch = true;
        }
    } else {
        /* No waiters - mutex is now free */
        mutex->owner = NULL;
        mutex->lock_count = 0;
    }

    /* Restore our priority if it was boosted */
    restore_owner_priority(current);

    rtos_port_exit_critical();

    if (need_switch && g_kernel_state == RTOS_KERNEL_RUNNING) {
        rtos_port_yield();
    }

    return RTOS_OK;
}

rtos_tcb_t *rtos_mutex_get_owner(rtos_mutex_t *mutex)
{
    if (mutex == NULL) {
        return NULL;
    }
    return mutex->owner;
}

bool rtos_mutex_is_locked(rtos_mutex_t *mutex)
{
    if (mutex == NULL) {
        return false;
    }
    return (mutex->owner != NULL);
}

#endif /* RTOS_USE_MUTEXES */
