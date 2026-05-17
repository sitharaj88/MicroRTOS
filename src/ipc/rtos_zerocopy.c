/**
 * MicroRTOS - Zero-Copy Messaging Implementation
 *
 * High-performance buffer ownership transfer without data copying.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos_zerocopy.h"
#include "rtos_types.h"
#include "rtos_list.h"
#include "rtos_atomic.h"
#include "rtos_memory.h"
#include "rtos_port.h"

#if RTOS_USE_ZEROCOPY

/*===========================================================================*/
/* External References                                                        */
/*===========================================================================*/

extern rtos_tcb_t *g_current_tcb;
extern volatile uint32_t g_tick_count;
extern rtos_list_t g_delayed_list;
extern void rtos_scheduler_add_ready(rtos_tcb_t *tcb);

/*===========================================================================*/
/* Helpers                                                                    */
/*===========================================================================*/

static bool zc_unblock_waiter(rtos_list_t *wait_list)
{
    rtos_list_node_t *node;
    rtos_tcb_t *waiter;

    node = rtos_list_remove_head(wait_list);
    if (node == NULL) {
        return false;
    }

    waiter = RTOS_TCB_FROM_EVENT_NODE(node);

    if (rtos_list_node_is_linked(&waiter->state_node)) {
        rtos_list_remove(&g_delayed_list, &waiter->state_node);
    }

    waiter->state = RTOS_TASK_READY;
    waiter->block_reason = RTOS_BLOCK_NONE;
    waiter->blocked_on = NULL;
    rtos_scheduler_add_ready(waiter);
    return true;
}

/*===========================================================================*/
/* Buffer Pool Integration                                                    */
/*===========================================================================*/

rtos_status_t rtos_zc_pool_init(rtos_mempool_t *pool,
                                 uint16_t buffer_size,
                                 uint16_t buffer_count,
                                 void *memory,
                                 uint32_t memory_size)
{
    uint32_t block_size;
    uint32_t required_size;

    if (pool == NULL || memory == NULL || buffer_count == 0) {
        return RTOS_ERR_PARAM;
    }

    /* Each block needs space for descriptor + data */
    block_size = sizeof(rtos_zc_buffer_t) + buffer_size;

    /* Align block size to 4 bytes */
    block_size = (block_size + 3) & ~3;

    required_size = block_size * buffer_count;

    if (memory_size < required_size) {
        return RTOS_ERR_NO_MEMORY;
    }

    /* Initialize the underlying memory pool */
    rtos_mempool_init(pool, memory, (uint16_t)block_size, buffer_count);
    return RTOS_OK;
}

/*===========================================================================*/
/* Buffer Management                                                          */
/*===========================================================================*/

rtos_zc_buffer_t *rtos_zc_buffer_alloc(rtos_mempool_t *pool,
                                        uint16_t size,
                                        uint32_t timeout)
{
    rtos_zc_buffer_t *buf;
    void *block;

    if (pool == NULL) {
        return NULL;
    }

    /* Allocate from memory pool */
    block = rtos_mempool_alloc(pool, timeout);
    if (block == NULL) {
        return NULL;
    }

    /* Initialize buffer descriptor */
    buf = (rtos_zc_buffer_t *)block;
    buf->data = (uint8_t *)block + sizeof(rtos_zc_buffer_t);
    buf->size = pool->block_size - sizeof(rtos_zc_buffer_t);
    buf->length = 0;
    buf->ref_count = 1;
    buf->flags = RTOS_ZC_FLAG_NONE;
    buf->pool = pool;
    buf->next = NULL;

    /* Check if allocated size meets requirements */
    if (buf->size < size) {
        rtos_mempool_free(pool, block);
        return NULL;
    }

    return buf;
}

rtos_zc_buffer_t *rtos_zc_buffer_alloc_nowait(rtos_mempool_t *pool,
                                               uint16_t size)
{
    return rtos_zc_buffer_alloc(pool, size, RTOS_NO_WAIT);
}

void rtos_zc_buffer_ref(rtos_zc_buffer_t *buf)
{
    if (buf == NULL || (buf->flags & RTOS_ZC_FLAG_STATIC)) {
        return;
    }

    rtos_atomic_fetch_add((volatile uint32_t *)&buf->ref_count, 1);
}

