/**
 * MicroRTOS - Software Timer API
 *
 * One-shot and periodic software timers with callbacks.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef RTOS_TIMER_H
#define RTOS_TIMER_H

#include "rtos_types.h"

#if RTOS_USE_TIMERS

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
typedef void (*rtos_timer_callback_t)(rtos_timer_t *timer);

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
void rtos_timer_init(
    rtos_timer_t *timer,
    const char *name,
    uint32_t period,
    bool auto_reload,
    rtos_timer_callback_t callback
);

/**
 * Start or restart a timer.
 *
 * If the timer is already running, it is reset to its period.
 *
 * @param timer Pointer to timer
 *
 * @return RTOS_OK on success
 */
rtos_status_t rtos_timer_start(rtos_timer_t *timer);

/**
 * Stop a running timer.
 *
 * @param timer Pointer to timer
 *
 * @return RTOS_OK on success
 */
rtos_status_t rtos_timer_stop(rtos_timer_t *timer);

/**
 * Reset a timer's period.
 *
 * If the timer is running, it restarts from the new period.
 * If stopped, it starts with the new period.
 *
 * @param timer Pointer to timer
 *
 * @return RTOS_OK on success
 */
rtos_status_t rtos_timer_reset(rtos_timer_t *timer);

/**
 * Change a timer's period.
 *
 * @param timer      Pointer to timer
 * @param new_period New period in ticks
 *
 * @return RTOS_OK on success
 */
rtos_status_t rtos_timer_change_period(rtos_timer_t *timer, uint32_t new_period);

/**
 * Check if a timer is running.
 *
 * @param timer Pointer to timer
 *
 * @return true if running, false if stopped
 */
bool rtos_timer_is_running(rtos_timer_t *timer);

/**
 * Get a timer's period.
 *
 * @param timer Pointer to timer
 *
 * @return Period in ticks
 */
uint32_t rtos_timer_get_period(rtos_timer_t *timer);

/**
 * Get the remaining time until a timer expires.
 *
 * @param timer Pointer to timer
 *
 * @return Remaining ticks, or 0 if stopped
 */
uint32_t rtos_timer_get_remaining(rtos_timer_t *timer);

/**
 * Set user data for a timer.
 *
 * @param timer Pointer to timer
 * @param data  User data pointer
 */
void rtos_timer_set_user_data(rtos_timer_t *timer, void *data);

/**
 * Get user data from a timer.
 *
 * @param timer Pointer to timer
 *
 * @return User data pointer
 */
void *rtos_timer_get_user_data(rtos_timer_t *timer);

/**
 * Get a timer's name.
 *
 * @param timer Pointer to timer
 *
 * @return Timer name, or "?" if names are disabled
 */
const char *rtos_timer_get_name(rtos_timer_t *timer);

/*===========================================================================*/
/* Timer Service (internal)                                                   */
/*===========================================================================*/

/**
 * Process expired timers.
 * Called from the kernel tick handler.
 */
void rtos_timer_process(void);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_USE_TIMERS */

#endif /* RTOS_TIMER_H */
