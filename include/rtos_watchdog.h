/**
 * MicroRTOS - Watchdog Integration
 *
 * System and per-task watchdog support for detecting hung tasks.
 * Integrates with hardware watchdog timers on supported platforms.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef RTOS_WATCHDOG_H
#define RTOS_WATCHDOG_H

#include "rtos_config.h"
#include "rtos_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Configuration                                                              */
/*===========================================================================*/

#ifndef RTOS_USE_WATCHDOG
#define RTOS_USE_WATCHDOG               0
#endif

#ifndef RTOS_USE_TASK_WATCHDOG
#define RTOS_USE_TASK_WATCHDOG          0
#endif

#ifndef RTOS_WATCHDOG_MAX_TASKS
#define RTOS_WATCHDOG_MAX_TASKS         8   /* Max tasks with watchdog */
#endif

#ifndef RTOS_WATCHDOG_DEFAULT_TIMEOUT_MS
#define RTOS_WATCHDOG_DEFAULT_TIMEOUT_MS    5000
#endif

/*===========================================================================*/
/* Watchdog Action                                                            */
/*===========================================================================*/

typedef enum {
    RTOS_WDT_ACTION_RESET = 0,      /* Reset the system (default) */
    RTOS_WDT_ACTION_CALLBACK,       /* Call user callback only */
    RTOS_WDT_ACTION_SUSPEND_TASK,   /* Suspend the offending task */
    RTOS_WDT_ACTION_DELETE_TASK,    /* Delete the offending task */
} rtos_wdt_action_t;

/*===========================================================================*/
/* Task Watchdog Entry                                                        */
/*===========================================================================*/

typedef struct rtos_task_wdt_entry {
    rtos_tcb_t      *tcb;           /* Task being monitored */
    uint32_t        timeout_ticks;  /* Timeout in ticks */
    uint32_t        last_feed_tick; /* Last feed timestamp */
    bool            enabled;        /* Monitoring enabled */
    rtos_wdt_action_t action;       /* Action on timeout */
    void (*callback)(rtos_tcb_t *); /* Optional timeout callback */
} rtos_task_wdt_entry_t;

/*===========================================================================*/
/* System Watchdog State                                                      */
/*===========================================================================*/

typedef struct {
    uint32_t        timeout_ms;     /* Hardware WDT timeout */
    uint32_t        last_feed_tick; /* Last feed timestamp */
    bool            enabled;        /* System WDT enabled */
    bool            hw_enabled;     /* Hardware WDT active */
} rtos_system_wdt_t;

/*===========================================================================*/
/* System Watchdog API                                                        */
/*===========================================================================*/

/**
 * Initialize the system watchdog.
 *
 * @param timeout_ms    Hardware watchdog timeout in milliseconds
 */
void rtos_wdt_init(uint32_t timeout_ms);

/**
 * Enable the system watchdog.
 * After calling this, rtos_wdt_feed() must be called periodically.
 */
void rtos_wdt_enable(void);

/**
 * Disable the system watchdog.
 * Note: Some hardware watchdogs cannot be disabled once enabled.
 */
void rtos_wdt_disable(void);

/**
 * Feed (kick) the system watchdog.
 * Must be called more frequently than the timeout period.
 */
void rtos_wdt_feed(void);

/**
 * Check if system watchdog is enabled.
 *
 * @return true if enabled, false otherwise
 */
bool rtos_wdt_is_enabled(void);

/**
 * Get time remaining until watchdog timeout.
 *
 * @return Milliseconds remaining, or 0 if disabled
 */
uint32_t rtos_wdt_time_remaining(void);

/*===========================================================================*/
/* Per-Task Watchdog API                                                      */
/*===========================================================================*/

#if RTOS_USE_TASK_WATCHDOG

/**
 * Initialize the task watchdog subsystem.
 * Called automatically by rtos_wdt_init().
 */
