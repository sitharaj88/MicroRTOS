/**
 * MicroRTOS - Zero-Copy Messaging Implementation
 *
 * High-performance buffer ownership transfer without data copying.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_zerocopy.h"
#include "mr_types.h"
#include "mr_list.h"
#include "mr_atomic.h"
#include "mr_memory.h"
#include "mr_port.h"

#if MR_USE_ZEROCOPY

/*===========================================================================*/
/* External References                                                        */
/*===========================================================================*/

extern mr_tcb_t *g_current_tcb;
extern volatile uint32_t g_tick_count;
extern mr_list_t g_delayed_list;
extern void mr_scheduler_add_ready(mr_tcb_t *tcb);

/*===========================================================================*/
/* Helpers                                                                    */
/*===========================================================================*/

static bool zc_unblock_waiter(mr_list_t *wait_list)
{
    mr_list_node_t *node;
    mr_tcb_t *waiter;

    node = mr_list_remove_head(wait_list);
    if (node == NULL) {
        return false;
    }

    waiter = MR_TCB_FROM_EVENT_NODE(node);

    if (mr_list_node_is_linked(&waiter->state_node)) {
        mr_list_remove(&g_delayed_list, &waiter->state_node);
    }

    waiter->state = MR_TASK_READY;
    waiter->block_reason = MR_BLOCK_NONE;
    waiter->blocked_on = NULL;
    mr_scheduler_add_ready(waiter);
    return true;
}

/*===========================================================================*/
/* Buffer Pool Integration                                                    */
/*===========================================================================*/

mr_status_t mr_zc_pool_init(mr_mempool_t *pool,
                                 uint16_t buffer_size,
                                 uint16_t buffer_count,
                                 void *memory,
                                 uint32_t memory_size)
{
    uint32_t block_size;
    uint32_t required_size;

    if (pool == NULL || memory == NULL || buffer_count == 0) {
        return MR_ERR_PARAM;
    }

    /* Each block needs space for descriptor + data */
    block_size = sizeof(mr_zc_buffer_t) + buffer_size;

    /* Align block size to 4 bytes */
    block_size = (block_size + 3) & ~3;

    required_size = block_size * buffer_count;

    if (memory_size < required_size) {
        return MR_ERR_NO_MEMORY;
    }

    /* Initialize the underlying memory pool */
    mr_mempool_init(pool, memory, (uint16_t)block_size, buffer_count);
    return MR_OK;
}

/*===========================================================================*/
/* Buffer Management                                                          */
/*===========================================================================*/

mr_zc_buffer_t *mr_zc_buffer_alloc(mr_mempool_t *pool,
                                        uint16_t size,
                                        uint32_t timeout)
{
    mr_zc_buffer_t *buf;
    void *block;

    if (pool == NULL) {
        return NULL;
    }

    /* Allocate from memory pool */
    block = mr_mempool_alloc(pool, timeout);
    if (block == NULL) {
        return NULL;
    }

    /* Initialize buffer descriptor */
    buf = (mr_zc_buffer_t *)block;
    buf->data = (uint8_t *)block + sizeof(mr_zc_buffer_t);
    buf->size = pool->block_size - sizeof(mr_zc_buffer_t);
    buf->length = 0;
    buf->ref_count = 1;
    buf->flags = MR_ZC_FLAG_NONE;
    buf->pool = pool;
    buf->next = NULL;

    /* Check if allocated size meets requirements */
    if (buf->size < size) {
        mr_mempool_free(pool, block);
        return NULL;
    }

    return buf;
}

mr_zc_buffer_t *mr_zc_buffer_alloc_nowait(mr_mempool_t *pool,
                                               uint16_t size)
{
    return mr_zc_buffer_alloc(pool, size, MR_NO_WAIT);
}

void mr_zc_buffer_ref(mr_zc_buffer_t *buf)
{
    if (buf == NULL || (buf->flags & MR_ZC_FLAG_STATIC)) {
        return;
    }

    mr_atomic_fetch_add((volatile uint32_t *)&buf->ref_count, 1);
}

void mr_zc_buffer_unref(mr_zc_buffer_t *buf)
{
    uint8_t old_count;

    if (buf == NULL || (buf->flags & MR_ZC_FLAG_STATIC)) {
        return;
    }

    /* Atomically decrement reference count */
    old_count = (uint8_t)mr_atomic_fetch_sub(
        (volatile uint32_t *)&buf->ref_count, 1);

    if (old_count == 1) {
        /* Reference count is now 0, free the buffer */
        if (buf->pool != NULL) {
            mr_mempool_free(buf->pool, buf);
        }
    }
}

void mr_zc_buffer_free(mr_zc_buffer_t *buf)
{
    if (buf == NULL || (buf->flags & MR_ZC_FLAG_STATIC)) {
        return;
    }

    /* Force free regardless of reference count */
    buf->ref_count = 0;

    if (buf->pool != NULL) {
        mr_mempool_free(buf->pool, buf);
    }
}

