/**
 * MicroRTOS - Semaphore API
 *
 * Binary and counting semaphores for synchronization.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef RTOS_SEMAPHORE_H
#define RTOS_SEMAPHORE_H

#include "rtos_types.h"

#if RTOS_USE_SEMAPHORES

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Semaphore API                                                              */
/*===========================================================================*/

/**
 * Initialize a semaphore.
 *
 * For a binary semaphore: initial=1, max=1
 * For a counting semaphore: initial=N, max=M
 *
 * @param sem     Pointer to semaphore structure
 * @param initial Initial count value
 * @param max     Maximum count value (use 1 for binary semaphore)
 */
void rtos_sem_init(rtos_sem_t *sem, uint32_t initial, uint32_t max);

/**
 * Initialize a binary semaphore.
 *
 * Convenience function that creates a semaphore with max=1.
 *
 * @param sem     Pointer to semaphore structure
 * @param initial Initial value (0 or 1)
 */
void rtos_sem_init_binary(rtos_sem_t *sem, uint32_t initial);

/**
 * Take (decrement) a semaphore with optional timeout.
 *
 * If the count is zero, the calling task will block until the
 * semaphore is given or the timeout expires.
 *
 * @param sem     Pointer to semaphore
 * @param timeout Maximum ticks to wait (RTOS_WAIT_FOREVER, RTOS_NO_WAIT)
 *
 * @return RTOS_OK if taken, RTOS_ERR_TIMEOUT if timeout expired
 */
rtos_status_t rtos_sem_take(rtos_sem_t *sem, uint32_t timeout);

/**
 * Give (increment) a semaphore.
 *
 * Increments the count and wakes a waiting task if any.
 * If count would exceed max, the semaphore is not given.
 *
 * @param sem Pointer to semaphore
 *
 * @return RTOS_OK on success, RTOS_ERR_FULL if at maximum count
 */
rtos_status_t rtos_sem_give(rtos_sem_t *sem);

/**
 * Give a semaphore from an ISR.
 *
 * @param sem            Pointer to semaphore
 * @param yield_required Set to true if a context switch should occur
 *
 * @return RTOS_OK on success, RTOS_ERR_FULL if at maximum count
 */
rtos_status_t rtos_sem_give_from_isr(rtos_sem_t *sem, bool *yield_required);

/**
 * Get the current count of a semaphore.
 *
 * @param sem Pointer to semaphore
 *
 * @return Current count value
 */
uint32_t rtos_sem_get_count(rtos_sem_t *sem);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_USE_SEMAPHORES */

#endif /* RTOS_SEMAPHORE_H */
