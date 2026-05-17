/**
 * MicroRTOS - Zero-Copy Messaging
 *
 * High-performance inter-task communication without data copying.
 * Uses reference-counted buffers with ownership transfer.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef MR_ZEROCOPY_H
#define MR_ZEROCOPY_H

#include "mr_config.h"
#include "mr_types.h"
#include "mr_memory.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Configuration                                                              */
/*===========================================================================*/

#ifndef MR_USE_ZEROCOPY
#define MR_USE_ZEROCOPY               0
#endif

#ifndef MR_ZEROCOPY_MAX_QUEUES
#define MR_ZEROCOPY_MAX_QUEUES        4
#endif

/*===========================================================================*/
/* Buffer Structure                                                           */
/*===========================================================================*/

/**
 * Zero-copy buffer descriptor.
 * Contains metadata and pointer to actual data.
 */
typedef struct mr_zc_buffer {
    void            *data;          /* Pointer to buffer data */
    uint16_t        size;           /* Size of data area */
    uint16_t        length;         /* Actual data length used */
    volatile uint8_t ref_count;     /* Reference count */
    uint8_t         flags;          /* Buffer flags */
    mr_mempool_t  *pool;          /* Origin pool for deallocation */
    struct mr_zc_buffer *next;    /* For internal linked lists */
} mr_zc_buffer_t;

/* Buffer flags */
#define MR_ZC_FLAG_NONE       0x00
#define MR_ZC_FLAG_STATIC     0x01    /* Static buffer (don't free) */
#define MR_ZC_FLAG_URGENT     0x02    /* Urgent/high-priority */
#define MR_ZC_FLAG_LAST       0x04    /* Last fragment in sequence */

/*===========================================================================*/
/* Zero-Copy Queue                                                            */
/*===========================================================================*/

/**
 * Zero-copy message queue.
 * Transfers buffer ownership between tasks.
 */
typedef struct mr_zc_queue {
    mr_zc_buffer_t **slots;       /* Array of buffer pointers */
    uint16_t        capacity;       /* Queue capacity */
    volatile uint16_t head;         /* Producer index */
    volatile uint16_t tail;         /* Consumer index */
    volatile uint16_t count;        /* Number of pending buffers */
    mr_list_t     send_wait;      /* Tasks waiting to send */
    mr_list_t     recv_wait;      /* Tasks waiting to receive */
} mr_zc_queue_t;

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
 * @return              MR_OK on success
 */
