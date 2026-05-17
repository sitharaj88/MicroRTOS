/**
 * MicroRTOS - Memory Pool API
 *
 * Fixed-size block memory pools for deterministic allocation.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef MR_MEMORY_H
#define MR_MEMORY_H

#include "mr_types.h"

#if MR_USE_MEMORY_POOLS

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
void mr_mempool_init(
    mr_mempool_t *pool,
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
void *mr_mempool_alloc(mr_mempool_t *pool, uint32_t timeout);

/**
 * Free a block back to a memory pool.
 *
 * @param pool  Pointer to pool
 * @param block Pointer to block to free
 *
 * @return MR_OK on success, MR_ERR_PARAM if block is invalid
 */
mr_status_t mr_mempool_free(mr_mempool_t *pool, void *block);

/**
 * Get the number of free blocks in a pool.
 *
 * @param pool Pointer to pool
 *
 * @return Number of free blocks
 */
uint16_t mr_mempool_available(mr_mempool_t *pool);

/**
 * Get the total number of blocks in a pool.
 *
 * @param pool Pointer to pool
 *
 * @return Total number of blocks
 */
uint16_t mr_mempool_total(mr_mempool_t *pool);

/**
 * Get the block size of a pool.
 *
 * @param pool Pointer to pool
 *
 * @return Block size in bytes
 */
uint16_t mr_mempool_block_size(mr_mempool_t *pool);

/**
 * Check if a pool is empty.
 *
 * @param pool Pointer to pool
 *
 * @return true if no blocks available, false otherwise
 */
bool mr_mempool_is_empty(mr_mempool_t *pool);

/**
 * Allocate a block from ISR context (non-blocking).
 *
 * @param pool Pointer to pool
 *
 * @return Pointer to allocated block, or NULL if pool is empty
 */
void *mr_mempool_alloc_from_isr(mr_mempool_t *pool);

/**
 * Free a block from ISR context.
 *
 * @param pool           Pointer to pool
 * @param block          Pointer to block to free
 * @param yield_required Set to true if context switch should occur
 *
 * @return MR_OK on success
 */
mr_status_t mr_mempool_free_from_isr(
    mr_mempool_t *pool,
    void *block,
    bool *yield_required
);

#ifdef __cplusplus
}
#endif

#endif /* MR_USE_MEMORY_POOLS */

#endif /* MR_MEMORY_H */
