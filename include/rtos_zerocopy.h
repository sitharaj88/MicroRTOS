/**
 * MicroRTOS - Zero-Copy Messaging
 *
 * High-performance inter-task communication without data copying.
 * Uses reference-counted buffers with ownership transfer.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef RTOS_ZEROCOPY_H
#define RTOS_ZEROCOPY_H

#include "rtos_config.h"
#include "rtos_types.h"
#include "rtos_memory.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Configuration                                                              */
/*===========================================================================*/

#ifndef RTOS_USE_ZEROCOPY
#define RTOS_USE_ZEROCOPY               0
#endif

#ifndef RTOS_ZEROCOPY_MAX_QUEUES
#define RTOS_ZEROCOPY_MAX_QUEUES        4
#endif

/*===========================================================================*/
/* Buffer Structure                                                           */
/*===========================================================================*/

/**
 * Zero-copy buffer descriptor.
 * Contains metadata and pointer to actual data.
 */
typedef struct rtos_zc_buffer {
    void            *data;          /* Pointer to buffer data */
    uint16_t        size;           /* Size of data area */
    uint16_t        length;         /* Actual data length used */
    volatile uint8_t ref_count;     /* Reference count */
    uint8_t         flags;          /* Buffer flags */
    rtos_mempool_t  *pool;          /* Origin pool for deallocation */
    struct rtos_zc_buffer *next;    /* For internal linked lists */
} rtos_zc_buffer_t;

/* Buffer flags */
#define RTOS_ZC_FLAG_NONE       0x00
#define RTOS_ZC_FLAG_STATIC     0x01    /* Static buffer (don't free) */
#define RTOS_ZC_FLAG_URGENT     0x02    /* Urgent/high-priority */
#define RTOS_ZC_FLAG_LAST       0x04    /* Last fragment in sequence */

/*===========================================================================*/
/* Zero-Copy Queue                                                            */
/*===========================================================================*/

/**
 * Zero-copy message queue.
 * Transfers buffer ownership between tasks.
 */
typedef struct rtos_zc_queue {
    rtos_zc_buffer_t **slots;       /* Array of buffer pointers */
    uint16_t        capacity;       /* Queue capacity */
    volatile uint16_t head;         /* Producer index */
    volatile uint16_t tail;         /* Consumer index */
    volatile uint16_t count;        /* Number of pending buffers */
    rtos_list_t     send_wait;      /* Tasks waiting to send */
    rtos_list_t     recv_wait;      /* Tasks waiting to receive */
} rtos_zc_queue_t;

/*===========================================================================*/
/* Buffer Pool Integration                                                    */
/*===========================================================================*/

/**
 * Create a zero-copy buffer pool.
 * Allocates buffers and descriptors from the provided memory.
 *
 * @param pool          Memory pool to use
 * @param buffer_size   Size of each buffer's data area
 * @param buffer_count  Number of buffers
 * @param memory        Memory for pool structures
 * @param memory_size   Size of provided memory
 * @return              RTOS_OK on success
 */
rtos_status_t rtos_zc_pool_init(rtos_mempool_t *pool,
                                 uint16_t buffer_size,
                                 uint16_t buffer_count,
                                 void *memory,
                                 uint32_t memory_size);

/*===========================================================================*/
/* Buffer Management API                                                      */
/*===========================================================================*/

/**
 * Allocate a zero-copy buffer from a pool.
 *
 * @param pool          Pool to allocate from
 * @param size          Minimum required size
 * @param timeout       Timeout in ticks (0 = no wait)
 * @return              Buffer pointer, or NULL if unavailable
 */
rtos_zc_buffer_t *rtos_zc_buffer_alloc(rtos_mempool_t *pool,
                                        uint16_t size,
                                        uint32_t timeout);

/**
 * Allocate a buffer immediately (no blocking).
 *
 * @param pool          Pool to allocate from
 * @param size          Minimum required size
 * @return              Buffer pointer, or NULL if unavailable
 */