void rtos_task_wdt_init(void);

/**
 * Enable watchdog monitoring for a task.
 *
 * @param tcb           Task to monitor
 * @param timeout_ms    Timeout in milliseconds
 * @return              RTOS_OK on success, error code otherwise
 */
rtos_status_t rtos_task_wdt_add(rtos_tcb_t *tcb, uint32_t timeout_ms);

/**
 * Enable watchdog for the current task.
 *
 * @param timeout_ms    Timeout in milliseconds
 * @return              RTOS_OK on success, error code otherwise
 */
rtos_status_t rtos_task_wdt_enable(uint32_t timeout_ms);

/**
 * Disable watchdog monitoring for a task.
 *
 * @param tcb           Task to stop monitoring
 */
void rtos_task_wdt_remove(rtos_tcb_t *tcb);

/**
 * Disable watchdog for the current task.
 */
void rtos_task_wdt_disable(void);

/**
 * Feed the watchdog for a specific task.
 *
 * @param tcb           Task to feed
 */
void rtos_task_wdt_feed_task(rtos_tcb_t *tcb);

/**
 * Feed the watchdog for the current task.
 * Should be called periodically from long-running loops.
 */
void rtos_task_wdt_feed(void);

/**
 * Set the action to take when a task watchdog expires.
 *
 * @param tcb           Task to configure
 * @param action        Action to take on timeout
 */
void rtos_task_wdt_set_action(rtos_tcb_t *tcb, rtos_wdt_action_t action);

/**
 * Set callback for task watchdog timeout.
 *
 * @param tcb           Task to configure
 * @param callback      Function to call on timeout
 */
void rtos_task_wdt_set_callback(rtos_tcb_t *tcb,
                                 void (*callback)(rtos_tcb_t *));

/**
 * Check task watchdogs and handle timeouts.
 * Called from the tick handler.
 */
void rtos_task_wdt_check(void);

/**
 * Get task watchdog status.
 *
 * @param tcb           Task to query
 * @return              true if task has active watchdog
 */
bool rtos_task_wdt_is_enabled(rtos_tcb_t *tcb);

/**
 * Get time remaining for a task's watchdog.
 *
 * @param tcb           Task to query
 * @return              Milliseconds remaining, 0 if not monitored
 */
uint32_t rtos_task_wdt_time_remaining(rtos_tcb_t *tcb);

#endif /* RTOS_USE_TASK_WATCHDOG */

/*===========================================================================*/
/* Platform-Specific Functions (implement per platform)                       */
/*===========================================================================*/

/**
 * Initialize hardware watchdog timer.
 *
 * @param timeout_ms    Desired timeout in milliseconds
 */
extern void rtos_port_wdt_init(uint32_t timeout_ms);

/**
 * Enable hardware watchdog.
 */
extern void rtos_port_wdt_enable(void);

/**
 * Disable hardware watchdog (if supported).
 */
extern void rtos_port_wdt_disable(void);

/**
 * Feed/kick the hardware watchdog.
 */
extern void rtos_port_wdt_feed(void);

/**
 * Check if hardware watchdog can be disabled.
 *
 * @return true if watchdog can be disabled after enabling
 */
extern bool rtos_port_wdt_can_disable(void);

/*===========================================================================*/
/* Watchdog Hooks                                                             */
/*===========================================================================*/

#if RTOS_USE_WATCHDOG

/**
 * User hook called before watchdog reset.
 * Can be used for emergency data saving.
 * Keep this function very short!
 */
extern void rtos_wdt_timeout_hook(void);

#endif

#if RTOS_USE_TASK_WATCHDOG

/**
 * Default handler for task watchdog timeout.
 *
 * @param tcb           Task that timed out
 */
void rtos_task_wdt_default_handler(rtos_tcb_t *tcb);

#endif

#ifdef __cplusplus
}
#endif

#endif /* RTOS_WATCHDOG_H */