mr_status_t mr_zc_pool_init(mr_mempool_t *pool,
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
mr_zc_buffer_t *mr_zc_buffer_alloc(mr_mempool_t *pool,
                                        uint16_t size,
                                        uint32_t timeout);

/**
 * Allocate a buffer immediately (no blocking).
 *
 * @param pool          Pool to allocate from
 * @param size          Minimum required size
 * @return              Buffer pointer, or NULL if unavailable
 */
mr_zc_buffer_t *mr_zc_buffer_alloc_nowait(mr_mempool_t *pool,
                                               uint16_t size);

/**
 * Increment buffer reference count.
 * Use when sharing a buffer between multiple owners.
 *
 * @param buf           Buffer to reference
 */
void mr_zc_buffer_ref(mr_zc_buffer_t *buf);

/**
 * Decrement buffer reference count.
 * Buffer is freed when count reaches zero.
 *
 * @param buf           Buffer to unreference
 */
void mr_zc_buffer_unref(mr_zc_buffer_t *buf);

/**
 * Free a buffer immediately (sets ref count to 0).
 *
 * @param buf           Buffer to free
 */
void mr_zc_buffer_free(mr_zc_buffer_t *buf);

/**
 * Get current reference count.
 *
 * @param buf           Buffer to query
 * @return              Current reference count
 */
uint8_t mr_zc_buffer_get_ref_count(mr_zc_buffer_t *buf);

/**
 * Set buffer data length.
 *
 * @param buf           Buffer to modify
 * @param length        Data length (must be <= size)
 */
void mr_zc_buffer_set_length(mr_zc_buffer_t *buf, uint16_t length);

/**
 * Get pointer to buffer data.
 *
 * @param buf           Buffer
 * @return              Pointer to data area
 */
static inline void *mr_zc_buffer_get_data(mr_zc_buffer_t *buf)
{
    return (buf != NULL) ? buf->data : NULL;
}

/**
 * Get buffer data length.
 *
 * @param buf           Buffer
 * @return              Data length
 */
static inline uint16_t mr_zc_buffer_get_length(mr_zc_buffer_t *buf)
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
void mr_zc_queue_init(mr_zc_queue_t *queue,
                         mr_zc_buffer_t **slots,
                         uint16_t capacity);

/**
 * Send a buffer to the queue (ownership transfer).
 * The buffer is passed to the receiver without copying.
 *
 * @param queue         Target queue
 * @param buf           Buffer to send (caller loses ownership)
 * @param timeout       Timeout in ticks
 * @return              MR_OK on success
 */
mr_status_t mr_zc_queue_send(mr_zc_queue_t *queue,
                                  mr_zc_buffer_t *buf,
                                  uint32_t timeout);

/**
 * Send a buffer to front of queue (high priority).
 *
 * @param queue         Target queue
 * @param buf           Buffer to send
 * @param timeout       Timeout in ticks
 * @return              MR_OK on success
 */
mr_status_t mr_zc_queue_send_front(mr_zc_queue_t *queue,
                                        mr_zc_buffer_t *buf,
                                        uint32_t timeout);

/**
 * Send from ISR context (non-blocking).
 *
 * @param queue             Target queue
 * @param buf               Buffer to send
 * @param higher_prio_woken Set to true if a higher priority task was woken
 * @return                  MR_OK on success
 */
mr_status_t mr_zc_queue_send_from_isr(mr_zc_queue_t *queue,
                                           mr_zc_buffer_t *buf,
                                           bool *higher_prio_woken);

/**
 * Receive a buffer from the queue.
 * Caller gains ownership of the returned buffer.
 *
 * @param queue         Source queue
 * @param timeout       Timeout in ticks
 * @return              Buffer pointer, or NULL on timeout
 */
mr_zc_buffer_t *mr_zc_queue_receive(mr_zc_queue_t *queue,
                                         uint32_t timeout);

/**
 * Receive from ISR context (non-blocking).
 *
 * @param queue             Source queue
 * @param higher_prio_woken Set to true if a higher priority task was woken
 * @return                  Buffer pointer, or NULL if empty
 */
mr_zc_buffer_t *mr_zc_queue_receive_from_isr(mr_zc_queue_t *queue,
                                                  bool *higher_prio_woken);

/**
 * Peek at the next buffer without removing it.
 *
 * @param queue         Source queue
 * @return              Buffer pointer, or NULL if empty
 */
mr_zc_buffer_t *mr_zc_queue_peek(mr_zc_queue_t *queue);

/**
 * Check if queue is empty.
 *
 * @param queue         Queue to check
 * @return              true if empty
 */
bool mr_zc_queue_is_empty(mr_zc_queue_t *queue);

/**
 * Check if queue is full.
 *
 * @param queue         Queue to check
 * @return              true if full
 */
bool mr_zc_queue_is_full(mr_zc_queue_t *queue);

/**
 * Get number of buffers in queue.
 *
 * @param queue         Queue to check
 * @return              Number of pending buffers
 */
uint16_t mr_zc_queue_get_count(mr_zc_queue_t *queue);

/**
 * Get available space in queue.
 *
 * @param queue         Queue to check
 * @return              Number of free slots
 */
uint16_t mr_zc_queue_get_available(mr_zc_queue_t *queue);

/**
 * Flush all buffers from queue.
 * All pending buffers are unreferenced (freed if ref count reaches 0).
 *
 * @param queue         Queue to flush
 */
void mr_zc_queue_flush(mr_zc_queue_t *queue);

/*===========================================================================*/
/* Scatter-Gather Support                                                     */
/*===========================================================================*/

/**
 * Buffer chain for scatter-gather operations.
 */
typedef struct mr_zc_chain {
    mr_zc_buffer_t    *head;      /* First buffer in chain */
    mr_zc_buffer_t    *tail;      /* Last buffer in chain */
    uint16_t            count;      /* Number of buffers */
    uint32_t            total_len;  /* Total data length */
} mr_zc_chain_t;

/**
 * Initialize a buffer chain.
 *
 * @param chain         Chain to initialize
 */
void mr_zc_chain_init(mr_zc_chain_t *chain);

/**
 * Append a buffer to the chain.
 *
 * @param chain         Chain to append to
 * @param buf           Buffer to append
 */
void mr_zc_chain_append(mr_zc_chain_t *chain, mr_zc_buffer_t *buf);

/**
 * Prepend a buffer to the chain.
 *
 * @param chain         Chain to prepend to
 * @param buf           Buffer to prepend
 */
void mr_zc_chain_prepend(mr_zc_chain_t *chain, mr_zc_buffer_t *buf);

/**
 * Get total data length in chain.
 *
 * @param chain         Chain to measure
 * @return              Total bytes of data
 */
uint32_t mr_zc_chain_get_length(mr_zc_chain_t *chain);

/**
 * Free all buffers in chain.
 *
 * @param chain         Chain to free
 */
void mr_zc_chain_free(mr_zc_chain_t *chain);

/*===========================================================================*/
/* Convenience Macros                                                         */
/*===========================================================================*/

/**
 * Define a zero-copy queue with static storage.
 */
#define MR_ZC_QUEUE_DEFINE(name, capacity) \
    static mr_zc_buffer_t *name##_slots[capacity]; \
    static mr_zc_queue_t name##_queue

/**
 * Initialize a statically defined queue.
 */
#define MR_ZC_QUEUE_INIT(name, capacity) \
    mr_zc_queue_init(&name##_queue, name##_slots, capacity)

#ifdef __cplusplus
}
#endif

#endif /* MR_ZEROCOPY_H */
