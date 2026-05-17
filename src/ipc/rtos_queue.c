/**
 * MicroRTOS - Message Queue Implementation
 *
 * Thread-safe circular buffer queues.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos_queue.h"
#include "rtos_list.h"
#include "rtos_port.h"
#include "rtos_task.h"
#include <string.h>

#if RTOS_USE_QUEUES

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
/* Helper Functions                                                           */
/*===========================================================================*/

/**
 * Copy an item to the queue at a specific position.
 */
static void queue_copy_in(rtos_queue_t *queue, const void *item, uint16_t pos)
{
    uint8_t *dest = queue->buffer + (pos * queue->item_size);
    memcpy(dest, item, queue->item_size);
}

/**
 * Copy an item from the queue at a specific position.
 */
static void queue_copy_out(rtos_queue_t *queue, void *item, uint16_t pos)
{
    uint8_t *src = queue->buffer + (pos * queue->item_size);
    memcpy(item, src, queue->item_size);
}

/**
 * Unblock a waiting task.
 */
static bool unblock_waiter(rtos_list_t *wait_list)
{
    rtos_tcb_t *waiter;
    rtos_list_node_t *node;

    node = rtos_list_remove_head(wait_list);
    if (node == NULL) {
        return false;
    }

    waiter = RTOS_TCB_FROM_EVENT_NODE(node);

    /* Remove from delayed list if it was there */
    if (rtos_list_node_is_linked(&waiter->state_node)) {
        rtos_list_remove(&g_delayed_list, &waiter->state_node);
    }

    /* Unblock */
    waiter->state = RTOS_TASK_READY;
    waiter->block_reason = RTOS_BLOCK_NONE;
    waiter->blocked_on = NULL;
    rtos_scheduler_add_ready(waiter);

    return true;
}

/*===========================================================================*/
/* Queue API Implementation                                                   */
/*===========================================================================*/

void rtos_queue_init(
    rtos_queue_t *queue,
    void *buffer,
    uint16_t item_size,
    uint16_t capacity)
{
    RTOS_ASSERT(queue != NULL);
    RTOS_ASSERT(buffer != NULL);
    RTOS_ASSERT(item_size > 0);
    RTOS_ASSERT(capacity > 0);

    queue->buffer = (uint8_t *)buffer;
    queue->item_size = item_size;
    queue->capacity = capacity;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
    rtos_list_init(&queue->send_wait_list);
    rtos_list_init(&queue->recv_wait_list);
}

rtos_status_t rtos_queue_send(
    rtos_queue_t *queue,
    const void *item,
    uint32_t timeout)
{
    rtos_tcb_t *current;
    rtos_status_t status = RTOS_OK;
    bool need_switch = false;

    RTOS_ASSERT(queue != NULL);
    RTOS_ASSERT(item != NULL);
    RTOS_ASSERT(!rtos_port_is_in_isr());

    rtos_port_enter_critical();

    current = g_current_tcb;

    /* Check if queue has space */
    if (queue->count < queue->capacity) {
        /* Add item to tail */
        queue_copy_in(queue, item, queue->tail);
        queue->tail = (queue->tail + 1) % queue->capacity;
        queue->count++;

        /* Wake any waiting receivers */
        if (unblock_waiter(&queue->recv_wait_list)) {
            rtos_list_node_t *node = rtos_list_peek_head(&queue->recv_wait_list);
            if (node == NULL) {
                node = queue->recv_wait_list.head;
            }
            /* Check after unblocking if we just unblocked someone */
            need_switch = true;
        }

        rtos_port_exit_critical();

        if (need_switch && g_kernel_state == RTOS_KERNEL_RUNNING) {
            rtos_port_yield();
        }
        return RTOS_OK;
    }

    /* Queue is full - need to wait */
    if (timeout == RTOS_NO_WAIT) {
        rtos_port_exit_critical();
        return RTOS_ERR_FULL;
    }

    /* Block waiting for space */
    rtos_scheduler_remove_ready(current);
    current->state = RTOS_TASK_BLOCKED;
    current->block_reason = RTOS_BLOCK_QUEUE_SEND;
    current->blocked_on = queue;

    /* Add to send wait list */
    current->event_node.value = current->priority;
    rtos_list_insert_priority(&queue->send_wait_list, &current->event_node);

    /* Set up timeout */
    if (timeout != RTOS_WAIT_FOREVER) {
        current->wake_tick = g_tick_count + timeout;
        current->state_node.value = current->wake_tick;
        rtos_list_insert_sorted(&g_delayed_list, &current->state_node);
    }

    rtos_port_exit_critical();

    /* Switch to another task */
    rtos_port_yield();

    /* We're back - try to send again or report timeout */
    rtos_port_enter_critical();

    /* Check if we're still waiting */
    if (rtos_list_node_is_linked(&current->event_node)) {
        rtos_list_remove(&queue->send_wait_list, &current->event_node);
        status = RTOS_ERR_TIMEOUT;
    } else {
        /* We were unblocked - space should be available */
        if (queue->count < queue->capacity) {
            queue_copy_in(queue, item, queue->tail);
            queue->tail = (queue->tail + 1) % queue->capacity;
            queue->count++;
            status = RTOS_OK;
        } else {
            status = RTOS_ERR_FULL;
        }
    }

    current->blocked_on = NULL;
    current->block_reason = RTOS_BLOCK_NONE;

    rtos_port_exit_critical();

    return status;
}

