/**
 * MicroRTOS - Watchdog Implementation
 *
 * System and per-task watchdog management.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos_watchdog.h"
#include "rtos_types.h"

#if RTOS_USE_WATCHDOG || RTOS_USE_TASK_WATCHDOG

/*===========================================================================*/
/* External References                                                        */
/*===========================================================================*/

extern volatile uint32_t rtos_tick_count;
extern rtos_tcb_t *g_current_tcb;
extern uint32_t rtos_port_disable_interrupts(void);
extern void rtos_port_restore_interrupts(uint32_t state);

#if RTOS_USE_TASK_WATCHDOG
extern void rtos_task_suspend(rtos_tcb_t *tcb);
extern void rtos_task_delete(rtos_tcb_t *tcb);
#endif

/*===========================================================================*/
/* Module State                                                               */
/*===========================================================================*/

#if RTOS_USE_WATCHDOG
static rtos_system_wdt_t system_wdt = {
    .timeout_ms = 0,
    .last_feed_tick = 0,
    .enabled = false,
    .hw_enabled = false,
};
#endif

#if RTOS_USE_TASK_WATCHDOG
static rtos_task_wdt_entry_t task_wdt_entries[RTOS_WATCHDOG_MAX_TASKS];
static uint8_t task_wdt_count = 0;
#endif

/*===========================================================================*/
/* System Watchdog Implementation                                             */
/*===========================================================================*/

#if RTOS_USE_WATCHDOG

void rtos_wdt_init(uint32_t timeout_ms)
{
    system_wdt.timeout_ms = timeout_ms;
    system_wdt.last_feed_tick = 0;
    system_wdt.enabled = false;
    system_wdt.hw_enabled = false;

    /* Initialize hardware watchdog */
    rtos_port_wdt_init(timeout_ms);

#if RTOS_USE_TASK_WATCHDOG
    rtos_task_wdt_init();
#endif
}

void rtos_wdt_enable(void)
{
    if (!system_wdt.enabled) {
        system_wdt.enabled = true;
        system_wdt.last_feed_tick = rtos_tick_count;

        /* Enable hardware watchdog */
        rtos_port_wdt_enable();
        system_wdt.hw_enabled = true;
    }
}

void rtos_wdt_disable(void)
{
    if (system_wdt.enabled && rtos_port_wdt_can_disable()) {
        rtos_port_wdt_disable();
        system_wdt.enabled = false;
        system_wdt.hw_enabled = false;
    }
}

void rtos_wdt_feed(void)
{
    if (system_wdt.enabled) {
        system_wdt.last_feed_tick = rtos_tick_count;

        /* Feed hardware watchdog */
        if (system_wdt.hw_enabled) {
            rtos_port_wdt_feed();
        }
    }
}

bool rtos_wdt_is_enabled(void)
{
    return system_wdt.enabled;
}

uint32_t rtos_wdt_time_remaining(void)
{
    uint32_t elapsed_ticks;
    uint32_t timeout_ticks;
    uint32_t remaining;

    if (!system_wdt.enabled) {
        return 0;
    }

    elapsed_ticks = rtos_tick_count - system_wdt.last_feed_tick;
    timeout_ticks = RTOS_MS_TO_TICKS(system_wdt.timeout_ms);

    if (elapsed_ticks >= timeout_ticks) {
        return 0;
    }

    remaining = timeout_ticks - elapsed_ticks;
    return RTOS_TICKS_TO_MS(remaining);
}

#else /* !RTOS_USE_WATCHDOG */

/* Stub implementations */
void rtos_wdt_init(uint32_t timeout_ms) { (void)timeout_ms; }
void rtos_wdt_enable(void) {}
void rtos_wdt_disable(void) {}
void rtos_wdt_feed(void) {}
bool rtos_wdt_is_enabled(void) { return false; }
uint32_t rtos_wdt_time_remaining(void) { return 0; }

#endif /* RTOS_USE_WATCHDOG */

/*===========================================================================*/
/* Per-Task Watchdog Implementation                                           */
/*===========================================================================*/

#if RTOS_USE_TASK_WATCHDOG

void rtos_task_wdt_init(void)
{
    uint8_t i;

    for (i = 0; i < RTOS_WATCHDOG_MAX_TASKS; i++) {
        task_wdt_entries[i].tcb = NULL;
        task_wdt_entries[i].enabled = false;
        task_wdt_entries[i].callback = NULL;
    }
    task_wdt_count = 0;
}

