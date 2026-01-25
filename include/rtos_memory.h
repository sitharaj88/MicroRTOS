/**
 * MicroRTOS - Memory Pool API
 *
 * Fixed-size block memory pools for deterministic allocation.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef RTOS_MEMORY_H
#define RTOS_MEMORY_H

#include "rtos_types.h"

#if RTOS_USE_MEMORY_POOLS

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Memory Pool API                                                            */
/*===========================================================================*/

/**
 * Initialize a memory pool.
 *
 * The buffer must be large enough to hold (block_size * block_count) bytes,
 * plus alignment padding if needed.
 *
 * @param pool        Pointer to pool structure
 * @param buffer      Pre-allocated buffer for blocks
 * @param block_size  Size of each block in bytes
 * @param block_count Total number of blocks
 */
void rtos_mempool_init(
    rtos_mempool_t *pool,
    void *buffer,
    uint16_t block_size,
    uint16_t block_count
);

/**
 * Allocate a block from a memory pool.
 *
 * @param pool    Pointer to pool
 * @param timeout Maximum ticks to wait if pool is empty
 *
 * @return Pointer to allocated block, or NULL on timeout
 */
void *rtos_mempool_alloc(rtos_mempool_t *pool, uint32_t timeout);

/**
 * Free a block back to a memory pool.
 *
 * @param pool  Pointer to pool
 * @param block Pointer to block to free
 *
 * @return RTOS_OK on success, RTOS_ERR_PARAM if block is invalid
 */
rtos_status_t rtos_mempool_free(rtos_mempool_t *pool, void *block);

/**
 * Get the number of free blocks in a pool.
 *
 * @param pool Pointer to pool
 *
 * @return Number of free blocks
 */
uint16_t rtos_mempool_available(rtos_mempool_t *pool);

/**
 * Get the total number of blocks in a pool.
 *
 * @param pool Pointer to pool
 *
 * @return Total number of blocks
 */
uint16_t rtos_mempool_total(rtos_mempool_t *pool);

/**
 * Get the block size of a pool.
 *
 * @param pool Pointer to pool
 *
 * @return Block size in bytes
 */
uint16_t rtos_mempool_block_size(rtos_mempool_t *pool);

/**
 * Check if a pool is empty.
 *
 * @param pool Pointer to pool
 *
 * @return true if no blocks available, false otherwise
 */
bool rtos_mempool_is_empty(rtos_mempool_t *pool);

/**
 * Allocate a block from ISR context (non-blocking).
 *
 * @param pool Pointer to pool
 *
 * @return Pointer to allocated block, or NULL if pool is empty
 */
void *rtos_mempool_alloc_from_isr(rtos_mempool_t *pool);

/**
 * Free a block from ISR context.
 *
 * @param pool           Pointer to pool
 * @param block          Pointer to block to free
 * @param yield_required Set to true if context switch should occur
 *
 * @return RTOS_OK on success
 */
rtos_status_t rtos_mempool_free_from_isr(
    rtos_mempool_t *pool,
    void *block,
    bool *yield_required
);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_USE_MEMORY_POOLS */

#endif /* RTOS_MEMORY_H */
