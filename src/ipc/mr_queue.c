/**
 * MicroRTOS - Message Queue Implementation
 *
 * Thread-safe circular buffer queues.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_queue.h"
#include "mr_list.h"
#include "mr_port.h"
#include "mr_task.h"
#include <string.h>

#if MR_USE_QUEUES

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
/* Helper Functions                                                           */
/*===========================================================================*/

/**
 * Copy an item to the queue at a specific position.
 */
static void queue_copy_in(mr_queue_t *queue, const void *item, uint16_t pos)
{
    uint8_t *dest = queue->buffer + (pos * queue->item_size);
    memcpy(dest, item, queue->item_size);
}

/**
 * Copy an item from the queue at a specific position.
 */
static void queue_copy_out(mr_queue_t *queue, void *item, uint16_t pos)
{
    uint8_t *src = queue->buffer + (pos * queue->item_size);
    memcpy(item, src, queue->item_size);
}

/**
 * Unblock a waiting task.
 */
static bool unblock_waiter(mr_list_t *wait_list)
{
    mr_tcb_t *waiter;
    mr_list_node_t *node;

    node = mr_list_remove_head(wait_list);
    if (node == NULL) {
        return false;
    }

    waiter = MR_TCB_FROM_EVENT_NODE(node);

    /* Remove from delayed list if it was there */
    if (mr_list_node_is_linked(&waiter->state_node)) {
        mr_list_remove(&g_delayed_list, &waiter->state_node);
    }

    /* Unblock */
    waiter->state = MR_TASK_READY;
    waiter->block_reason = MR_BLOCK_NONE;
    waiter->blocked_on = NULL;
    mr_scheduler_add_ready(waiter);

    return true;
}

/*===========================================================================*/
/* Queue API Implementation                                                   */
/*===========================================================================*/

void mr_queue_init(
    mr_queue_t *queue,
    void *buffer,
    uint16_t item_size,
    uint16_t capacity)
{
    MR_ASSERT(queue != NULL);
    MR_ASSERT(buffer != NULL);
    MR_ASSERT(item_size > 0);
    MR_ASSERT(capacity > 0);

    queue->buffer = (uint8_t *)buffer;
    queue->item_size = item_size;
    queue->capacity = capacity;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
    mr_list_init(&queue->send_wait_list);
    mr_list_init(&queue->recv_wait_list);
}

mr_status_t mr_queue_send(
    mr_queue_t *queue,
    const void *item,
    uint32_t timeout)
{
    mr_tcb_t *current;
    mr_status_t status = MR_OK;
    bool need_switch = false;

    MR_ASSERT(queue != NULL);
    MR_ASSERT(item != NULL);
    MR_ASSERT(!mr_port_is_in_isr());

    mr_port_enter_critical();

    current = g_current_tcb;

    /* Check if queue has space */
    if (queue->count < queue->capacity) {
        /* Add item to tail */
        queue_copy_in(queue, item, queue->tail);
        queue->tail = (queue->tail + 1) % queue->capacity;
        queue->count++;

        /* Wake any waiting receivers */
        if (unblock_waiter(&queue->recv_wait_list)) {
            mr_list_node_t *node = mr_list_peek_head(&queue->recv_wait_list);
            if (node == NULL) {
                node = queue->recv_wait_list.head;
            }
            /* Check after unblocking if we just unblocked someone */
            need_switch = true;
        }

        mr_port_exit_critical();

        if (need_switch && g_kernel_state == MR_KERNEL_RUNNING) {
            mr_port_yield();
        }
        return MR_OK;
    }

    /* Queue is full - need to wait */
    if (timeout == MR_NO_WAIT) {
        mr_port_exit_critical();
        return MR_ERR_FULL;
    }

    /* Block waiting for space */
    mr_scheduler_remove_ready(current);
    current->state = MR_TASK_BLOCKED;
    current->block_reason = MR_BLOCK_QUEUE_SEND;
    current->blocked_on = queue;

    /* Add to send wait list */
    current->event_node.value = current->priority;
    mr_list_insert_priority(&queue->send_wait_list, &current->event_node);

    /* Set up timeout */
    if (timeout != MR_WAIT_FOREVER) {
        current->wake_tick = g_tick_count + timeout;
        current->state_node.value = current->wake_tick;
        mr_list_insert_sorted(&g_delayed_list, &current->state_node);
    }

    mr_port_exit_critical();

    /* Switch to another task */
    mr_port_yield();

    /* We're back - try to send again or report timeout */
    mr_port_enter_critical();

    /* Check if we're still waiting */
    if (mr_list_node_is_linked(&current->event_node)) {
        mr_list_remove(&queue->send_wait_list, &current->event_node);
        status = MR_ERR_TIMEOUT;
    } else {
        /* We were unblocked - space should be available */
        if (queue->count < queue->capacity) {
            queue_copy_in(queue, item, queue->tail);
            queue->tail = (queue->tail + 1) % queue->capacity;
            queue->count++;
            status = MR_OK;
        } else {
            status = MR_ERR_FULL;
        }
    }

    current->blocked_on = NULL;
    current->block_reason = MR_BLOCK_NONE;

    mr_port_exit_critical();

    return status;
}