rtos_zc_buffer_t *rtos_zc_buffer_alloc_nowait(rtos_mempool_t *pool,
                                               uint16_t size);

/**
 * Increment buffer reference count.
 * Use when sharing a buffer between multiple owners.
 *
 * @param buf           Buffer to reference
 */
void rtos_zc_buffer_ref(rtos_zc_buffer_t *buf);

/**
 * Decrement buffer reference count.
 * Buffer is freed when count reaches zero.
 *
 * @param buf           Buffer to unreference
 */
void rtos_zc_buffer_unref(rtos_zc_buffer_t *buf);

/**
 * Free a buffer immediately (sets ref count to 0).
 *
 * @param buf           Buffer to free
 */
void rtos_zc_buffer_free(rtos_zc_buffer_t *buf);

/**
 * Get current reference count.
 *
 * @param buf           Buffer to query
 * @return              Current reference count
 */
uint8_t rtos_zc_buffer_get_ref_count(rtos_zc_buffer_t *buf);

/**
 * Set buffer data length.
 *
 * @param buf           Buffer to modify
 * @param length        Data length (must be <= size)
 */
void rtos_zc_buffer_set_length(rtos_zc_buffer_t *buf, uint16_t length);

/**
 * Get pointer to buffer data.
 *
 * @param buf           Buffer
 * @return              Pointer to data area
 */
static inline void *rtos_zc_buffer_get_data(rtos_zc_buffer_t *buf)
{
    return (buf != NULL) ? buf->data : NULL;
}

/**
 * Get buffer data length.
 *
 * @param buf           Buffer
 * @return              Data length
 */
static inline uint16_t rtos_zc_buffer_get_length(rtos_zc_buffer_t *buf)
{
    return (buf != NULL) ? buf->length : 0;
}

/*===========================================================================*/
/* Zero-Copy Queue API                                                        */
/*===========================================================================*/

/**
 * Initialize a zero-copy queue.
 *
 * @param queue         Queue to initialize
 * @param slots         Array of buffer pointer slots
 * @param capacity      Number of slots (must be power of 2)
 */
void rtos_zc_queue_init(rtos_zc_queue_t *queue,
                         rtos_zc_buffer_t **slots,
                         uint16_t capacity);

/**
 * Send a buffer to the queue (ownership transfer).
 * The buffer is passed to the receiver without copying.
 *
 * @param queue         Target queue
 * @param buf           Buffer to send (caller loses ownership)
 * @param timeout       Timeout in ticks
 * @return              RTOS_OK on success
 */
rtos_status_t rtos_zc_queue_send(rtos_zc_queue_t *queue,
                                  rtos_zc_buffer_t *buf,
                                  uint32_t timeout);

/**
 * Send a buffer to front of queue (high priority).
 *
 * @param queue         Target queue
 * @param buf           Buffer to send
 * @param timeout       Timeout in ticks
 * @return              RTOS_OK on success
 */
rtos_status_t rtos_zc_queue_send_front(rtos_zc_queue_t *queue,
                                        rtos_zc_buffer_t *buf,
                                        uint32_t timeout);

/**
 * Send from ISR context (non-blocking).
 *
 * @param queue             Target queue
 * @param buf               Buffer to send
 * @param higher_prio_woken Set to true if a higher priority task was woken
 * @return                  RTOS_OK on success
 */
rtos_status_t rtos_zc_queue_send_from_isr(rtos_zc_queue_t *queue,
                                           rtos_zc_buffer_t *buf,
                                           bool *higher_prio_woken);

/**
 * Receive a buffer from the queue.
 * Caller gains ownership of the returned buffer.
 *
 * @param queue         Source queue
 * @param timeout       Timeout in ticks
 * @return              Buffer pointer, or NULL on timeout
 */
rtos_zc_buffer_t *rtos_zc_queue_receive(rtos_zc_queue_t *queue,
                                         uint32_t timeout);

