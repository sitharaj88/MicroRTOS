/**
 * MicroRTOS - Lock-Free Data Structures Implementation
 *
 * High-performance lock-free queues and stacks.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos_lockfree.h"
#include <string.h>

/*===========================================================================*/
/* SPSC Queue Implementation                                                  */
/*===========================================================================*/

void rtos_spsc_init(rtos_spsc_queue_t *queue,
                    void *buffer,
                    uint16_t item_size,
                    uint32_t capacity)
{
    RTOS_ASSERT(queue != NULL);
    RTOS_ASSERT(buffer != NULL);
    RTOS_ASSERT(item_size > 0);
    RTOS_ASSERT(RTOS_IS_POWER_OF_2(capacity));

    queue->buffer = (uint8_t *)buffer;
    queue->capacity = capacity;
    queue->mask = capacity - 1;
    queue->item_size = item_size;
    queue->head = 0;
    queue->tail = 0;
}

bool rtos_spsc_push(rtos_spsc_queue_t *queue, const void *item)
{
    uint32_t head;
    uint32_t tail;
    uint32_t next_head;

    /* Load head (written by us) - relaxed is fine */
    head = queue->head;

    /* Calculate next head position */
    next_head = (head + 1) & queue->mask;

    /* Load tail with acquire to see consumer's writes */
    tail = rtos_atomic_load(&queue->tail);

    /* Check if full */
    if (next_head == tail) {
        return false;
    }

    /* Copy item to buffer */
    memcpy(queue->buffer + (head * queue->item_size),
           item,
           queue->item_size);

    /* Store new head with release to make item visible to consumer */
    rtos_atomic_store(&queue->head, next_head);

    return true;
}

bool rtos_spsc_pop(rtos_spsc_queue_t *queue, void *item)
{
    uint32_t head;
    uint32_t tail;
    uint32_t next_tail;

    /* Load tail (written by us) */
    tail = queue->tail;

    /* Load head with acquire to see producer's writes */
    head = rtos_atomic_load(&queue->head);

    /* Check if empty */
    if (tail == head) {
        return false;
    }

    /* Copy item from buffer */
    memcpy(item,
           queue->buffer + (tail * queue->item_size),
           queue->item_size);

    /* Calculate next tail position */
    next_tail = (tail + 1) & queue->mask;

    /* Store new tail with release */
    rtos_atomic_store(&queue->tail, next_tail);

    return true;
}

bool rtos_spsc_peek(rtos_spsc_queue_t *queue, void *item)
{
    uint32_t head;
    uint32_t tail;

    tail = queue->tail;
    head = rtos_atomic_load(&queue->head);

    if (tail == head) {
        return false;
    }

    memcpy(item,
           queue->buffer + (tail * queue->item_size),
           queue->item_size);

    return true;
}

bool rtos_spsc_is_empty(rtos_spsc_queue_t *queue)
{
    uint32_t head = rtos_atomic_load(&queue->head);
    uint32_t tail = rtos_atomic_load(&queue->tail);
    return (head == tail);
}

bool rtos_spsc_is_full(rtos_spsc_queue_t *queue)
{
    uint32_t head = rtos_atomic_load(&queue->head);
    uint32_t tail = rtos_atomic_load(&queue->tail);
    uint32_t next_head = (head + 1) & queue->mask;
    return (next_head == tail);
}

uint32_t rtos_spsc_count(rtos_spsc_queue_t *queue)
{
    uint32_t head = rtos_atomic_load(&queue->head);
    uint32_t tail = rtos_atomic_load(&queue->tail);
    return (head - tail) & queue->mask;
}

uint32_t rtos_spsc_available(rtos_spsc_queue_t *queue)
{
    return queue->capacity - 1 - rtos_spsc_count(queue);
}

/*===========================================================================*/
/* MPSC Queue Implementation                                                  */
/*===========================================================================*/

void rtos_mpsc_init(rtos_mpsc_queue_t *queue)
{
    RTOS_ASSERT(queue != NULL);

    queue->stub.next = NULL;
    queue->head = &queue->stub;
    queue->tail = &queue->stub;
}

void rtos_mpsc_push(rtos_mpsc_queue_t *queue, rtos_mpsc_node_t *node)
{
    rtos_mpsc_node_t *prev;

    RTOS_ASSERT(queue != NULL);
    RTOS_ASSERT(node != NULL);

    /* Initialize node */
    node->next = NULL;

    /* Atomically swap tail and get previous tail */
    prev = (rtos_mpsc_node_t *)rtos_atomic_exchange(
        (volatile uint32_t *)&queue->tail,
        (uint32_t)node
    );

    /* Link previous tail to new node */
    /* This store is release to make node visible */
    rtos_atomic_store((volatile uint32_t *)&prev->next, (uint32_t)node);
}

rtos_mpsc_node_t *rtos_mpsc_pop(rtos_mpsc_queue_t *queue)
{
    rtos_mpsc_node_t *head;
    rtos_mpsc_node_t *next;

    RTOS_ASSERT(queue != NULL);

    head = queue->head;
    next = (rtos_mpsc_node_t *)rtos_atomic_load((volatile uint32_t *)&head->next);

    /* Skip stub node */
    if (head == &queue->stub) {
        if (next == NULL) {
            return NULL;  /* Empty */
        }
        queue->head = next;
        head = next;
        next = (rtos_mpsc_node_t *)rtos_atomic_load((volatile uint32_t *)&head->next);
    }

    /* Normal case - next exists */
    if (next != NULL) {
        queue->head = next;
        return head;
    }

    /* Check if this is the last node */
    rtos_mpsc_node_t *tail = queue->tail;
    if (head != tail) {
        /* Producer is in the middle of push, spin briefly */
        return NULL;
    }

    /* Re-insert stub to maintain invariant */
    rtos_mpsc_push(queue, &queue->stub);

    next = (rtos_mpsc_node_t *)rtos_atomic_load((volatile uint32_t *)&head->next);
    if (next != NULL) {
        queue->head = next;
        return head;
    }

    return NULL;
}

