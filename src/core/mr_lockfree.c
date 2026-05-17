/**
 * MicroRTOS - Lock-Free Data Structures Implementation
 *
 * High-performance lock-free queues and stacks.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_lockfree.h"
#include "mr_types.h"
#include <stdint.h>
#include <string.h>

/*===========================================================================*/
/* SPSC Queue Implementation                                                  */
/*===========================================================================*/

void mr_spsc_init(mr_spsc_queue_t *queue,
                    void *buffer,
                    uint16_t item_size,
                    uint32_t capacity)
{
    MR_ASSERT(queue != NULL);
    MR_ASSERT(buffer != NULL);
    MR_ASSERT(item_size > 0);
    MR_ASSERT(MR_IS_POWER_OF_2(capacity));

    queue->buffer = (uint8_t *)buffer;
    queue->capacity = capacity;
    queue->mask = capacity - 1;
    queue->item_size = item_size;
    queue->head = 0;
    queue->tail = 0;
}

bool mr_spsc_push(mr_spsc_queue_t *queue, const void *item)
{
    uint32_t head;
    uint32_t tail;
    uint32_t next_head;

    /* Load head (written by us) - relaxed is fine */
    head = queue->head;

    /* Calculate next head position */
    next_head = (head + 1) & queue->mask;

    /* Load tail with acquire to see consumer's writes */
    tail = mr_atomic_load(&queue->tail);

    /* Check if full */
    if (next_head == tail) {
        return false;
    }

    /* Copy item to buffer */
    memcpy(queue->buffer + (head * queue->item_size),
           item,
           queue->item_size);

    /* Store new head with release to make item visible to consumer */
    mr_atomic_store(&queue->head, next_head);

    return true;
}

bool mr_spsc_pop(mr_spsc_queue_t *queue, void *item)
{
    uint32_t head;
    uint32_t tail;
    uint32_t next_tail;

    /* Load tail (written by us) */
    tail = queue->tail;

    /* Load head with acquire to see producer's writes */
    head = mr_atomic_load(&queue->head);

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
    mr_atomic_store(&queue->tail, next_tail);

    return true;
}

bool mr_spsc_peek(mr_spsc_queue_t *queue, void *item)
{
    uint32_t head;
    uint32_t tail;

    tail = queue->tail;
    head = mr_atomic_load(&queue->head);

    if (tail == head) {
        return false;
    }

    memcpy(item,
           queue->buffer + (tail * queue->item_size),
           queue->item_size);

    return true;
}

bool mr_spsc_is_empty(mr_spsc_queue_t *queue)
{
    uint32_t head = mr_atomic_load(&queue->head);
    uint32_t tail = mr_atomic_load(&queue->tail);
    return (head == tail);
}

bool mr_spsc_is_full(mr_spsc_queue_t *queue)
{
    uint32_t head = mr_atomic_load(&queue->head);
    uint32_t tail = mr_atomic_load(&queue->tail);
    uint32_t next_head = (head + 1) & queue->mask;
    return (next_head == tail);
}

uint32_t mr_spsc_count(mr_spsc_queue_t *queue)
{
    uint32_t head = mr_atomic_load(&queue->head);
    uint32_t tail = mr_atomic_load(&queue->tail);
    return (head - tail) & queue->mask;
}

uint32_t mr_spsc_available(mr_spsc_queue_t *queue)
{
    return queue->capacity - 1 - mr_spsc_count(queue);
}

/*===========================================================================*/
/* MPSC Queue Implementation                                                  */
/*===========================================================================*/

void mr_mpsc_init(mr_mpsc_queue_t *queue)
{
    MR_ASSERT(queue != NULL);

    queue->stub.next = NULL;
    queue->head = &queue->stub;
    queue->tail = &queue->stub;
}

void mr_mpsc_push(mr_mpsc_queue_t *queue, mr_mpsc_node_t *node)
{
    mr_mpsc_node_t *prev;

    MR_ASSERT(queue != NULL);
    MR_ASSERT(node != NULL);

    /* Initialize node */
    node->next = NULL;

    /* Atomically swap tail and get previous tail.
     * Pointers go through uintptr_t to make the size cast explicit; on AVR
     * the 16-bit pointer is zero-extended to fit the 32-bit atomic word. */
    prev = (mr_mpsc_node_t *)(uintptr_t)mr_atomic_exchange(
        (volatile uint32_t *)&queue->tail,
        (uint32_t)(uintptr_t)node
    );

    /* Link previous tail to new node */
    /* This store is release to make node visible */
    mr_atomic_store((volatile uint32_t *)&prev->next, (uint32_t)(uintptr_t)node);
}

