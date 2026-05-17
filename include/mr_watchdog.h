/**
 * MicroRTOS - Watchdog Integration
 *
 * System and per-task watchdog support for detecting hung tasks.
 * Integrates with hardware watchdog timers on supported platforms.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef MR_WATCHDOG_H
#define MR_WATCHDOG_H

#include "mr_config.h"
#include "mr_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Configuration                                                              */
/*===========================================================================*/

#ifndef MR_USE_WATCHDOG
#define MR_USE_WATCHDOG               0
#endif

#ifndef MR_USE_TASK_WATCHDOG
#define MR_USE_TASK_WATCHDOG          0
#endif

#ifndef MR_WATCHDOG_MAX_TASKS
#define MR_WATCHDOG_MAX_TASKS         8   /* Max tasks with watchdog */
#endif

#ifndef MR_WATCHDOG_DEFAULT_TIMEOUT_MS
#define MR_WATCHDOG_DEFAULT_TIMEOUT_MS    5000
#endif

/*===========================================================================*/
/* Watchdog Action                                                            */
/*===========================================================================*/

typedef enum {
    MR_WDT_ACTION_RESET = 0,      /* Reset the system (default) */
    MR_WDT_ACTION_CALLBACK,       /* Call user callback only */
    MR_WDT_ACTION_SUSPEND_TASK,   /* Suspend the offending task */
    MR_WDT_ACTION_DELETE_TASK,    /* Delete the offending task */
} mr_wdt_action_t;

/*===========================================================================*/
/* Task Watchdog Entry                                                        */
/*===========================================================================*/

typedef struct mr_task_wdt_entry {
    mr_tcb_t      *tcb;           /* Task being monitored */
    uint32_t        timeout_ticks;  /* Timeout in ticks */
    uint32_t        last_feed_tick; /* Last feed timestamp */
    bool            enabled;        /* Monitoring enabled */
    mr_wdt_action_t action;       /* Action on timeout */
    void (*callback)(mr_tcb_t *); /* Optional timeout callback */
} mr_task_wdt_entry_t;

/*===========================================================================*/
/* System Watchdog State                                                      */
/*===========================================================================*/

typedef struct {
    uint32_t        timeout_ms;     /* Hardware WDT timeout */
    uint32_t        last_feed_tick; /* Last feed timestamp */
    bool            enabled;        /* System WDT enabled */
    bool            hw_enabled;     /* Hardware WDT active */
} mr_system_wdt_t;

/*===========================================================================*/
/* System Watchdog API                                                        */
/*===========================================================================*/

/**
 * Initialize the system watchdog.
 *
 * @param timeout_ms    Hardware watchdog timeout in milliseconds
 */
void mr_wdt_init(uint32_t timeout_ms);

/**
 * Enable the system watchdog.
 * After calling this, mr_wdt_feed() must be called periodically.
 */
void mr_wdt_enable(void);

/**
 * Disable the system watchdog.
 * Note: Some hardware watchdogs cannot be disabled once enabled.
 */
void mr_wdt_disable(void);

/**
 * Feed (kick) the system watchdog.
 * Must be called more frequently than the timeout period.
 */
void mr_wdt_feed(void);

/**
 * Check if system watchdog is enabled.
 *
 * @return true if enabled, false otherwise
 */
bool mr_wdt_is_enabled(void);

/**
 * Get time remaining until watchdog timeout.
 *
 * @return Milliseconds remaining, or 0 if disabled
 */
uint32_t mr_wdt_time_remaining(void);

/*===========================================================================*/
/* Per-Task Watchdog API                                                      */
/*===========================================================================*/

#if MR_USE_TASK_WATCHDOG

/**
 * Initialize the task watchdog subsystem.
 * Called automatically by mr_wdt_init().
 */
void mr_task_wdt_init(void);

/**
 * Enable watchdog monitoring for a task.
 *
 * @param tcb           Task to monitor
 * @param timeout_ms    Timeout in milliseconds
 * @return              MR_OK on success, error code otherwise
 */
mr_status_t mr_task_wdt_add(mr_tcb_t *tcb, uint32_t timeout_ms);

/**
 * Enable watchdog for the current task.
 *
 * @param timeout_ms    Timeout in milliseconds
 * @return              MR_OK on success, error code otherwise
 */
mr_status_t mr_task_wdt_enable(uint32_t timeout_ms);

/**
 * Disable watchdog monitoring for a task.
 *
 * @param tcb           Task to stop monitoring
 */
void mr_task_wdt_remove(mr_tcb_t *tcb);

/**
 * Disable watchdog for the current task.
 */
void mr_task_wdt_disable(void);

/**
 * Feed the watchdog for a specific task.
 *
 * @param tcb           Task to feed
 */
void mr_task_wdt_feed_task(mr_tcb_t *tcb);

/**
 * Feed the watchdog for the current task.
 * Should be called periodically from long-running loops.
 */
void mr_task_wdt_feed(void);

/**
 * Set the action to take when a task watchdog expires.
 *
 * @param tcb           Task to configure
 * @param action        Action to take on timeout
 */
void mr_task_wdt_set_action(mr_tcb_t *tcb, mr_wdt_action_t action);

/**
 * Set callback for task watchdog timeout.
 *
 * @param tcb           Task to configure
 * @param callback      Function to call on timeout
 */
void mr_task_wdt_set_callback(mr_tcb_t *tcb,
                                 void (*callback)(mr_tcb_t *));

/**
 * Check task watchdogs and handle timeouts.
 * Called from the tick handler.
 */
void mr_task_wdt_check(void);

/**
 * Get task watchdog status.
 *
 * @param tcb           Task to query
 * @return              true if task has active watchdog
 */
bool mr_task_wdt_is_enabled(mr_tcb_t *tcb);

/**
 * Get time remaining for a task's watchdog.
 *
 * @param tcb           Task to query
 * @return              Milliseconds remaining, 0 if not monitored
 */
uint32_t mr_task_wdt_time_remaining(mr_tcb_t *tcb);

#endif /* MR_USE_TASK_WATCHDOG */

/*===========================================================================*/
/* Platform-Specific Functions (implement per platform)                       */
/*===========================================================================*/

/**
 * Initialize hardware watchdog timer.
 *
 * @param timeout_ms    Desired timeout in milliseconds
 */
extern void mr_port_wdt_init(uint32_t timeout_ms);

/**
 * Enable hardware watchdog.
 */
extern void mr_port_wdt_enable(void);

/**
 * Disable hardware watchdog (if supported).
 */
extern void mr_port_wdt_disable(void);

/**
 * Feed/kick the hardware watchdog.
 */
extern void mr_port_wdt_feed(void);

/**
 * Check if hardware watchdog can be disabled.
 *
 * @return true if watchdog can be disabled after enabling
 */
extern bool mr_port_wdt_can_disable(void);

/*===========================================================================*/
/* Watchdog Hooks                                                             */
/*===========================================================================*/

#if MR_USE_WATCHDOG

/**
 * User hook called before watchdog reset.
 * Can be used for emergency data saving.
 * Keep this function very short!
 */
extern void mr_wdt_timeout_hook(void);

#endif

#if MR_USE_TASK_WATCHDOG

/**
 * Default handler for task watchdog timeout.
 *
 * @param tcb           Task that timed out
 */
void mr_task_wdt_default_handler(mr_tcb_t *tcb);

#endif

#ifdef __cplusplus
}
#endif

#endif /* MR_WATCHDOG_H */
