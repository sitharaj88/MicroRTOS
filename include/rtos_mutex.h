/**
 * MicroRTOS - Mutex API
 *
 * Mutual exclusion locks with priority inheritance to prevent
 * unbounded priority inversion.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef RTOS_MUTEX_H
#define RTOS_MUTEX_H

#include "rtos_types.h"

#if RTOS_USE_MUTEXES

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Mutex API                                                                  */
/*===========================================================================*/

/**
 * Initialize a mutex.
 *
 * @param mutex Pointer to mutex structure
 */
void rtos_mutex_init(rtos_mutex_t *mutex);

/**
 * Acquire a mutex with optional timeout.
 *
 * If the mutex is held by another task, the calling task will block
 * until the mutex becomes available or the timeout expires.
 *
 * Priority inheritance: If a higher priority task blocks on a mutex
 * held by a lower priority task, the holder's priority is temporarily
 * raised to prevent priority inversion.
 *
 * @param mutex   Pointer to mutex
 * @param timeout Maximum ticks to wait (RTOS_WAIT_FOREVER, RTOS_NO_WAIT)
 *
 * @return RTOS_OK if acquired, RTOS_ERR_TIMEOUT if timeout expired
 */
rtos_status_t rtos_mutex_lock(rtos_mutex_t *mutex, uint32_t timeout);

/**
 * Try to acquire a mutex without blocking.
 *
 * @param mutex Pointer to mutex
 *
 * @return RTOS_OK if acquired, RTOS_ERR_TIMEOUT if mutex is held
 */
rtos_status_t rtos_mutex_trylock(rtos_mutex_t *mutex);

/**
 * Release a mutex.
 *
 * Must be called by the task that holds the mutex. If priority
 * inheritance was applied, the owner's priority is restored.
 *
 * @param mutex Pointer to mutex
 *
 * @return RTOS_OK on success, RTOS_ERR_NOT_OWNER if not the owner
 */
rtos_status_t rtos_mutex_unlock(rtos_mutex_t *mutex);

/**
 * Get the current owner of a mutex.
 *
 * @param mutex Pointer to mutex
 *
 * @return Pointer to owner TCB, or NULL if unlocked
 */
rtos_tcb_t *rtos_mutex_get_owner(rtos_mutex_t *mutex);

/**
 * Check if a mutex is currently locked.
 *
 * @param mutex Pointer to mutex
 *
 * @return true if locked, false if available
 */
bool rtos_mutex_is_locked(rtos_mutex_t *mutex);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_USE_MUTEXES */

#endif /* RTOS_MUTEX_H */
