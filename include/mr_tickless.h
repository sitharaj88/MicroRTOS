/**
 * MicroRTOS - Tickless Idle Mode
 *
 * Low-power tickless operation for battery-powered applications.
 * Suppresses periodic tick interrupts during idle to reduce power consumption.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef MR_TICKLESS_H
#define MR_TICKLESS_H

#include "mr_config.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Configuration                                                              */
/*===========================================================================*/

#ifndef MR_USE_TICKLESS_IDLE
#define MR_USE_TICKLESS_IDLE      0
#endif

#ifndef MR_TICKLESS_MIN_TICKS
#define MR_TICKLESS_MIN_TICKS     2   /* Minimum ticks worth sleeping */
#endif

#ifndef MR_TICKLESS_MAX_TICKS
#define MR_TICKLESS_MAX_TICKS     0xFFFFFF    /* Maximum sleep duration */
#endif

/*===========================================================================*/
/* Sleep Mode Selection                                                       */
/*===========================================================================*/

typedef enum {
    MR_SLEEP_IDLE = 0,        /* Lightest sleep, fastest wake */
    MR_SLEEP_STANDBY,         /* Deeper sleep, slower wake */
    MR_SLEEP_POWER_DOWN,      /* Deepest sleep, slowest wake */
} mr_sleep_mode_t;

/*===========================================================================*/
/* Tickless State                                                             */
/*===========================================================================*/

typedef struct {
    volatile bool       enabled;            /* Tickless mode enabled */
    volatile bool       sleeping;           /* Currently in sleep */
    mr_sleep_mode_t   sleep_mode;         /* Configured sleep mode */
    uint32_t            expected_ticks;     /* Expected sleep duration */
    uint32_t            actual_ticks;       /* Actual slept ticks */
    uint32_t            sleep_count;        /* Number of sleep entries */
    uint32_t            abort_count;        /* Sleep aborted early */
    uint32_t            total_slept_ticks;  /* Cumulative sleep ticks */
} mr_tickless_state_t;

/*===========================================================================*/
/* Tickless API                                                               */
/*===========================================================================*/

/**
 * Initialize tickless idle mode.
 * Must be called before using any tickless functions.
 */
void mr_tickless_init(void);

/**
 * Enable or disable tickless idle mode.
 *
 * @param enable    true to enable, false to disable
 */
void mr_tickless_enable(bool enable);

/**
 * Check if tickless mode is enabled.
 *
 * @return true if enabled, false otherwise
 */
bool mr_tickless_is_enabled(void);

/**
 * Set the sleep mode for tickless idle.
 *
 * @param mode      Sleep mode to use
 */
void mr_tickless_set_sleep_mode(mr_sleep_mode_t mode);

/**
 * Get current sleep mode.
 *
 * @return Current sleep mode
 */
mr_sleep_mode_t mr_tickless_get_sleep_mode(void);

/**
 * Calculate the maximum number of ticks the system can sleep.
 * Examines delayed task list and timer list to find next wake time.
 *
 * @return Maximum sleep time in ticks, 0 if cannot sleep
 */
uint32_t mr_tickless_get_sleep_time(void);

/**
 * Enter tickless idle state.
 * Called from idle task when no tasks are ready.
 * Will configure hardware for extended sleep and enter low-power mode.
 *
 * @param expected_ticks    Expected sleep duration in ticks
 */
void mr_tickless_enter(uint32_t expected_ticks);

/**
 * Exit tickless idle state.
 * Called when waking from sleep (timer or interrupt).
 * Compensates system tick counter for elapsed time.
 *
 * @param slept_ticks   Actual ticks slept
 */
void mr_tickless_exit(uint32_t slept_ticks);

/**
 * Compensate the system tick counter after sleeping.
 * Advances the tick count by the number of suppressed ticks.
 *
 * @param slept_ticks   Number of ticks that passed during sleep
 */
void mr_tickless_compensate(uint32_t slept_ticks);

/**
 * Abort tickless sleep early.
 * Called when an interrupt occurs during sleep that requires processing.
 */
void mr_tickless_abort(void);

/*===========================================================================*/
/* Statistics                                                                 */
/*===========================================================================*/

/**
 * Get the total number of times the system entered tickless sleep.
 *
 * @return Sleep entry count
 */
uint32_t mr_tickless_get_sleep_count(void);

/**
 * Get the total number of ticks spent sleeping.
 *
 * @return Total slept ticks
 */
uint32_t mr_tickless_get_total_slept(void);

/**
 * Get the number of times sleep was aborted early.
 *
 * @return Abort count
 */
uint32_t mr_tickless_get_abort_count(void);

/**
 * Reset tickless statistics.
 */
void mr_tickless_reset_stats(void);

/*===========================================================================*/
/* Port-Specific Functions (to be implemented per platform)                   */
/*===========================================================================*/

/**
 * Configure the hardware timer for extended sleep period.
 * Platform-specific implementation required.
 *
 * @param sleep_ticks   Number of ticks to sleep
 */
extern void mr_port_tickless_setup(uint32_t sleep_ticks);

/**
 * Enter low-power sleep mode.
 * Platform-specific implementation required.
 *
 * @param mode      Sleep mode to enter
 */
extern void mr_port_sleep_enter(mr_sleep_mode_t mode);

/**
 * Get the number of ticks elapsed during sleep.
 * Platform-specific implementation required.
 *
 * @return Number of ticks that actually elapsed
 */
extern uint32_t mr_port_tickless_get_elapsed(void);

/**
 * Restore normal tick operation after sleep.
 * Platform-specific implementation required.
 */
extern void mr_port_tickless_restore(void);

/**
 * Check if wakeup was due to timer or external interrupt.
 * Platform-specific implementation required.
 *
 * @return true if woken by timer, false if external interrupt
 */
extern bool mr_port_tickless_timer_wakeup(void);

/*===========================================================================*/
/* Pre-Sleep and Post-Sleep Hooks                                             */
/*===========================================================================*/

#if MR_USE_TICKLESS_HOOKS

/**
 * User hook called before entering sleep.
 * Can be used to prepare peripherals for low power.
 *
 * @param expected_ticks    Expected sleep duration
 */
extern void mr_tickless_pre_sleep_hook(uint32_t expected_ticks);

/**
 * User hook called after waking from sleep.
 * Can be used to restore peripheral states.
 *
 * @param slept_ticks   Actual time slept
 */
extern void mr_tickless_post_sleep_hook(uint32_t slept_ticks);

#endif /* MR_USE_TICKLESS_HOOKS */

#ifdef __cplusplus
}
#endif

#endif /* MR_TICKLESS_H */
