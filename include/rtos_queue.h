/**
 * MicroRTOS - Message Queue API
 *
 * Thread-safe message queues for inter-task communication.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef RTOS_QUEUE_H
#define RTOS_QUEUE_H

#include "rtos_types.h"

#if RTOS_USE_QUEUES

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Queue API                                                                  */
/*===========================================================================*/

/**
 * Initialize a message queue.
 *
 * @param queue     Pointer to queue structure
 * @param buffer    Pre-allocated buffer for messages
 * @param item_size Size of each message in bytes
 * @param capacity  Maximum number of messages the queue can hold
 */
void rtos_queue_init(
    rtos_queue_t *queue,
    void *buffer,
    uint16_t item_size,
    uint16_t capacity
);

/**
 * Send a message to the back of a queue.
 *
 * @param queue   Pointer to queue
 * @param item    Pointer to message to copy into queue
 * @param timeout Maximum ticks to wait if queue is full
 *
 * @return RTOS_OK if sent, RTOS_ERR_TIMEOUT if timeout expired
 */
rtos_status_t rtos_queue_send(
    rtos_queue_t *queue,
    const void *item,
    uint32_t timeout
);

/**
 * Send a message to the front of a queue.
 *
 * Useful for urgent messages that should be processed first.
 *
 * @param queue   Pointer to queue
 * @param item    Pointer to message to copy into queue
 * @param timeout Maximum ticks to wait if queue is full
 *
 * @return RTOS_OK if sent, RTOS_ERR_TIMEOUT if timeout expired
 */
rtos_status_t rtos_queue_send_front(
    rtos_queue_t *queue,
    const void *item,
    uint32_t timeout
);

/**
 * Receive a message from a queue.
 *
 * @param queue   Pointer to queue
 * @param item    Pointer to buffer to copy message into
 * @param timeout Maximum ticks to wait if queue is empty
 *
 * @return RTOS_OK if received, RTOS_ERR_TIMEOUT if timeout expired
 */
rtos_status_t rtos_queue_receive(
    rtos_queue_t *queue,
    void *item,
    uint32_t timeout
);

/**
 * Peek at the front message without removing it.
 *
 * @param queue   Pointer to queue
 * @param item    Pointer to buffer to copy message into
 * @param timeout Maximum ticks to wait if queue is empty
 *
 * @return RTOS_OK if peeked, RTOS_ERR_TIMEOUT if timeout expired
 */
rtos_status_t rtos_queue_peek(
    rtos_queue_t *queue,
    void *item,
    uint32_t timeout
);

/**
 * Send a message from an ISR.
 *
 * @param queue          Pointer to queue
 * @param item           Pointer to message to copy
 * @param yield_required Set to true if a context switch should occur
 *
 * @return RTOS_OK if sent, RTOS_ERR_FULL if queue is full
 */
rtos_status_t rtos_queue_send_from_isr(
    rtos_queue_t *queue,
    const void *item,
    bool *yield_required
);

/**
 * Receive a message from an ISR.
 *
 * @param queue          Pointer to queue
 * @param item           Pointer to buffer to copy message into
 * @param yield_required Set to true if a context switch should occur
 *
 * @return RTOS_OK if received, RTOS_ERR_EMPTY if queue is empty
 */
rtos_status_t rtos_queue_receive_from_isr(
    rtos_queue_t *queue,
    void *item,
    bool *yield_required
);

/**
 * Get the current number of messages in a queue.
 *
 * @param queue Pointer to queue
 *
 * @return Number of messages
 */
uint16_t rtos_queue_count(rtos_queue_t *queue);

/**
 * Get the number of free spaces in a queue.
 *
 * @param queue Pointer to queue
 *
 * @return Number of free spaces
 */
uint16_t rtos_queue_spaces_available(rtos_queue_t *queue);

/**
 * Check if a queue is empty.
 *
 * @param queue Pointer to queue
 *
 * @return true if empty, false otherwise
 */
bool rtos_queue_is_empty(rtos_queue_t *queue);

/**
 * Check if a queue is full.
 *
 * @param queue Pointer to queue
 *
 * @return true if full, false otherwise
 */
bool rtos_queue_is_full(rtos_queue_t *queue);

/**
 * Reset a queue to empty state.
 *
 * @param queue Pointer to queue
 */
void rtos_queue_reset(rtos_queue_t *queue);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_USE_QUEUES */

#endif /* RTOS_QUEUE_H */
