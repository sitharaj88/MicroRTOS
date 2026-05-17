/**
 * MicroRTOS - Mutex API
 *
 * Mutual exclusion locks with priority inheritance to prevent
 * unbounded priority inversion.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef MR_MUTEX_H
#define MR_MUTEX_H

#include "mr_types.h"

#if MR_USE_MUTEXES

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
void mr_mutex_init(mr_mutex_t *mutex);

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
 * @param timeout Maximum ticks to wait (MR_WAIT_FOREVER, MR_NO_WAIT)
 *
 * @return MR_OK if acquired, MR_ERR_TIMEOUT if timeout expired
 */
mr_status_t mr_mutex_lock(mr_mutex_t *mutex, uint32_t timeout);

/**
 * Try to acquire a mutex without blocking.
 *
 * @param mutex Pointer to mutex
 *
 * @return MR_OK if acquired, MR_ERR_TIMEOUT if mutex is held
 */
mr_status_t mr_mutex_trylock(mr_mutex_t *mutex);

/**
 * Release a mutex.
 *
 * Must be called by the task that holds the mutex. If priority
 * inheritance was applied, the owner's priority is restored.
 *
 * @param mutex Pointer to mutex
 *
 * @return MR_OK on success, MR_ERR_NOT_OWNER if not the owner
 */
mr_status_t mr_mutex_unlock(mr_mutex_t *mutex);

/**
 * Get the current owner of a mutex.
 *
 * @param mutex Pointer to mutex
 *
 * @return Pointer to owner TCB, or NULL if unlocked
 */
mr_tcb_t *mr_mutex_get_owner(mr_mutex_t *mutex);

/**
 * Check if a mutex is currently locked.
 *
 * @param mutex Pointer to mutex
 *
 * @return true if locked, false if available
 */
bool mr_mutex_is_locked(mr_mutex_t *mutex);

#ifdef __cplusplus
}
#endif

#endif /* MR_USE_MUTEXES */

#endif /* MR_MUTEX_H */
