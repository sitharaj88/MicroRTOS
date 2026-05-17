/**
 * MicroRTOS - Lock-Free Data Structures
 *
 * Lock-free queues for high-performance inter-task communication
 * without mutex overhead.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef MR_LOCKFREE_H
#define MR_LOCKFREE_H

#include "mr_config.h"
#include "mr_atomic.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* SPSC Queue (Single Producer, Single Consumer)                              */
/*===========================================================================*/

/**
 * Lock-free SPSC ring buffer.
 *
 * Safe when exactly ONE task writes and ONE task reads.
 * Zero contention, no CAS operations needed.
 */
typedef struct mr_spsc_queue {
    uint8_t *buffer;                /**< Data buffer */
    uint32_t capacity;              /**< Number of slots (power of 2) */
    uint32_t mask;                  /**< capacity - 1 for fast modulo */
    uint16_t item_size;             /**< Size of each item */
    volatile uint32_t head;         /**< Write index (producer only) */
    volatile uint32_t tail;         /**< Read index (consumer only) */
} mr_spsc_queue_t;

/**
 * Initialize an SPSC queue.
 *
 * @param queue     Pointer to queue structure
 * @param buffer    Pre-allocated buffer (size = item_size * capacity)
 * @param item_size Size of each item in bytes
 * @param capacity  Number of slots (MUST be power of 2)
 */
void mr_spsc_init(mr_spsc_queue_t *queue,
                    void *buffer,
                    uint16_t item_size,
                    uint32_t capacity);

/**
 * Push an item to the queue (producer only).
 *
 * @param queue Pointer to queue
 * @param item  Pointer to item to copy into queue
 *
 * @return true if successful, false if queue is full
 */
bool mr_spsc_push(mr_spsc_queue_t *queue, const void *item);

/**
 * Pop an item from the queue (consumer only).
 *
 * @param queue Pointer to queue
 * @param item  Pointer to buffer to copy item into
 *
 * @return true if successful, false if queue is empty
 */
bool mr_spsc_pop(mr_spsc_queue_t *queue, void *item);

/**
 * Peek at the front item without removing it (consumer only).
 *
 * @param queue Pointer to queue
 * @param item  Pointer to buffer to copy item into
 *
 * @return true if successful, false if queue is empty
 */
bool mr_spsc_peek(mr_spsc_queue_t *queue, void *item);

/**
 * Check if queue is empty.
 */
bool mr_spsc_is_empty(mr_spsc_queue_t *queue);

/**
 * Check if queue is full.
 */
bool mr_spsc_is_full(mr_spsc_queue_t *queue);

/**
 * Get current number of items in queue.
 */
uint32_t mr_spsc_count(mr_spsc_queue_t *queue);

/**
 * Get available space in queue.
 */
uint32_t mr_spsc_available(mr_spsc_queue_t *queue);

/*===========================================================================*/
/* MPSC Queue (Multiple Producer, Single Consumer)                            */
/*===========================================================================*/

/**
 * MPSC queue node (intrusive).
 * Embed this in your data structure.
 */
typedef struct mr_mpsc_node {
    struct mr_mpsc_node *volatile next;
} mr_mpsc_node_t;

/**
 * Lock-free MPSC queue using intrusive linked list.
 *
 * Safe when MULTIPLE tasks push and ONE task pops.
 * Uses CAS for thread-safe enqueue.
 */
typedef struct mr_mpsc_queue {
    mr_mpsc_node_t *volatile head;    /**< Consumer reads from here */
    mr_mpsc_node_t *volatile tail;    /**< Producers append here */
    mr_mpsc_node_t stub;              /**< Dummy node */
} mr_mpsc_queue_t;

/**
 * Initialize an MPSC queue.
 *
 * @param queue Pointer to queue structure
 */
void mr_mpsc_init(mr_mpsc_queue_t *queue);

/**
 * Push a node to the queue (multiple producers safe).
 *
 * @param queue Pointer to queue
 * @param node  Pointer to node to enqueue
 */