uint8_t mr_zc_buffer_get_ref_count(mr_zc_buffer_t *buf)
{
    if (buf == NULL) {
        return 0;
    }
    return buf->ref_count;
}

void mr_zc_buffer_set_length(mr_zc_buffer_t *buf, uint16_t length)
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

void mr_zc_queue_init(mr_zc_queue_t *queue,
                         mr_zc_buffer_t **slots,
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
    mr_list_init(&queue->send_wait);
    mr_list_init(&queue->recv_wait);

    /* Clear slots */
    for (i = 0; i < capacity; i++) {
        slots[i] = NULL;
    }
}

mr_status_t mr_zc_queue_send(mr_zc_queue_t *queue,
                                  mr_zc_buffer_t *buf,
                                  uint32_t timeout)
{
    uint32_t state;
    uint16_t next_head;
    uint32_t start_tick;
    uint32_t elapsed;

    if (queue == NULL || buf == NULL) {
        return MR_ERR_PARAM;
    }

    start_tick = g_tick_count;

    while (1) {
        state = mr_port_disable_interrupts();

        /* Check if queue has space */
        next_head = (queue->head + 1) % queue->capacity;

        if (queue->count < queue->capacity) {
            /* Space available - insert buffer */
            queue->slots[queue->head] = buf;
            queue->head = next_head;
            queue->count++;

            /* Wake up any waiting receiver */
            (void)zc_unblock_waiter(&queue->recv_wait);

            mr_port_restore_interrupts(state);
            return MR_OK;
        }

        /* Queue full - check timeout */
        if (timeout == MR_NO_WAIT) {
            mr_port_restore_interrupts(state);
            return MR_ERR_TIMEOUT;
        }

        elapsed = g_tick_count - start_tick;
        if (timeout != MR_WAIT_FOREVER && elapsed >= timeout) {
            mr_port_restore_interrupts(state);
            return MR_ERR_TIMEOUT;
        }

        /* Block waiting for space */
        g_current_tcb->event_node.value = g_current_tcb->priority;
        mr_list_insert_priority(&queue->send_wait,
                                   &g_current_tcb->event_node);

        mr_port_restore_interrupts(state);

        /* Yield to let other tasks run */
        mr_port_yield();
    }
}

mr_status_t mr_zc_queue_send_front(mr_zc_queue_t *queue,
                                        mr_zc_buffer_t *buf,
                                        uint32_t timeout)
{
    uint32_t state;
    uint16_t new_tail;
    uint32_t start_tick;
    uint32_t elapsed;

    if (queue == NULL || buf == NULL) {
        return MR_ERR_PARAM;
    }

    start_tick = g_tick_count;

    while (1) {
        state = mr_port_disable_interrupts();

        if (queue->count < queue->capacity) {
            /* Insert at front (before tail) */
            new_tail = (queue->tail == 0) ?
                        queue->capacity - 1 : queue->tail - 1;

            queue->slots[new_tail] = buf;
            queue->tail = new_tail;
            queue->count++;

            /* Wake up any waiting receiver */
            (void)zc_unblock_waiter(&queue->recv_wait);

            mr_port_restore_interrupts(state);
            return MR_OK;
        }

        /* Queue full */
        if (timeout == MR_NO_WAIT) {
            mr_port_restore_interrupts(state);
            return MR_ERR_TIMEOUT;
        }

        elapsed = g_tick_count - start_tick;
        if (timeout != MR_WAIT_FOREVER && elapsed >= timeout) {
            mr_port_restore_interrupts(state);
            return MR_ERR_TIMEOUT;
        }

        g_current_tcb->event_node.value = g_current_tcb->priority;
        mr_list_insert_priority(&queue->send_wait,
                                   &g_current_tcb->event_node);

        mr_port_restore_interrupts(state);
        mr_port_yield();
    }
}

mr_status_t mr_zc_queue_send_from_isr(mr_zc_queue_t *queue,
                                           mr_zc_buffer_t *buf,
                                           bool *higher_prio_woken)
{
    uint16_t next_head;

    if (queue == NULL || buf == NULL) {
        return MR_ERR_PARAM;
    }

    if (higher_prio_woken != NULL) {
        *higher_prio_woken = false;
    }

    if (queue->count >= queue->capacity) {
        return MR_ERR_FULL;
    }

    /* Insert buffer */
    next_head = (queue->head + 1) % queue->capacity;
    queue->slots[queue->head] = buf;
    queue->head = next_head;
    queue->count++;

    /* Wake up waiting receiver */
    if (!mr_list_is_empty(&queue->recv_wait)) {
        mr_list_node_t *node = mr_list_peek_head(&queue->recv_wait);
        mr_tcb_t *waiter = (node != NULL) ? MR_TCB_FROM_EVENT_NODE(node) : NULL;
        (void)zc_unblock_waiter(&queue->recv_wait);

        if (higher_prio_woken != NULL && waiter != NULL && g_current_tcb != NULL) {
            *higher_prio_woken = (waiter->priority < g_current_tcb->priority);
        }
    }

    return MR_OK;
}

