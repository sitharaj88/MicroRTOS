/**
 * MicroRTOS - Mutex Implementation
 *
 * Mutual exclusion with priority inheritance support.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_mutex.h"
#include "mr_list.h"
#include "mr_port.h"
#include "mr_task.h"

#if MR_USE_MUTEXES

/*===========================================================================*/
/* External Declarations                                                      */
/*===========================================================================*/

extern mr_tcb_t *g_current_tcb;
extern mr_list_t g_ready_list[MR_MAX_PRIORITIES];
extern mr_list_t g_delayed_list;
extern volatile uint32_t g_tick_count;
extern volatile mr_kernel_state_t g_kernel_state;

extern void mr_scheduler_add_ready(mr_tcb_t *tcb);
extern void mr_scheduler_remove_ready(mr_tcb_t *tcb);

/*===========================================================================*/
/* Priority Inheritance Helpers                                               */
/*===========================================================================*/

/**
 * Boost the priority of a task holding a mutex.
 */
static void boost_owner_priority(mr_tcb_t *owner, uint8_t new_priority)
{
    if (owner == NULL || new_priority >= owner->priority) {
        return;
    }

    /* Remove from current ready list position */
    if (owner->state == MR_TASK_READY || owner->state == MR_TASK_RUNNING) {
        mr_scheduler_remove_ready(owner);
        owner->priority = new_priority;
        mr_scheduler_add_ready(owner);
    } else {
        /* Just update priority, will take effect when unblocked */
        owner->priority = new_priority;
    }
}

/**
 * Restore a task's priority after releasing a mutex.
 */
static void restore_owner_priority(mr_tcb_t *owner)
{
    uint8_t highest_needed = owner->base_priority;
    mr_mutex_t *mutex;
    mr_list_node_t *node;
    mr_tcb_t *waiter;

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
        node = mr_list_peek_head(&mutex->wait_list);
        if (node != NULL) {
            waiter = MR_TCB_FROM_EVENT_NODE(node);
            if (waiter->priority < highest_needed) {
                highest_needed = waiter->priority;
            }
        }
        mutex = mutex->next;
    }

    /* Apply new priority if different */
    if (owner->priority != highest_needed) {
        if (owner->state == MR_TASK_READY || owner->state == MR_TASK_RUNNING) {
            mr_scheduler_remove_ready(owner);
            owner->priority = highest_needed;
            mr_scheduler_add_ready(owner);
        } else {
            owner->priority = highest_needed;
        }
    }
}

/*===========================================================================*/
/* Mutex API Implementation                                                   */
/*===========================================================================*/

void mr_mutex_init(mr_mutex_t *mutex)
{
    MR_ASSERT(mutex != NULL);

    mutex->owner = NULL;
    mutex->lock_count = 0;
    mutex->next = NULL;
    mr_list_init(&mutex->wait_list);
}

mr_status_t mr_mutex_lock(mr_mutex_t *mutex, uint32_t timeout)
{
    mr_tcb_t *current;
    mr_status_t status = MR_OK;

    MR_ASSERT(mutex != NULL);
    MR_ASSERT(!mr_port_is_in_isr());

    mr_port_enter_critical();

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

        mr_port_exit_critical();
        return MR_OK;
    }

    /* Check for recursive lock by same owner */
    if (mutex->owner == current) {
        mutex->lock_count++;
        mr_port_exit_critical();
        return MR_OK;
    }

    /* Mutex is held by another task - need to wait */
    if (timeout == MR_NO_WAIT) {
        mr_port_exit_critical();
        return MR_ERR_TIMEOUT;
    }

    /* Priority inheritance: boost owner if we have higher priority */
    if (current->priority < mutex->owner->priority) {
        boost_owner_priority(mutex->owner, current->priority);
    }

    /* Block waiting for mutex */
    mr_scheduler_remove_ready(current);
    current->state = MR_TASK_BLOCKED;
    current->block_reason = MR_BLOCK_MUTEX;
    current->blocked_on = mutex;

    /* Add to wait list sorted by priority */
    current->event_node.value = current->priority;
    mr_list_insert_priority(&mutex->wait_list, &current->event_node);

    /* Set up timeout if specified */
    if (timeout != MR_WAIT_FOREVER) {
        current->wake_tick = g_tick_count + timeout;
        current->state_node.value = current->wake_tick;
        mr_list_insert_sorted(&g_delayed_list, &current->state_node);
    }

    mr_port_exit_critical();

    /* Switch to another task */
    mr_port_yield();

    /* We're back - check if we got the mutex or timed out */
    mr_port_enter_critical();

    if (mutex->owner == current) {
        status = MR_OK;
    } else {
        /* Timed out - remove from wait list if still there */
        if (mr_list_node_is_linked(&current->event_node)) {
            mr_list_remove(&mutex->wait_list, &current->event_node);
        }
        status = MR_ERR_TIMEOUT;
    }

    current->blocked_on = NULL;
    current->block_reason = MR_BLOCK_NONE;

    mr_port_exit_critical();

    return status;
}

mr_status_t mr_mutex_trylock(mr_mutex_t *mutex)
{
    return mr_mutex_lock(mutex, MR_NO_WAIT);
}

mr_status_t mr_mutex_unlock(mr_mutex_t *mutex)
{
    mr_tcb_t *current;
    mr_tcb_t *next_owner;
    mr_list_node_t *node;
    mr_mutex_t **mutex_ptr;
    bool need_switch = false;

    MR_ASSERT(mutex != NULL);
    MR_ASSERT(!mr_port_is_in_isr());

    mr_port_enter_critical();

    current = g_current_tcb;

    /* Verify we own this mutex */
    if (mutex->owner != current) {
        mr_port_exit_critical();
        return MR_ERR_NOT_OWNER;
    }

    /* Handle recursive unlock */
    if (mutex->lock_count > 1) {
        mutex->lock_count--;
        mr_port_exit_critical();
        return MR_OK;
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
    node = mr_list_remove_head(&mutex->wait_list);

    if (node != NULL) {
        /* Give mutex to highest priority waiter */
        next_owner = MR_TCB_FROM_EVENT_NODE(node);

        /* Remove from delayed list if it was there */
        if (mr_list_node_is_linked(&next_owner->state_node)) {
            mr_list_remove(&g_delayed_list, &next_owner->state_node);
        }

        /* Transfer ownership */
        mutex->owner = next_owner;
        mutex->lock_count = 1;

        /* Add to new owner's held list */
        mutex->next = next_owner->mutex_held;
        next_owner->mutex_held = mutex;
        next_owner->mutexes_held_count++;

        /* Unblock the new owner */
        next_owner->state = MR_TASK_READY;
        next_owner->block_reason = MR_BLOCK_NONE;
        next_owner->blocked_on = NULL;
        mr_scheduler_add_ready(next_owner);

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

    mr_port_exit_critical();

    if (need_switch && g_kernel_state == MR_KERNEL_RUNNING) {
        mr_port_yield();
    }

    return MR_OK;
}

mr_tcb_t *mr_mutex_get_owner(mr_mutex_t *mutex)
{
    if (mutex == NULL) {
        return NULL;
    }
    return mutex->owner;
}

bool mr_mutex_is_locked(mr_mutex_t *mutex)
{
    if (mutex == NULL) {
        return false;
    }
    return (mutex->owner != NULL);
}

#endif /* MR_USE_MUTEXES */