void mr_mpsc_push(mr_mpsc_queue_t *queue, mr_mpsc_node_t *node);

/**
 * Pop a node from the queue (single consumer only).
 *
 * @param queue Pointer to queue
 *
 * @return Pointer to dequeued node, or NULL if empty
 */
mr_mpsc_node_t *mr_mpsc_pop(mr_mpsc_queue_t *queue);

/**
 * Check if queue is empty.
 * Note: May return false positive due to race conditions.
 */
bool mr_mpsc_is_empty(mr_mpsc_queue_t *queue);

/*===========================================================================*/
/* MPMC Queue (Multiple Producer, Multiple Consumer)                          */
/*===========================================================================*/

/**
 * Lock-free MPMC bounded queue.
 *
 * Safe for ANY number of producers and consumers.
 * Uses CAS operations for both enqueue and dequeue.
 */
typedef struct mr_mpmc_queue {
    uint8_t *buffer;                /**< Data buffer */
    uint32_t *sequence;             /**< Sequence numbers for each slot */
    uint32_t capacity;              /**< Number of slots (power of 2) */
    uint32_t mask;                  /**< capacity - 1 */
    uint16_t item_size;             /**< Size of each item */
    volatile uint32_t head;         /**< Enqueue position */
    volatile uint32_t tail;         /**< Dequeue position */
} mr_mpmc_queue_t;

/**
 * Initialize an MPMC queue.
 *
 * @param queue     Pointer to queue structure
 * @param buffer    Pre-allocated data buffer
 * @param sequence  Pre-allocated sequence array (capacity * sizeof(uint32_t))
 * @param item_size Size of each item
 * @param capacity  Number of slots (MUST be power of 2)
 */
void mr_mpmc_init(mr_mpmc_queue_t *queue,
                    void *buffer,
                    uint32_t *sequence,
                    uint16_t item_size,
                    uint32_t capacity);

/**
 * Push an item to the queue.
 *
 * @param queue Pointer to queue
 * @param item  Pointer to item to copy
 *
 * @return true if successful, false if queue is full
 */
bool mr_mpmc_push(mr_mpmc_queue_t *queue, const void *item);

/**
 * Pop an item from the queue.
 *
 * @param queue Pointer to queue
 * @param item  Pointer to buffer to copy item into
 *
 * @return true if successful, false if queue is empty
 */
bool mr_mpmc_pop(mr_mpmc_queue_t *queue, void *item);

/*===========================================================================*/
/* Lock-Free Stack (LIFO)                                                     */
/*===========================================================================*/

/**
 * Lock-free stack node (intrusive).
 */
typedef struct mr_stack_node {
    struct mr_stack_node *volatile next;
} mr_stack_node_t;

/**
 * Lock-free stack using CAS.
 */
typedef struct mr_lockfree_stack {
    mr_stack_node_t *volatile top;
} mr_lockfree_stack_t;

/**
 * Initialize a lock-free stack.
 */
void mr_stack_init(mr_lockfree_stack_t *stack);

/**
 * Push a node onto the stack.
 */
void mr_stack_push(mr_lockfree_stack_t *stack, mr_stack_node_t *node);

/**
 * Pop a node from the stack.
 *
 * @return Popped node, or NULL if empty
 */
mr_stack_node_t *mr_stack_pop(mr_lockfree_stack_t *stack);

/**
 * Check if stack is empty.
 */
bool mr_stack_is_empty(mr_lockfree_stack_t *stack);

/*===========================================================================*/
/* Utility Macros                                                             */
/*===========================================================================*/

/**
 * Check if a value is a power of 2.
 */
#define MR_IS_POWER_OF_2(x) (((x) != 0) && (((x) & ((x) - 1)) == 0))

/**
 * Get container structure from embedded node.
 */
#define MR_CONTAINER_OF_NODE(ptr, type, member) \
    ((type *)((uint8_t *)(ptr) - offsetof(type, member)))

#ifdef __cplusplus
}
#endif

#endif /* MR_LOCKFREE_H */