/**
 * Find the watchdog entry for a task.
 */
static rtos_task_wdt_entry_t *find_task_wdt_entry(rtos_tcb_t *tcb)
{
    uint8_t i;

    for (i = 0; i < RTOS_WATCHDOG_MAX_TASKS; i++) {
        if (task_wdt_entries[i].tcb == tcb) {
            return &task_wdt_entries[i];
        }
    }
    return NULL;
}

/**
 * Find a free watchdog entry slot.
 */
static rtos_task_wdt_entry_t *find_free_wdt_entry(void)
{
    uint8_t i;

    for (i = 0; i < RTOS_WATCHDOG_MAX_TASKS; i++) {
        if (task_wdt_entries[i].tcb == NULL) {
            return &task_wdt_entries[i];
        }
    }
    return NULL;
}

rtos_status_t rtos_task_wdt_add(rtos_tcb_t *tcb, uint32_t timeout_ms)
{
    rtos_task_wdt_entry_t *entry;
    uint32_t state;

    if (tcb == NULL || timeout_ms == 0) {
        return RTOS_ERR_PARAM;
    }

    state = rtos_port_disable_interrupts();

    /* Check if already registered */
    entry = find_task_wdt_entry(tcb);
    if (entry != NULL) {
        /* Update existing entry */
        entry->timeout_ticks = RTOS_MS_TO_TICKS(timeout_ms);
        entry->last_feed_tick = rtos_tick_count;
        entry->enabled = true;
        rtos_port_restore_interrupts(state);
        return RTOS_OK;
    }

    /* Find free slot */
    entry = find_free_wdt_entry();
    if (entry == NULL) {
        rtos_port_restore_interrupts(state);
        return RTOS_ERR_NO_MEMORY;
    }

    /* Initialize entry */
    entry->tcb = tcb;
    entry->timeout_ticks = RTOS_MS_TO_TICKS(timeout_ms);
    entry->last_feed_tick = rtos_tick_count;
    entry->enabled = true;
    entry->action = RTOS_WDT_ACTION_CALLBACK;
    entry->callback = rtos_task_wdt_default_handler;

    task_wdt_count++;

    rtos_port_restore_interrupts(state);
    return RTOS_OK;
}

rtos_status_t rtos_task_wdt_enable(uint32_t timeout_ms)
{
    if (g_current_tcb == NULL) {
        return RTOS_ERR_STATE;
    }
    return rtos_task_wdt_add(g_current_tcb, timeout_ms);
}

void rtos_task_wdt_remove(rtos_tcb_t *tcb)
{
    rtos_task_wdt_entry_t *entry;
    uint32_t state;

    if (tcb == NULL) {
        return;
    }

    state = rtos_port_disable_interrupts();

    entry = find_task_wdt_entry(tcb);
    if (entry != NULL) {
        entry->tcb = NULL;
        entry->enabled = false;
        entry->callback = NULL;
        if (task_wdt_count > 0) {
            task_wdt_count--;
        }
    }

    rtos_port_restore_interrupts(state);
}

void rtos_task_wdt_disable(void)
{
    if (g_current_tcb != NULL) {
        rtos_task_wdt_remove(g_current_tcb);
    }
}

void rtos_task_wdt_feed_task(rtos_tcb_t *tcb)
{
    rtos_task_wdt_entry_t *entry;

    if (tcb == NULL) {
        return;
    }

    entry = find_task_wdt_entry(tcb);
    if (entry != NULL && entry->enabled) {
        entry->last_feed_tick = rtos_tick_count;
    }
}

void rtos_task_wdt_feed(void)
{
    if (g_current_tcb != NULL) {
        rtos_task_wdt_feed_task(g_current_tcb);
    }
}

void rtos_task_wdt_set_action(rtos_tcb_t *tcb, rtos_wdt_action_t action)
{
    rtos_task_wdt_entry_t *entry;

    if (tcb == NULL) {
        return;
    }

    entry = find_task_wdt_entry(tcb);
    if (entry != NULL) {
        entry->action = action;
    }
}

void rtos_task_wdt_set_callback(rtos_tcb_t *tcb,
                                 void (*callback)(rtos_tcb_t *))
{
    rtos_task_wdt_entry_t *entry;

    if (tcb == NULL) {
        return;
    }

    entry = find_task_wdt_entry(tcb);
    if (entry != NULL) {
        entry->callback = callback;
    }
}