mr_mpsc_node_t *mr_mpsc_pop(mr_mpsc_queue_t *queue)
{
    mr_mpsc_node_t *head;
    mr_mpsc_node_t *next;

    MR_ASSERT(queue != NULL);

    head = queue->head;
    next = (mr_mpsc_node_t *)(uintptr_t)mr_atomic_load((volatile uint32_t *)&head->next);

    /* Skip stub node */
    if (head == &queue->stub) {
        if (next == NULL) {
            return NULL;  /* Empty */
        }
        queue->head = next;
        head = next;
        next = (mr_mpsc_node_t *)(uintptr_t)mr_atomic_load((volatile uint32_t *)&head->next);
    }

    /* Normal case - next exists */
    if (next != NULL) {
        queue->head = next;
        return head;
    }

    /* Check if this is the last node */
    mr_mpsc_node_t *tail = queue->tail;
    if (head != tail) {
        /* Producer is in the middle of push, spin briefly */
        return NULL;
    }

    /* Re-insert stub to maintain invariant */
    mr_mpsc_push(queue, &queue->stub);

    next = (mr_mpsc_node_t *)(uintptr_t)mr_atomic_load((volatile uint32_t *)&head->next);
    if (next != NULL) {
        queue->head = next;
        return head;
    }

    return NULL;
}

bool mr_mpsc_is_empty(mr_mpsc_queue_t *queue)
{
    mr_mpsc_node_t *head = queue->head;
    mr_mpsc_node_t *next = (mr_mpsc_node_t *)(uintptr_t)mr_atomic_load(
        (volatile uint32_t *)&head->next);

    if (head == &queue->stub && next == NULL) {
        return true;
    }
    return false;
}

/*===========================================================================*/
/* MPMC Queue Implementation                                                  */
/*===========================================================================*/

void mr_mpmc_init(mr_mpmc_queue_t *queue,
                    void *buffer,
                    uint32_t *sequence,
                    uint16_t item_size,
                    uint32_t capacity)
{
    uint32_t i;

    MR_ASSERT(queue != NULL);
    MR_ASSERT(buffer != NULL);
    MR_ASSERT(sequence != NULL);
    MR_ASSERT(MR_IS_POWER_OF_2(capacity));

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

bool mr_mpmc_push(mr_mpmc_queue_t *queue, const void *item)
{
    uint32_t head;
    uint32_t seq;
    uint32_t pos;
    int32_t diff;

    while (1) {
        head = mr_atomic_load(&queue->head);
        pos = head & queue->mask;
        seq = mr_atomic_load(&queue->sequence[pos]);

        diff = (int32_t)(seq - head);

        if (diff == 0) {
            /* Slot is available for this position */
            if (mr_atomic_compare_exchange_weak(
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
    mr_atomic_store(&queue->sequence[pos], head + 1);

    return true;
}

bool mr_mpmc_pop(mr_mpmc_queue_t *queue, void *item)
{
    uint32_t tail;
    uint32_t seq;
    uint32_t pos;
    int32_t diff;

    while (1) {
        tail = mr_atomic_load(&queue->tail);
        pos = tail & queue->mask;
        seq = mr_atomic_load(&queue->sequence[pos]);

        diff = (int32_t)(seq - (tail + 1));

        if (diff == 0) {
            /* Item is available */
            if (mr_atomic_compare_exchange_weak(
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
    mr_atomic_store(&queue->sequence[pos], tail + queue->capacity);

    return true;
}

/*===========================================================================*/
/* Lock-Free Stack Implementation                                             */
/*===========================================================================*/

void mr_stack_init(mr_lockfree_stack_t *stack)
{
    MR_ASSERT(stack != NULL);
    stack->top = NULL;
}

void mr_stack_push(mr_lockfree_stack_t *stack, mr_stack_node_t *node)
{
    mr_stack_node_t *old_top;

    MR_ASSERT(stack != NULL);
    MR_ASSERT(node != NULL);

    do {
        old_top = (mr_stack_node_t *)(uintptr_t)mr_atomic_load(
            (volatile uint32_t *)&stack->top);
        node->next = old_top;
    } while (!mr_atomic_compare_exchange_weak(
        (volatile uint32_t *)&stack->top,
        (uint32_t *)&old_top,
        (uint32_t)(uintptr_t)node));
}

mr_stack_node_t *mr_stack_pop(mr_lockfree_stack_t *stack)
{
    mr_stack_node_t *old_top;
    mr_stack_node_t *new_top;

    MR_ASSERT(stack != NULL);

    do {
        old_top = (mr_stack_node_t *)(uintptr_t)mr_atomic_load(
            (volatile uint32_t *)&stack->top);

        if (old_top == NULL) {
            return NULL;
        }

        new_top = (mr_stack_node_t *)(uintptr_t)mr_atomic_load(
            (volatile uint32_t *)&old_top->next);

    } while (!mr_atomic_compare_exchange_weak(
        (volatile uint32_t *)&stack->top,
        (uint32_t *)&old_top,
        (uint32_t)(uintptr_t)new_top));

    return old_top;
}

bool mr_stack_is_empty(mr_lockfree_stack_t *stack)
{
    return (mr_atomic_load((volatile uint32_t *)&stack->top) == 0);
}
