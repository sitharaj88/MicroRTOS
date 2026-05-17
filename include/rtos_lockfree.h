/**
 * MicroRTOS - Lock-Free Data Structures
 *
 * Lock-free queues for high-performance inter-task communication
 * without mutex overhead.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef RTOS_LOCKFREE_H
#define RTOS_LOCKFREE_H

#include "rtos_config.h"
#include "rtos_atomic.h"
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
typedef struct rtos_spsc_queue {
    uint8_t *buffer;                /**< Data buffer */
    uint32_t capacity;              /**< Number of slots (power of 2) */
    uint32_t mask;                  /**< capacity - 1 for fast modulo */
    uint16_t item_size;             /**< Size of each item */
    volatile uint32_t head;         /**< Write index (producer only) */
    volatile uint32_t tail;         /**< Read index (consumer only) */
} rtos_spsc_queue_t;

/**
 * Initialize an SPSC queue.
 *
 * @param queue     Pointer to queue structure
 * @param buffer    Pre-allocated buffer (size = item_size * capacity)
 * @param item_size Size of each item in bytes
 * @param capacity  Number of slots (MUST be power of 2)
 */
void rtos_spsc_init(rtos_spsc_queue_t *queue,
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
bool rtos_spsc_push(rtos_spsc_queue_t *queue, const void *item);

/**
 * Pop an item from the queue (consumer only).
 *
 * @param queue Pointer to queue
 * @param item  Pointer to buffer to copy item into
 *
 * @return true if successful, false if queue is empty
 */
bool rtos_spsc_pop(rtos_spsc_queue_t *queue, void *item);

/**
 * Peek at the front item without removing it (consumer only).
 *
 * @param queue Pointer to queue
 * @param item  Pointer to buffer to copy item into
 *
 * @return true if successful, false if queue is empty
 */
bool rtos_spsc_peek(rtos_spsc_queue_t *queue, void *item);

/**
 * Check if queue is empty.
 */
bool rtos_spsc_is_empty(rtos_spsc_queue_t *queue);

/**
 * Check if queue is full.
 */
bool rtos_spsc_is_full(rtos_spsc_queue_t *queue);

/**
 * Get current number of items in queue.
 */
uint32_t rtos_spsc_count(rtos_spsc_queue_t *queue);

/**
 * Get available space in queue.
 */
uint32_t rtos_spsc_available(rtos_spsc_queue_t *queue);

/*===========================================================================*/
/* MPSC Queue (Multiple Producer, Single Consumer)                            */
/*===========================================================================*/

/**
 * MPSC queue node (intrusive).
 * Embed this in your data structure.
 */
typedef struct rtos_mpsc_node {
    struct rtos_mpsc_node *volatile next;
} rtos_mpsc_node_t;

/**
 * Lock-free MPSC queue using intrusive linked list.
 *
 * Safe when MULTIPLE tasks push and ONE task pops.
 * Uses CAS for thread-safe enqueue.
 */
typedef struct rtos_mpsc_queue {
    rtos_mpsc_node_t *volatile head;    /**< Consumer reads from here */
    rtos_mpsc_node_t *volatile tail;    /**< Producers append here */
    rtos_mpsc_node_t stub;              /**< Dummy node */
} rtos_mpsc_queue_t;

/**
 * Initialize an MPSC queue.
 *
 * @param queue Pointer to queue structure
 */
void rtos_mpsc_init(rtos_mpsc_queue_t *queue);

/**
 * Push a node to the queue (multiple producers safe).
 *
 * @param queue Pointer to queue
 * @param node  Pointer to node to enqueue
 */
void rtos_mpsc_push(rtos_mpsc_queue_t *queue, rtos_mpsc_node_t *node);

/**
 * Pop a node from the queue (single consumer only).
 *
 * @param queue Pointer to queue
 *
 * @return Pointer to dequeued node, or NULL if empty
 */
rtos_mpsc_node_t *rtos_mpsc_pop(rtos_mpsc_queue_t *queue);

/**
 * Check if queue is empty.
 * Note: May return false positive due to race conditions.
 */
bool rtos_mpsc_is_empty(rtos_mpsc_queue_t *queue);

/*===========================================================================*/
/* MPMC Queue (Multiple Producer, Multiple Consumer)                          */
/*===========================================================================*/

/**
 * Lock-free MPMC bounded queue.
 *
 * Safe for ANY number of producers and consumers.
 * Uses CAS operations for both enqueue and dequeue.
 */
typedef struct rtos_mpmc_queue {
    uint8_t *buffer;                /**< Data buffer */
    uint32_t *sequence;             /**< Sequence numbers for each slot */
    uint32_t capacity;              /**< Number of slots (power of 2) */
    uint32_t mask;                  /**< capacity - 1 */
    uint16_t item_size;             /**< Size of each item */
    volatile uint32_t head;         /**< Enqueue position */
    volatile uint32_t tail;         /**< Dequeue position */
} rtos_mpmc_queue_t;

/**
 * Initialize an MPMC queue.
 *
 * @param queue     Pointer to queue structure
 * @param buffer    Pre-allocated data buffer
 * @param sequence  Pre-allocated sequence array (capacity * sizeof(uint32_t))
 * @param item_size Size of each item
 * @param capacity  Number of slots (MUST be power of 2)
 */
void rtos_mpmc_init(rtos_mpmc_queue_t *queue,
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
bool rtos_mpmc_push(rtos_mpmc_queue_t *queue, const void *item);

/**
 * Pop an item from the queue.
 *
 * @param queue Pointer to queue
 * @param item  Pointer to buffer to copy item into
 *
 * @return true if successful, false if queue is empty
 */
bool rtos_mpmc_pop(rtos_mpmc_queue_t *queue, void *item);

/*===========================================================================*/
/* Lock-Free Stack (LIFO)                                                     */
/*===========================================================================*/

/**
 * Lock-free stack node (intrusive).
 */
typedef struct rtos_stack_node {
    struct rtos_stack_node *volatile next;
} rtos_stack_node_t;

/**
 * Lock-free stack using CAS.
 */
typedef struct rtos_lockfree_stack {
    rtos_stack_node_t *volatile top;
} rtos_lockfree_stack_t;

/**
 * Initialize a lock-free stack.
 */
void rtos_stack_init(rtos_lockfree_stack_t *stack);

/**
 * Push a node onto the stack.
 */
void rtos_stack_push(rtos_lockfree_stack_t *stack, rtos_stack_node_t *node);

/**
 * Pop a node from the stack.
 *
 * @return Popped node, or NULL if empty
 */
rtos_stack_node_t *rtos_stack_pop(rtos_lockfree_stack_t *stack);

/**
 * Check if stack is empty.
 */
bool rtos_stack_is_empty(rtos_lockfree_stack_t *stack);

/*===========================================================================*/
/* Utility Macros                                                             */
/*===========================================================================*/

/**
 * Check if a value is a power of 2.
 */
#define RTOS_IS_POWER_OF_2(x) (((x) != 0) && (((x) & ((x) - 1)) == 0))

/**
 * Get container structure from embedded node.
 */
#define RTOS_CONTAINER_OF_NODE(ptr, type, member) \
    ((type *)((uint8_t *)(ptr) - offsetof(type, member)))

#ifdef __cplusplus
}
#endif

#endif /* RTOS_LOCKFREE_H */