void rtos_task_wdt_check(void)
{
    uint8_t i;
    uint32_t current_tick;
    uint32_t elapsed;
    rtos_task_wdt_entry_t *entry;

    if (task_wdt_count == 0) {
        return;
    }

    current_tick = rtos_tick_count;

    for (i = 0; i < RTOS_WATCHDOG_MAX_TASKS; i++) {
        entry = &task_wdt_entries[i];

        if (entry->tcb == NULL || !entry->enabled) {
            continue;
        }

        /* Calculate elapsed time since last feed */
        elapsed = current_tick - entry->last_feed_tick;

        if (elapsed >= entry->timeout_ticks) {
            /* Timeout occurred! */

            /* Call callback if registered */
            if (entry->callback != NULL) {
                entry->callback(entry->tcb);
            }

            /* Take configured action */
            switch (entry->action) {
                case RTOS_WDT_ACTION_RESET:
                    /* Force system reset via hardware watchdog */
                    /* Don't feed the hardware watchdog - let it reset */
                    while (1) { }
                    break;

                case RTOS_WDT_ACTION_SUSPEND_TASK:
                    rtos_task_suspend(entry->tcb);
                    entry->enabled = false;  /* Stop monitoring */
                    break;

                case RTOS_WDT_ACTION_DELETE_TASK:
                    rtos_task_delete(entry->tcb);
                    entry->tcb = NULL;
                    entry->enabled = false;
                    if (task_wdt_count > 0) {
                        task_wdt_count--;
                    }
                    break;

                case RTOS_WDT_ACTION_CALLBACK:
                default:
                    /* Just callback (already done above) */
                    /* Reset the feed time to prevent continuous callbacks */
                    entry->last_feed_tick = current_tick;
                    break;
            }
        }
    }
}

bool rtos_task_wdt_is_enabled(rtos_tcb_t *tcb)
{
    rtos_task_wdt_entry_t *entry;

    if (tcb == NULL) {
        return false;
    }

    entry = find_task_wdt_entry(tcb);
    return (entry != NULL && entry->enabled);
}

uint32_t rtos_task_wdt_time_remaining(rtos_tcb_t *tcb)
{
    rtos_task_wdt_entry_t *entry;
    uint32_t elapsed;
    uint32_t remaining;

    if (tcb == NULL) {
        return 0;
    }

    entry = find_task_wdt_entry(tcb);
    if (entry == NULL || !entry->enabled) {
        return 0;
    }

    elapsed = rtos_tick_count - entry->last_feed_tick;

    if (elapsed >= entry->timeout_ticks) {
        return 0;
    }

    remaining = entry->timeout_ticks - elapsed;
    return RTOS_TICKS_TO_MS(remaining);
}

/**
 * Default handler for task watchdog timeout.
 */
void rtos_task_wdt_default_handler(rtos_tcb_t *tcb)
{
    /*
     * Default behavior: log the timeout.
     * In a real application, this could:
     * - Send debug output to serial
     * - Log to flash
     * - Set an error flag
     * - Trigger system diagnostics
     */
    (void)tcb;

#if RTOS_USE_TRACING
    #include "rtos_trace.h"
    rtos_trace_event(RTOS_TRACE_STACK_OVERFLOW,
                     (uint8_t)((uintptr_t)tcb >> 2),
                     0, 0);
#endif
}

#endif /* RTOS_USE_TASK_WATCHDOG */

/*===========================================================================*/
/* Platform-Specific Default Implementations                                  */
/*===========================================================================*/

/**
 * Weak default implementations for platforms without hardware watchdog.
 */
__attribute__((weak))
void rtos_port_wdt_init(uint32_t timeout_ms)
{
    (void)timeout_ms;
}

__attribute__((weak))
void rtos_port_wdt_enable(void)
{
}

__attribute__((weak))
void rtos_port_wdt_disable(void)
{
}

__attribute__((weak))
void rtos_port_wdt_feed(void)
{
}

__attribute__((weak))
bool rtos_port_wdt_can_disable(void)
{
    return true;
}

#if RTOS_USE_WATCHDOG
__attribute__((weak))
void rtos_wdt_timeout_hook(void)
{
    /* Default: do nothing */
}
#endif

#endif /* RTOS_USE_WATCHDOG || RTOS_USE_TASK_WATCHDOG */