rtos_status_t rtos_queue_send_front(
    rtos_queue_t *queue,
    const void *item,
    uint32_t timeout)
{
    rtos_status_t status = RTOS_OK;
    bool need_switch = false;

    RTOS_ASSERT(queue != NULL);
    RTOS_ASSERT(item != NULL);
    RTOS_ASSERT(!rtos_port_is_in_isr());

    rtos_port_enter_critical();

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

        rtos_port_exit_critical();

        if (need_switch && g_kernel_state == RTOS_KERNEL_RUNNING) {
            rtos_port_yield();
        }
        return RTOS_OK;
    }

    /* Queue is full - handle same as regular send */
    if (timeout == RTOS_NO_WAIT) {
        rtos_port_exit_critical();
        return RTOS_ERR_FULL;
    }

    /* Block and retry (simplified - reuses send logic) */
    rtos_port_exit_critical();
    status = rtos_queue_send(queue, item, timeout);

    return status;
}

rtos_status_t rtos_queue_receive(
    rtos_queue_t *queue,
    void *item,
    uint32_t timeout)
{
    rtos_tcb_t *current;
    rtos_status_t status = RTOS_OK;
    bool need_switch = false;

    RTOS_ASSERT(queue != NULL);
    RTOS_ASSERT(item != NULL);
    RTOS_ASSERT(!rtos_port_is_in_isr());

    rtos_port_enter_critical();

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

        rtos_port_exit_critical();

        if (need_switch && g_kernel_state == RTOS_KERNEL_RUNNING) {
            rtos_port_yield();
        }
        return RTOS_OK;
    }

    /* Queue is empty - need to wait */
    if (timeout == RTOS_NO_WAIT) {
        rtos_port_exit_critical();
        return RTOS_ERR_EMPTY;
    }

    /* Block waiting for data */
    rtos_scheduler_remove_ready(current);
    current->state = RTOS_TASK_BLOCKED;
    current->block_reason = RTOS_BLOCK_QUEUE_RECV;
    current->blocked_on = queue;

    /* Add to receive wait list */
    current->event_node.value = current->priority;
    rtos_list_insert_priority(&queue->recv_wait_list, &current->event_node);

    /* Set up timeout */
    if (timeout != RTOS_WAIT_FOREVER) {
        current->wake_tick = g_tick_count + timeout;
        current->state_node.value = current->wake_tick;
        rtos_list_insert_sorted(&g_delayed_list, &current->state_node);
    }

    rtos_port_exit_critical();

    /* Switch to another task */
    rtos_port_yield();

    /* We're back - try to receive or report timeout */
    rtos_port_enter_critical();

    /* Check if we're still waiting */
    if (rtos_list_node_is_linked(&current->event_node)) {
        rtos_list_remove(&queue->recv_wait_list, &current->event_node);
        status = RTOS_ERR_TIMEOUT;
    } else {
        /* We were unblocked - data should be available */
        if (queue->count > 0) {
            queue_copy_out(queue, item, queue->head);
            queue->head = (queue->head + 1) % queue->capacity;
            queue->count--;
            status = RTOS_OK;
        } else {
            status = RTOS_ERR_EMPTY;
        }
    }

    current->blocked_on = NULL;
    current->block_reason = RTOS_BLOCK_NONE;

    rtos_port_exit_critical();

    return status;
}