mr_status_t mr_queue_send_front(
    mr_queue_t *queue,
    const void *item,
    uint32_t timeout)
{
    mr_status_t status = MR_OK;
    bool need_switch = false;

    MR_ASSERT(queue != NULL);
    MR_ASSERT(item != NULL);
    MR_ASSERT(!mr_port_is_in_isr());

    mr_port_enter_critical();

    /* Check if queue has space */
    if (queue->count < queue->capacity) {
        /* Add item to front (before head) */
        queue->head = (queue->head == 0) ? (queue->capacity - 1) : (queue->head - 1);
        queue_copy_in(queue, item, queue->head);
        queue->count++;

        /* Wake any waiting receivers */
        if (unblock_waiter(&queue->recv_wait_list)) {
            need_switch = true;
        }

        mr_port_exit_critical();

        if (need_switch && g_kernel_state == MR_KERNEL_RUNNING) {
            mr_port_yield();
        }
        return MR_OK;
    }

    /* Queue is full - handle same as regular send */
    if (timeout == MR_NO_WAIT) {
        mr_port_exit_critical();
        return MR_ERR_FULL;
    }

    /* Block and retry (simplified - reuses send logic) */
    mr_port_exit_critical();
    status = mr_queue_send(queue, item, timeout);

    return status;
}

mr_status_t mr_queue_receive(
    mr_queue_t *queue,
    void *item,
    uint32_t timeout)
{
    mr_tcb_t *current;
    mr_status_t status = MR_OK;
    bool need_switch = false;

    MR_ASSERT(queue != NULL);
    MR_ASSERT(item != NULL);
    MR_ASSERT(!mr_port_is_in_isr());

    mr_port_enter_critical();

    current = g_current_tcb;

    /* Check if queue has items */
    if (queue->count > 0) {
        /* Get item from head */
        queue_copy_out(queue, item, queue->head);
        queue->head = (queue->head + 1) % queue->capacity;
        queue->count--;

        /* Wake any waiting senders */
        if (unblock_waiter(&queue->send_wait_list)) {
            need_switch = true;
        }

        mr_port_exit_critical();

        if (need_switch && g_kernel_state == MR_KERNEL_RUNNING) {
            mr_port_yield();
        }
        return MR_OK;
    }

    /* Queue is empty - need to wait */
    if (timeout == MR_NO_WAIT) {
        mr_port_exit_critical();
        return MR_ERR_EMPTY;
    }

    /* Block waiting for data */
    mr_scheduler_remove_ready(current);
    current->state = MR_TASK_BLOCKED;
    current->block_reason = MR_BLOCK_QUEUE_RECV;
    current->blocked_on = queue;

    /* Add to receive wait list */
    current->event_node.value = current->priority;
    mr_list_insert_priority(&queue->recv_wait_list, &current->event_node);

    /* Set up timeout */
    if (timeout != MR_WAIT_FOREVER) {
        current->wake_tick = g_tick_count + timeout;
        current->state_node.value = current->wake_tick;
        mr_list_insert_sorted(&g_delayed_list, &current->state_node);
    }

    mr_port_exit_critical();

    /* Switch to another task */
    mr_port_yield();

    /* We're back - try to receive or report timeout */
    mr_port_enter_critical();

    /* Check if we're still waiting */
    if (mr_list_node_is_linked(&current->event_node)) {
        mr_list_remove(&queue->recv_wait_list, &current->event_node);
        status = MR_ERR_TIMEOUT;
    } else {
        /* We were unblocked - data should be available */
        if (queue->count > 0) {
            queue_copy_out(queue, item, queue->head);
            queue->head = (queue->head + 1) % queue->capacity;
            queue->count--;
            status = MR_OK;
        } else {
            status = MR_ERR_EMPTY;
        }
    }

    current->blocked_on = NULL;
    current->block_reason = MR_BLOCK_NONE;

    mr_port_exit_critical();

    return status;
}

