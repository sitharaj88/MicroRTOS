/**
 * MicroRTOS - Software Timer API
 *
 * One-shot and periodic software timers with callbacks.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef MR_TIMER_H
#define MR_TIMER_H

#include "mr_types.h"

#if MR_USE_TIMERS

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Timer Callback Type                                                        */
/*===========================================================================*/

/**
 * Timer callback function signature.
 *
 * @param timer Pointer to the timer that expired
 */
typedef void (*mr_timer_callback_t)(mr_timer_t *timer);

/*===========================================================================*/
/* Timer API                                                                  */
/*===========================================================================*/

/**
 * Initialize a software timer.
 *
 * @param timer       Pointer to timer structure
 * @param name        Timer name for debugging (can be NULL)
 * @param period      Timer period in ticks
 * @param auto_reload true for periodic, false for one-shot
 * @param callback    Function to call when timer expires
 */
void mr_timer_init(
    mr_timer_t *timer,
    const char *name,
    uint32_t period,
    bool auto_reload,
    mr_timer_callback_t callback
);

/**
 * Start or restart a timer.
 *
 * If the timer is already running, it is reset to its period.
 *
 * @param timer Pointer to timer
 *
 * @return MR_OK on success
 */
mr_status_t mr_timer_start(mr_timer_t *timer);

/**
 * Stop a running timer.
 *
 * @param timer Pointer to timer
 *
 * @return MR_OK on success
 */
mr_status_t mr_timer_stop(mr_timer_t *timer);

/**
 * Reset a timer's period.
 *
 * If the timer is running, it restarts from the new period.
 * If stopped, it starts with the new period.
 *
 * @param timer Pointer to timer
 *
 * @return MR_OK on success
 */
mr_status_t mr_timer_reset(mr_timer_t *timer);

/**
 * Change a timer's period.
 *
 * @param timer      Pointer to timer
 * @param new_period New period in ticks
 *
 * @return MR_OK on success
 */
mr_status_t mr_timer_change_period(mr_timer_t *timer, uint32_t new_period);

/**
 * Check if a timer is running.
 *
 * @param timer Pointer to timer
 *
 * @return true if running, false if stopped
 */
bool mr_timer_is_running(mr_timer_t *timer);

/**
 * Get a timer's period.
 *
 * @param timer Pointer to timer
 *
 * @return Period in ticks
 */
uint32_t mr_timer_get_period(mr_timer_t *timer);

/**
 * Get the remaining time until a timer expires.
 *
 * @param timer Pointer to timer
 *
 * @return Remaining ticks, or 0 if stopped
 */
uint32_t mr_timer_get_remaining(mr_timer_t *timer);

/**
 * Set user data for a timer.
 *
 * @param timer Pointer to timer
 * @param data  User data pointer
 */
void mr_timer_set_user_data(mr_timer_t *timer, void *data);

/**
 * Get user data from a timer.
 *
 * @param timer Pointer to timer
 *
 * @return User data pointer
 */
void *mr_timer_get_user_data(mr_timer_t *timer);

/**
 * Get a timer's name.
 *
 * @param timer Pointer to timer
 *
 * @return Timer name, or "?" if names are disabled
 */
const char *mr_timer_get_name(mr_timer_t *timer);

/*===========================================================================*/
/* Timer Service (internal)                                                   */
/*===========================================================================*/

/**
 * Process expired timers.
 * Called from the kernel tick handler.
 */
void mr_timer_process(void);

#ifdef __cplusplus
}
#endif

#endif /* MR_USE_TIMERS */

#endif /* MR_TIMER_H */