bool rtos_mpsc_is_empty(rtos_mpsc_queue_t *queue)
{
    rtos_mpsc_node_t *head = queue->head;
    rtos_mpsc_node_t *next = (rtos_mpsc_node_t *)rtos_atomic_load(
        (volatile uint32_t *)&head->next);

    if (head == &queue->stub && next == NULL) {
        return true;
    }
    return false;
}

/*===========================================================================*/
/* MPMC Queue Implementation                                                  */
/*===========================================================================*/

void rtos_mpmc_init(rtos_mpmc_queue_t *queue,
                    void *buffer,
                    uint32_t *sequence,
                    uint16_t item_size,
                    uint32_t capacity)
{
    uint32_t i;

    RTOS_ASSERT(queue != NULL);
    RTOS_ASSERT(buffer != NULL);
    RTOS_ASSERT(sequence != NULL);
    RTOS_ASSERT(RTOS_IS_POWER_OF_2(capacity));

    queue->buffer = (uint8_t *)buffer;
    queue->sequence = sequence;
    queue->capacity = capacity;
    queue->mask = capacity - 1;
    queue->item_size = item_size;
    queue->head = 0;
    queue->tail = 0;

    /* Initialize sequence numbers */
    for (i = 0; i < capacity; i++) {
        queue->sequence[i] = i;
    }
}

bool rtos_mpmc_push(rtos_mpmc_queue_t *queue, const void *item)
{
    uint32_t head;
    uint32_t seq;
    uint32_t pos;
    int32_t diff;

    while (1) {
        head = rtos_atomic_load(&queue->head);
        pos = head & queue->mask;
        seq = rtos_atomic_load(&queue->sequence[pos]);

        diff = (int32_t)(seq - head);

        if (diff == 0) {
            /* Slot is available for this position */
            if (rtos_atomic_compare_exchange_weak(
                    &queue->head, &head, head + 1)) {
                break;
            }
        } else if (diff < 0) {
            /* Queue is full */
            return false;
        }
        /* else: another producer took this slot, retry */
    }

    /* Copy data */
    memcpy(queue->buffer + (pos * queue->item_size), item, queue->item_size);

    /* Release the slot for consumers */
    rtos_atomic_store(&queue->sequence[pos], head + 1);

    return true;
}

bool rtos_mpmc_pop(rtos_mpmc_queue_t *queue, void *item)
{
    uint32_t tail;
    uint32_t seq;
    uint32_t pos;
    int32_t diff;

    while (1) {
        tail = rtos_atomic_load(&queue->tail);
        pos = tail & queue->mask;
        seq = rtos_atomic_load(&queue->sequence[pos]);

        diff = (int32_t)(seq - (tail + 1));

        if (diff == 0) {
            /* Item is available */
            if (rtos_atomic_compare_exchange_weak(
                    &queue->tail, &tail, tail + 1)) {
                break;
            }
        } else if (diff < 0) {
            /* Queue is empty */
            return false;
        }
        /* else: another consumer took this item, retry */
    }

    /* Copy data */
    memcpy(item, queue->buffer + (pos * queue->item_size), queue->item_size);

    /* Release the slot for producers */
    rtos_atomic_store(&queue->sequence[pos], tail + queue->capacity);

    return true;
}

/*===========================================================================*/
/* Lock-Free Stack Implementation                                             */
/*===========================================================================*/

void rtos_stack_init(rtos_lockfree_stack_t *stack)
{
    RTOS_ASSERT(stack != NULL);
    stack->top = NULL;
}

void rtos_stack_push(rtos_lockfree_stack_t *stack, rtos_stack_node_t *node)
{
    rtos_stack_node_t *old_top;

    RTOS_ASSERT(stack != NULL);
    RTOS_ASSERT(node != NULL);

    do {
        old_top = (rtos_stack_node_t *)rtos_atomic_load(
            (volatile uint32_t *)&stack->top);
        node->next = old_top;
    } while (!rtos_atomic_compare_exchange_weak(
        (volatile uint32_t *)&stack->top,
        (uint32_t *)&old_top,
        (uint32_t)node));
}

rtos_stack_node_t *rtos_stack_pop(rtos_lockfree_stack_t *stack)
{
    rtos_stack_node_t *old_top;
    rtos_stack_node_t *new_top;

    RTOS_ASSERT(stack != NULL);

    do {
        old_top = (rtos_stack_node_t *)rtos_atomic_load(
            (volatile uint32_t *)&stack->top);

        if (old_top == NULL) {
            return NULL;
        }

        new_top = (rtos_stack_node_t *)rtos_atomic_load(
            (volatile uint32_t *)&old_top->next);

    } while (!rtos_atomic_compare_exchange_weak(
        (volatile uint32_t *)&stack->top,
        (uint32_t *)&old_top,
        (uint32_t)new_top));

    return old_top;
}

bool rtos_stack_is_empty(rtos_lockfree_stack_t *stack)
{
    return (rtos_atomic_load((volatile uint32_t *)&stack->top) == 0);
}