mr_status_t mr_queue_peek(
    mr_queue_t *queue,
    void *item,
    uint32_t timeout)
{
    mr_tcb_t *current;
    mr_status_t status = MR_OK;

    MR_ASSERT(queue != NULL);
    MR_ASSERT(item != NULL);
    MR_ASSERT(!mr_port_is_in_isr());

    mr_port_enter_critical();

    current = g_current_tcb;

    /* Check if queue has items */
    if (queue->count > 0) {
        /* Copy item without removing */
        queue_copy_out(queue, item, queue->head);
        mr_port_exit_critical();
        return MR_OK;
    }

    /* Queue is empty - need to wait */
    if (timeout == MR_NO_WAIT) {
        mr_port_exit_critical();
        return MR_ERR_EMPTY;
    }

    /* Block waiting for data */
    mr_scheduler_remove_ready(current);
    current->state = MR_TASK_BLOCKED;
    current->block_reason = MR_BLOCK_QUEUE_RECV;
    current->blocked_on = queue;

    current->event_node.value = current->priority;
    mr_list_insert_priority(&queue->recv_wait_list, &current->event_node);

    if (timeout != MR_WAIT_FOREVER) {
        current->wake_tick = g_tick_count + timeout;
        current->state_node.value = current->wake_tick;
        mr_list_insert_sorted(&g_delayed_list, &current->state_node);
    }

    mr_port_exit_critical();
    mr_port_yield();

    mr_port_enter_critical();

    if (mr_list_node_is_linked(&current->event_node)) {
        mr_list_remove(&queue->recv_wait_list, &current->event_node);
        status = MR_ERR_TIMEOUT;
    } else {
        if (queue->count > 0) {
            queue_copy_out(queue, item, queue->head);
            status = MR_OK;
        } else {
            status = MR_ERR_EMPTY;
        }
    }

    current->blocked_on = NULL;
    current->block_reason = MR_BLOCK_NONE;

    mr_port_exit_critical();

    return status;
}

mr_status_t mr_queue_send_from_isr(
    mr_queue_t *queue,
    const void *item,
    bool *yield_required)
{
    mr_tcb_t *waiter;
    mr_list_node_t *node;

    MR_ASSERT(queue != NULL);
    MR_ASSERT(item != NULL);

    if (yield_required != NULL) {
        *yield_required = false;
    }

    /* Check if queue has space */
    if (queue->count >= queue->capacity) {
        return MR_ERR_FULL;
    }

    /* Add item */
    queue_copy_in(queue, item, queue->tail);
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;

    /* Wake any waiting receivers */
    node = mr_list_remove_head(&queue->recv_wait_list);
    if (node != NULL) {
        waiter = MR_TCB_FROM_EVENT_NODE(node);

        if (mr_list_node_is_linked(&waiter->state_node)) {
            mr_list_remove(&g_delayed_list, &waiter->state_node);
        }

        waiter->state = MR_TASK_READY;
        waiter->block_reason = MR_BLOCK_NONE;
        waiter->blocked_on = NULL;
        mr_scheduler_add_ready(waiter);

        if (yield_required != NULL &&
            g_current_tcb != NULL &&
            waiter->priority < g_current_tcb->priority) {
            *yield_required = true;
        }
    }

    return MR_OK;
}

mr_status_t mr_queue_receive_from_isr(
    mr_queue_t *queue,
    void *item,
    bool *yield_required)
{
    mr_tcb_t *waiter;
    mr_list_node_t *node;

    MR_ASSERT(queue != NULL);
    MR_ASSERT(item != NULL);

    if (yield_required != NULL) {
        *yield_required = false;
    }

    /* Check if queue has items */
    if (queue->count == 0) {
        return MR_ERR_EMPTY;
    }

    /* Get item */
    queue_copy_out(queue, item, queue->head);
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;

    /* Wake any waiting senders */
    node = mr_list_remove_head(&queue->send_wait_list);
    if (node != NULL) {
        waiter = MR_TCB_FROM_EVENT_NODE(node);

        if (mr_list_node_is_linked(&waiter->state_node)) {
            mr_list_remove(&g_delayed_list, &waiter->state_node);
        }

        waiter->state = MR_TASK_READY;
        waiter->block_reason = MR_BLOCK_NONE;
        waiter->blocked_on = NULL;
        mr_scheduler_add_ready(waiter);

        if (yield_required != NULL &&
            g_current_tcb != NULL &&
            waiter->priority < g_current_tcb->priority) {
            *yield_required = true;
        }
    }

    return MR_OK;
}

uint16_t mr_queue_count(mr_queue_t *queue)
{
    return (queue != NULL) ? queue->count : 0;
}

uint16_t mr_queue_spaces_available(mr_queue_t *queue)
{
    return (queue != NULL) ? (queue->capacity - queue->count) : 0;
}

bool mr_queue_is_empty(mr_queue_t *queue)
{
    return (queue != NULL) ? (queue->count == 0) : true;
}

bool mr_queue_is_full(mr_queue_t *queue)
{
    return (queue != NULL) ? (queue->count >= queue->capacity) : true;
}

void mr_queue_reset(mr_queue_t *queue)
{
    if (queue == NULL) {
        return;
    }

    mr_port_enter_critical();
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
    mr_port_exit_critical();
}

#endif /* MR_USE_QUEUES */