mr_zc_buffer_t *mr_zc_queue_receive(mr_zc_queue_t *queue,
                                         uint32_t timeout)
{
    uint32_t state;
    mr_zc_buffer_t *buf;
    uint32_t start_tick;
    uint32_t elapsed;

    if (queue == NULL) {
        return NULL;
    }

    start_tick = g_tick_count;

    while (1) {
        state = mr_port_disable_interrupts();

        if (queue->count > 0) {
            /* Get buffer from tail */
            buf = queue->slots[queue->tail];
            queue->slots[queue->tail] = NULL;
            queue->tail = (queue->tail + 1) % queue->capacity;
            queue->count--;

            /* Wake up any waiting sender */
            (void)zc_unblock_waiter(&queue->send_wait);

            mr_port_restore_interrupts(state);
            return buf;
        }

        /* Queue empty */
        if (timeout == MR_NO_WAIT) {
            mr_port_restore_interrupts(state);
            return NULL;
        }

        elapsed = g_tick_count - start_tick;
        if (timeout != MR_WAIT_FOREVER && elapsed >= timeout) {
            mr_port_restore_interrupts(state);
            return NULL;
        }

        /* Block waiting for data */
        g_current_tcb->event_node.value = g_current_tcb->priority;
        mr_list_insert_priority(&queue->recv_wait,
                                   &g_current_tcb->event_node);

        mr_port_restore_interrupts(state);
        mr_port_yield();
    }
}

mr_zc_buffer_t *mr_zc_queue_receive_from_isr(mr_zc_queue_t *queue,
                                                  bool *higher_prio_woken)
{
    mr_zc_buffer_t *buf;

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
    if (!mr_list_is_empty(&queue->send_wait)) {
        mr_list_node_t *node = mr_list_peek_head(&queue->send_wait);
        mr_tcb_t *waiter = (node != NULL) ? MR_TCB_FROM_EVENT_NODE(node) : NULL;
        (void)zc_unblock_waiter(&queue->send_wait);

        if (higher_prio_woken != NULL && waiter != NULL && g_current_tcb != NULL) {
            *higher_prio_woken = (waiter->priority < g_current_tcb->priority);
        }
    }

    return buf;
}

mr_zc_buffer_t *mr_zc_queue_peek(mr_zc_queue_t *queue)
{
    if (queue == NULL || queue->count == 0) {
        return NULL;
    }

    return queue->slots[queue->tail];
}

bool mr_zc_queue_is_empty(mr_zc_queue_t *queue)
{
    return (queue == NULL || queue->count == 0);
}

bool mr_zc_queue_is_full(mr_zc_queue_t *queue)
{
    return (queue != NULL && queue->count >= queue->capacity);
}

uint16_t mr_zc_queue_get_count(mr_zc_queue_t *queue)
{
    return (queue != NULL) ? queue->count : 0;
}

uint16_t mr_zc_queue_get_available(mr_zc_queue_t *queue)
{
    return (queue != NULL) ? (queue->capacity - queue->count) : 0;
}

void mr_zc_queue_flush(mr_zc_queue_t *queue)
{
    uint32_t state;
    mr_zc_buffer_t *buf;

    if (queue == NULL) {
        return;
    }

    state = mr_port_disable_interrupts();

    while (queue->count > 0) {
        buf = queue->slots[queue->tail];
        queue->slots[queue->tail] = NULL;
        queue->tail = (queue->tail + 1) % queue->capacity;
        queue->count--;

        /* Unreference each buffer */
        if (buf != NULL) {
            mr_zc_buffer_unref(buf);
        }
    }

    mr_port_restore_interrupts(state);
}

/*===========================================================================*/
/* Scatter-Gather Implementation                                              */
/*===========================================================================*/

void mr_zc_chain_init(mr_zc_chain_t *chain)
{
    if (chain == NULL) {
        return;
    }

    chain->head = NULL;
    chain->tail = NULL;
    chain->count = 0;
    chain->total_len = 0;
}

void mr_zc_chain_append(mr_zc_chain_t *chain, mr_zc_buffer_t *buf)
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

void mr_zc_chain_prepend(mr_zc_chain_t *chain, mr_zc_buffer_t *buf)
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

uint32_t mr_zc_chain_get_length(mr_zc_chain_t *chain)
{
    return (chain != NULL) ? chain->total_len : 0;
}

void mr_zc_chain_free(mr_zc_chain_t *chain)
{
    mr_zc_buffer_t *buf;
    mr_zc_buffer_t *next;

    if (chain == NULL) {
        return;
    }

    buf = chain->head;
    while (buf != NULL) {
        next = buf->next;
        mr_zc_buffer_unref(buf);
        buf = next;
    }

    chain->head = NULL;
    chain->tail = NULL;
    chain->count = 0;
    chain->total_len = 0;
}

#endif /* MR_USE_ZEROCOPY */