void rtos_zc_buffer_unref(rtos_zc_buffer_t *buf)
{
    uint8_t old_count;

    if (buf == NULL || (buf->flags & RTOS_ZC_FLAG_STATIC)) {
        return;
    }

    /* Atomically decrement reference count */
    old_count = (uint8_t)rtos_atomic_fetch_sub(
        (volatile uint32_t *)&buf->ref_count, 1);

    if (old_count == 1) {
        /* Reference count is now 0, free the buffer */
        if (buf->pool != NULL) {
            rtos_mempool_free(buf->pool, buf);
        }
    }
}

void rtos_zc_buffer_free(rtos_zc_buffer_t *buf)
{
    if (buf == NULL || (buf->flags & RTOS_ZC_FLAG_STATIC)) {
        return;
    }

    /* Force free regardless of reference count */
    buf->ref_count = 0;

    if (buf->pool != NULL) {
        rtos_mempool_free(buf->pool, buf);
    }
}

uint8_t rtos_zc_buffer_get_ref_count(rtos_zc_buffer_t *buf)
{
    if (buf == NULL) {
        return 0;
    }
    return buf->ref_count;
}

void rtos_zc_buffer_set_length(rtos_zc_buffer_t *buf, uint16_t length)
{
    if (buf == NULL) {
        return;
    }

    if (length > buf->size) {
        length = buf->size;
    }

    buf->length = length;
}

/*===========================================================================*/
/* Zero-Copy Queue Implementation                                             */
/*===========================================================================*/

void rtos_zc_queue_init(rtos_zc_queue_t *queue,
                         rtos_zc_buffer_t **slots,
                         uint16_t capacity)
{
    uint16_t i;

    if (queue == NULL || slots == NULL || capacity == 0) {
        return;
    }

    queue->slots = slots;
    queue->capacity = capacity;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;

    /* Initialize wait lists */
    rtos_list_init(&queue->send_wait);
    rtos_list_init(&queue->recv_wait);

    /* Clear slots */
    for (i = 0; i < capacity; i++) {
        slots[i] = NULL;
    }
}

rtos_status_t rtos_zc_queue_send(rtos_zc_queue_t *queue,
                                  rtos_zc_buffer_t *buf,
                                  uint32_t timeout)
{
    uint32_t state;
    uint16_t next_head;
    uint32_t start_tick;
    uint32_t elapsed;

    if (queue == NULL || buf == NULL) {
        return RTOS_ERR_PARAM;
    }

    start_tick = g_tick_count;

    while (1) {
        state = rtos_port_disable_interrupts();

        /* Check if queue has space */
        next_head = (queue->head + 1) % queue->capacity;

        if (queue->count < queue->capacity) {
            /* Space available - insert buffer */
            queue->slots[queue->head] = buf;
            queue->head = next_head;
            queue->count++;

            /* Wake up any waiting receiver */
            (void)zc_unblock_waiter(&queue->recv_wait);

            rtos_port_restore_interrupts(state);
            return RTOS_OK;
        }

        /* Queue full - check timeout */
        if (timeout == RTOS_NO_WAIT) {
            rtos_port_restore_interrupts(state);
            return RTOS_ERR_TIMEOUT;
        }

        elapsed = g_tick_count - start_tick;
        if (timeout != RTOS_WAIT_FOREVER && elapsed >= timeout) {
            rtos_port_restore_interrupts(state);
            return RTOS_ERR_TIMEOUT;
        }

        /* Block waiting for space */
        g_current_tcb->event_node.value = g_current_tcb->priority;
        rtos_list_insert_priority(&queue->send_wait,
                                   &g_current_tcb->event_node);

        rtos_port_restore_interrupts(state);

        /* Yield to let other tasks run */
        rtos_port_yield();
    }
}