/**
 * Receive from ISR context (non-blocking).
 *
 * @param queue             Source queue
 * @param higher_prio_woken Set to true if a higher priority task was woken
 * @return                  Buffer pointer, or NULL if empty
 */
rtos_zc_buffer_t *rtos_zc_queue_receive_from_isr(rtos_zc_queue_t *queue,
                                                  bool *higher_prio_woken);

/**
 * Peek at the next buffer without removing it.
 *
 * @param queue         Source queue
 * @return              Buffer pointer, or NULL if empty
 */
rtos_zc_buffer_t *rtos_zc_queue_peek(rtos_zc_queue_t *queue);

/**
 * Check if queue is empty.
 *
 * @param queue         Queue to check
 * @return              true if empty
 */
bool rtos_zc_queue_is_empty(rtos_zc_queue_t *queue);

/**
 * Check if queue is full.
 *
 * @param queue         Queue to check
 * @return              true if full
 */
bool rtos_zc_queue_is_full(rtos_zc_queue_t *queue);

/**
 * Get number of buffers in queue.
 *
 * @param queue         Queue to check
 * @return              Number of pending buffers
 */
uint16_t rtos_zc_queue_get_count(rtos_zc_queue_t *queue);

/**
 * Get available space in queue.
 *
 * @param queue         Queue to check
 * @return              Number of free slots
 */
uint16_t rtos_zc_queue_get_available(rtos_zc_queue_t *queue);

/**
 * Flush all buffers from queue.
 * All pending buffers are unreferenced (freed if ref count reaches 0).
 *
 * @param queue         Queue to flush
 */
void rtos_zc_queue_flush(rtos_zc_queue_t *queue);

/*===========================================================================*/
/* Scatter-Gather Support                                                     */
/*===========================================================================*/

/**
 * Buffer chain for scatter-gather operations.
 */
typedef struct rtos_zc_chain {
    rtos_zc_buffer_t    *head;      /* First buffer in chain */
    rtos_zc_buffer_t    *tail;      /* Last buffer in chain */
    uint16_t            count;      /* Number of buffers */
    uint32_t            total_len;  /* Total data length */
} rtos_zc_chain_t;

/**
 * Initialize a buffer chain.
 *
 * @param chain         Chain to initialize
 */
void rtos_zc_chain_init(rtos_zc_chain_t *chain);

/**
 * Append a buffer to the chain.
 *
 * @param chain         Chain to append to
 * @param buf           Buffer to append
 */
void rtos_zc_chain_append(rtos_zc_chain_t *chain, rtos_zc_buffer_t *buf);

/**
 * Prepend a buffer to the chain.
 *
 * @param chain         Chain to prepend to
 * @param buf           Buffer to prepend
 */
void rtos_zc_chain_prepend(rtos_zc_chain_t *chain, rtos_zc_buffer_t *buf);

/**
 * Get total data length in chain.
 *
 * @param chain         Chain to measure
 * @return              Total bytes of data
 */
uint32_t rtos_zc_chain_get_length(rtos_zc_chain_t *chain);

/**
 * Free all buffers in chain.
 *
 * @param chain         Chain to free
 */
void rtos_zc_chain_free(rtos_zc_chain_t *chain);

/*===========================================================================*/
/* Convenience Macros                                                         */
/*===========================================================================*/

/**
 * Define a zero-copy queue with static storage.
 */
#define RTOS_ZC_QUEUE_DEFINE(name, capacity) \
    static rtos_zc_buffer_t *name##_slots[capacity]; \
    static rtos_zc_queue_t name##_queue

/**
 * Initialize a statically defined queue.
 */
#define RTOS_ZC_QUEUE_INIT(name, capacity) \
    rtos_zc_queue_init(&name##_queue, name##_slots, capacity)

#ifdef __cplusplus
}
#endif

#endif /* RTOS_ZEROCOPY_H */