rtos_status_t rtos_queue_peek(
    rtos_queue_t *queue,
    void *item,
    uint32_t timeout)
{
    rtos_tcb_t *current;
    rtos_status_t status = RTOS_OK;

    RTOS_ASSERT(queue != NULL);
    RTOS_ASSERT(item != NULL);
    RTOS_ASSERT(!rtos_port_is_in_isr());

    rtos_port_enter_critical();

    current = g_current_tcb;

    /* Check if queue has items */
    if (queue->count > 0) {
        /* Copy item without removing */
        queue_copy_out(queue, item, queue->head);
        rtos_port_exit_critical();
        return RTOS_OK;
    }

    /* Queue is empty - need to wait */
    if (timeout == RTOS_NO_WAIT) {
        rtos_port_exit_critical();
        return RTOS_ERR_EMPTY;
    }

    /* Block waiting for data */
    rtos_scheduler_remove_ready(current);
    current->state = RTOS_TASK_BLOCKED;
    current->block_reason = RTOS_BLOCK_QUEUE_RECV;
    current->blocked_on = queue;

    current->event_node.value = current->priority;
    rtos_list_insert_priority(&queue->recv_wait_list, &current->event_node);

    if (timeout != RTOS_WAIT_FOREVER) {
        current->wake_tick = g_tick_count + timeout;
        current->state_node.value = current->wake_tick;
        rtos_list_insert_sorted(&g_delayed_list, &current->state_node);
    }

    rtos_port_exit_critical();
    rtos_port_yield();

    rtos_port_enter_critical();

    if (rtos_list_node_is_linked(&current->event_node)) {
        rtos_list_remove(&queue->recv_wait_list, &current->event_node);
        status = RTOS_ERR_TIMEOUT;
    } else {
        if (queue->count > 0) {
            queue_copy_out(queue, item, queue->head);
            status = RTOS_OK;
        } else {
            status = RTOS_ERR_EMPTY;
        }
    }

    current->blocked_on = NULL;
    current->block_reason = RTOS_BLOCK_NONE;

    rtos_port_exit_critical();

    return status;
}

rtos_status_t rtos_queue_send_from_isr(
    rtos_queue_t *queue,
    const void *item,
    bool *yield_required)
{
    rtos_tcb_t *waiter;
    rtos_list_node_t *node;

    RTOS_ASSERT(queue != NULL);
    RTOS_ASSERT(item != NULL);

    if (yield_required != NULL) {
        *yield_required = false;
    }

    /* Check if queue has space */
    if (queue->count >= queue->capacity) {
        return RTOS_ERR_FULL;
    }

    /* Add item */
    queue_copy_in(queue, item, queue->tail);
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;

    /* Wake any waiting receivers */
    node = rtos_list_remove_head(&queue->recv_wait_list);
    if (node != NULL) {
        waiter = RTOS_TCB_FROM_EVENT_NODE(node);

        if (rtos_list_node_is_linked(&waiter->state_node)) {
            rtos_list_remove(&g_delayed_list, &waiter->state_node);
        }

        waiter->state = RTOS_TASK_READY;
        waiter->block_reason = RTOS_BLOCK_NONE;
        waiter->blocked_on = NULL;
        rtos_scheduler_add_ready(waiter);

        if (yield_required != NULL &&
            g_current_tcb != NULL &&
            waiter->priority < g_current_tcb->priority) {
            *yield_required = true;
        }
    }

    return RTOS_OK;
}

rtos_status_t rtos_queue_receive_from_isr(
    rtos_queue_t *queue,
    void *item,
    bool *yield_required)
{
    rtos_tcb_t *waiter;
    rtos_list_node_t *node;

    RTOS_ASSERT(queue != NULL);
    RTOS_ASSERT(item != NULL);

    if (yield_required != NULL) {
        *yield_required = false;
    }

    /* Check if queue has items */
    if (queue->count == 0) {
        return RTOS_ERR_EMPTY;
    }

    /* Get item */
    queue_copy_out(queue, item, queue->head);
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;

    /* Wake any waiting senders */
    node = rtos_list_remove_head(&queue->send_wait_list);
    if (node != NULL) {
        waiter = RTOS_TCB_FROM_EVENT_NODE(node);

        if (rtos_list_node_is_linked(&waiter->state_node)) {
            rtos_list_remove(&g_delayed_list, &waiter->state_node);
        }

        waiter->state = RTOS_TASK_READY;
        waiter->block_reason = RTOS_BLOCK_NONE;
        waiter->blocked_on = NULL;
        rtos_scheduler_add_ready(waiter);

        if (yield_required != NULL &&
            g_current_tcb != NULL &&
            waiter->priority < g_current_tcb->priority) {
            *yield_required = true;
        }
    }

    return RTOS_OK;
}

uint16_t rtos_queue_count(rtos_queue_t *queue)
{
    return (queue != NULL) ? queue->count : 0;
}

uint16_t rtos_queue_spaces_available(rtos_queue_t *queue)
{
    return (queue != NULL) ? (queue->capacity - queue->count) : 0;
}

bool rtos_queue_is_empty(rtos_queue_t *queue)
{
    return (queue != NULL) ? (queue->count == 0) : true;
}

bool rtos_queue_is_full(rtos_queue_t *queue)
{
    return (queue != NULL) ? (queue->count >= queue->capacity) : true;
}

void rtos_queue_reset(rtos_queue_t *queue)
{
    if (queue == NULL) {
        return;
    }

    rtos_port_enter_critical();
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
    rtos_port_exit_critical();
}

#endif /* RTOS_USE_QUEUES */