rtos_status_t rtos_zc_queue_send_front(rtos_zc_queue_t *queue,
                                        rtos_zc_buffer_t *buf,
                                        uint32_t timeout)
{
    uint32_t state;
    uint16_t new_tail;
    uint32_t start_tick;
    uint32_t elapsed;

    if (queue == NULL || buf == NULL) {
        return RTOS_ERR_PARAM;
    }

    start_tick = g_tick_count;

    while (1) {
        state = rtos_port_disable_interrupts();

        if (queue->count < queue->capacity) {
            /* Insert at front (before tail) */
            new_tail = (queue->tail == 0) ?
                        queue->capacity - 1 : queue->tail - 1;

            queue->slots[new_tail] = buf;
            queue->tail = new_tail;
            queue->count++;

            /* Wake up any waiting receiver */
            (void)zc_unblock_waiter(&queue->recv_wait);

            rtos_port_restore_interrupts(state);
            return RTOS_OK;
        }

        /* Queue full */
        if (timeout == RTOS_NO_WAIT) {
            rtos_port_restore_interrupts(state);
            return RTOS_ERR_TIMEOUT;
        }

        elapsed = g_tick_count - start_tick;
        if (timeout != RTOS_WAIT_FOREVER && elapsed >= timeout) {
            rtos_port_restore_interrupts(state);
            return RTOS_ERR_TIMEOUT;
        }

        g_current_tcb->event_node.value = g_current_tcb->priority;
        rtos_list_insert_priority(&queue->send_wait,
                                   &g_current_tcb->event_node);

        rtos_port_restore_interrupts(state);
        rtos_port_yield();
    }
}

rtos_status_t rtos_zc_queue_send_from_isr(rtos_zc_queue_t *queue,
                                           rtos_zc_buffer_t *buf,
                                           bool *higher_prio_woken)
{
    uint16_t next_head;

    if (queue == NULL || buf == NULL) {
        return RTOS_ERR_PARAM;
    }

    if (higher_prio_woken != NULL) {
        *higher_prio_woken = false;
    }

    if (queue->count >= queue->capacity) {
        return RTOS_ERR_FULL;
    }

    /* Insert buffer */
    next_head = (queue->head + 1) % queue->capacity;
    queue->slots[queue->head] = buf;
    queue->head = next_head;
    queue->count++;

    /* Wake up waiting receiver */
    if (!rtos_list_is_empty(&queue->recv_wait)) {
        rtos_list_node_t *node = rtos_list_peek_head(&queue->recv_wait);
        rtos_tcb_t *waiter = (node != NULL) ? RTOS_TCB_FROM_EVENT_NODE(node) : NULL;
        (void)zc_unblock_waiter(&queue->recv_wait);

        if (higher_prio_woken != NULL && waiter != NULL && g_current_tcb != NULL) {
            *higher_prio_woken = (waiter->priority < g_current_tcb->priority);
        }
    }

    return RTOS_OK;
}

rtos_zc_buffer_t *rtos_zc_queue_receive(rtos_zc_queue_t *queue,
                                         uint32_t timeout)
{
    uint32_t state;
    rtos_zc_buffer_t *buf;
    uint32_t start_tick;
    uint32_t elapsed;

    if (queue == NULL) {
        return NULL;
    }

    start_tick = g_tick_count;

    while (1) {
        state = rtos_port_disable_interrupts();

        if (queue->count > 0) {
            /* Get buffer from tail */
            buf = queue->slots[queue->tail];
            queue->slots[queue->tail] = NULL;
            queue->tail = (queue->tail + 1) % queue->capacity;
            queue->count--;

            /* Wake up any waiting sender */
            (void)zc_unblock_waiter(&queue->send_wait);

            rtos_port_restore_interrupts(state);
            return buf;
        }

        /* Queue empty */
        if (timeout == RTOS_NO_WAIT) {
            rtos_port_restore_interrupts(state);
            return NULL;
        }

        elapsed = g_tick_count - start_tick;
        if (timeout != RTOS_WAIT_FOREVER && elapsed >= timeout) {
            rtos_port_restore_interrupts(state);
            return NULL;
        }

        /* Block waiting for data */
        g_current_tcb->event_node.value = g_current_tcb->priority;
        rtos_list_insert_priority(&queue->recv_wait,
                                   &g_current_tcb->event_node);

        rtos_port_restore_interrupts(state);
        rtos_port_yield();
    }
}

rtos_zc_buffer_t *rtos_zc_queue_receive_from_isr(rtos_zc_queue_t *queue,
                                                  bool *higher_prio_woken)
{
    rtos_zc_buffer_t *buf;

    if (queue == NULL) {
        return NULL;
    }

    if (higher_prio_woken != NULL) {
        *higher_prio_woken = false;
    }

    if (queue->count == 0) {
        return NULL;
    }

    /* Get buffer */
    buf = queue->slots[queue->tail];
    queue->slots[queue->tail] = NULL;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count--;

    /* Wake up waiting sender */
    if (!rtos_list_is_empty(&queue->send_wait)) {
        rtos_list_node_t *node = rtos_list_peek_head(&queue->send_wait);
        rtos_tcb_t *waiter = (node != NULL) ? RTOS_TCB_FROM_EVENT_NODE(node) : NULL;
        (void)zc_unblock_waiter(&queue->send_wait);

        if (higher_prio_woken != NULL && waiter != NULL && g_current_tcb != NULL) {
            *higher_prio_woken = (waiter->priority < g_current_tcb->priority);
        }
    }

    return buf;
}

rtos_zc_buffer_t *rtos_zc_queue_peek(rtos_zc_queue_t *queue)
{
    if (queue == NULL || queue->count == 0) {
        return NULL;
    }

    return queue->slots[queue->tail];
}

bool rtos_zc_queue_is_empty(rtos_zc_queue_t *queue)
{
    return (queue == NULL || queue->count == 0);
}

bool rtos_zc_queue_is_full(rtos_zc_queue_t *queue)
{
    return (queue != NULL && queue->count >= queue->capacity);
}

uint16_t rtos_zc_queue_get_count(rtos_zc_queue_t *queue)
{
    return (queue != NULL) ? queue->count : 0;
}

uint16_t rtos_zc_queue_get_available(rtos_zc_queue_t *queue)
{
    return (queue != NULL) ? (queue->capacity - queue->count) : 0;
}

void rtos_zc_queue_flush(rtos_zc_queue_t *queue)
{
    uint32_t state;
    rtos_zc_buffer_t *buf;

    if (queue == NULL) {
        return;
    }

    state = rtos_port_disable_interrupts();

    while (queue->count > 0) {
        buf = queue->slots[queue->tail];
        queue->slots[queue->tail] = NULL;
        queue->tail = (queue->tail + 1) % queue->capacity;
        queue->count--;

        /* Unreference each buffer */
        if (buf != NULL) {
            rtos_zc_buffer_unref(buf);
        }
    }

    rtos_port_restore_interrupts(state);
}

/*===========================================================================*/
/* Scatter-Gather Implementation                                              */
/*===========================================================================*/

void rtos_zc_chain_init(rtos_zc_chain_t *chain)
{
    if (chain == NULL) {
        return;
    }

    chain->head = NULL;
    chain->tail = NULL;
    chain->count = 0;
    chain->total_len = 0;
}

void rtos_zc_chain_append(rtos_zc_chain_t *chain, rtos_zc_buffer_t *buf)
{
    if (chain == NULL || buf == NULL) {
        return;
    }

    buf->next = NULL;

    if (chain->tail == NULL) {
        chain->head = buf;
        chain->tail = buf;
    } else {
        chain->tail->next = buf;
        chain->tail = buf;
    }

    chain->count++;
    chain->total_len += buf->length;
}

void rtos_zc_chain_prepend(rtos_zc_chain_t *chain, rtos_zc_buffer_t *buf)
{
    if (chain == NULL || buf == NULL) {
        return;
    }

    buf->next = chain->head;
    chain->head = buf;

    if (chain->tail == NULL) {
        chain->tail = buf;
    }

    chain->count++;
    chain->total_len += buf->length;
}

uint32_t rtos_zc_chain_get_length(rtos_zc_chain_t *chain)
{
    return (chain != NULL) ? chain->total_len : 0;
}

void rtos_zc_chain_free(rtos_zc_chain_t *chain)
{
    rtos_zc_buffer_t *buf;
    rtos_zc_buffer_t *next;

    if (chain == NULL) {
        return;
    }

    buf = chain->head;
    while (buf != NULL) {
        next = buf->next;
        rtos_zc_buffer_unref(buf);
        buf = next;
    }

    chain->head = NULL;
    chain->tail = NULL;
    chain->count = 0;
    chain->total_len = 0;
}

#endif /* RTOS_USE_ZEROCOPY */
