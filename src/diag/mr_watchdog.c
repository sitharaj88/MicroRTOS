/**
 * MicroRTOS - Watchdog Implementation
 *
 * System and per-task watchdog management.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_watchdog.h"
#include "mr_types.h"

#if MR_USE_WATCHDOG || MR_USE_TASK_WATCHDOG

/*===========================================================================*/
/* External References                                                        */
/*===========================================================================*/

extern volatile uint32_t mr_tick_count;
extern mr_tcb_t *g_current_tcb;
extern uint32_t mr_port_disable_interrupts(void);
extern void mr_port_restore_interrupts(uint32_t state);

#if MR_USE_TASK_WATCHDOG
extern void mr_task_suspend(mr_tcb_t *tcb);
extern void mr_task_delete(mr_tcb_t *tcb);
#endif

/*===========================================================================*/
/* Module State                                                               */
/*===========================================================================*/

#if MR_USE_WATCHDOG
static mr_system_wdt_t system_wdt = {
    .timeout_ms = 0,
    .last_feed_tick = 0,
    .enabled = false,
    .hw_enabled = false,
};
#endif

#if MR_USE_TASK_WATCHDOG
static mr_task_wdt_entry_t task_wdt_entries[MR_WATCHDOG_MAX_TASKS];
static uint8_t task_wdt_count = 0;
#endif

/*===========================================================================*/
/* System Watchdog Implementation                                             */
/*===========================================================================*/

#if MR_USE_WATCHDOG

void mr_wdt_init(uint32_t timeout_ms)
{
    system_wdt.timeout_ms = timeout_ms;
    system_wdt.last_feed_tick = 0;
    system_wdt.enabled = false;
    system_wdt.hw_enabled = false;

    /* Initialize hardware watchdog */
    mr_port_wdt_init(timeout_ms);

#if MR_USE_TASK_WATCHDOG
    mr_task_wdt_init();
#endif
}

void mr_wdt_enable(void)
{
    if (!system_wdt.enabled) {
        system_wdt.enabled = true;
        system_wdt.last_feed_tick = mr_tick_count;

        /* Enable hardware watchdog */
        mr_port_wdt_enable();
        system_wdt.hw_enabled = true;
    }
}

void mr_wdt_disable(void)
{
    if (system_wdt.enabled && mr_port_wdt_can_disable()) {
        mr_port_wdt_disable();
        system_wdt.enabled = false;
        system_wdt.hw_enabled = false;
    }
}

void mr_wdt_feed(void)
{
    if (system_wdt.enabled) {
        system_wdt.last_feed_tick = mr_tick_count;

        /* Feed hardware watchdog */
        if (system_wdt.hw_enabled) {
            mr_port_wdt_feed();
        }
    }
}

bool mr_wdt_is_enabled(void)
{
    return system_wdt.enabled;
}

uint32_t mr_wdt_time_remaining(void)
{
    uint32_t elapsed_ticks;
    uint32_t timeout_ticks;
    uint32_t remaining;

    if (!system_wdt.enabled) {
        return 0;
    }

    elapsed_ticks = mr_tick_count - system_wdt.last_feed_tick;
    timeout_ticks = MR_MS_TO_TICKS(system_wdt.timeout_ms);

    if (elapsed_ticks >= timeout_ticks) {
        return 0;
    }

    remaining = timeout_ticks - elapsed_ticks;
    return MR_TICKS_TO_MS(remaining);
}

#else /* !MR_USE_WATCHDOG */

/* Stub implementations */
void mr_wdt_init(uint32_t timeout_ms) { (void)timeout_ms; }
void mr_wdt_enable(void) {}
void mr_wdt_disable(void) {}
void mr_wdt_feed(void) {}
bool mr_wdt_is_enabled(void) { return false; }
uint32_t mr_wdt_time_remaining(void) { return 0; }

#endif /* MR_USE_WATCHDOG */

/*===========================================================================*/
/* Per-Task Watchdog Implementation                                           */
/*===========================================================================*/

#if MR_USE_TASK_WATCHDOG

void mr_task_wdt_init(void)
{
    uint8_t i;

    for (i = 0; i < MR_WATCHDOG_MAX_TASKS; i++) {
        task_wdt_entries[i].tcb = NULL;
        task_wdt_entries[i].enabled = false;
        task_wdt_entries[i].callback = NULL;
    }
    task_wdt_count = 0;
}

/**
 * Find the watchdog entry for a task.
 */
static mr_task_wdt_entry_t *find_task_wdt_entry(mr_tcb_t *tcb)
{
    uint8_t i;

    for (i = 0; i < MR_WATCHDOG_MAX_TASKS; i++) {
        if (task_wdt_entries[i].tcb == tcb) {
            return &task_wdt_entries[i];
        }
    }
    return NULL;
}

/**
 * Find a free watchdog entry slot.
 */
static mr_task_wdt_entry_t *find_free_wdt_entry(void)
{
    uint8_t i;

    for (i = 0; i < MR_WATCHDOG_MAX_TASKS; i++) {
        if (task_wdt_entries[i].tcb == NULL) {
            return &task_wdt_entries[i];
        }
    }
    return NULL;
}

mr_status_t mr_task_wdt_add(mr_tcb_t *tcb, uint32_t timeout_ms)
{
    mr_task_wdt_entry_t *entry;
    uint32_t state;

    if (tcb == NULL || timeout_ms == 0) {
        return MR_ERR_PARAM;
    }

    state = mr_port_disable_interrupts();

    /* Check if already registered */
    entry = find_task_wdt_entry(tcb);
    if (entry != NULL) {
        /* Update existing entry */
        entry->timeout_ticks = MR_MS_TO_TICKS(timeout_ms);
        entry->last_feed_tick = mr_tick_count;
        entry->enabled = true;
        mr_port_restore_interrupts(state);
        return MR_OK;
    }

    /* Find free slot */
    entry = find_free_wdt_entry();
    if (entry == NULL) {
        mr_port_restore_interrupts(state);
        return MR_ERR_NO_MEMORY;
    }

    /* Initialize entry */
    entry->tcb = tcb;
    entry->timeout_ticks = MR_MS_TO_TICKS(timeout_ms);
    entry->last_feed_tick = mr_tick_count;
    entry->enabled = true;
    entry->action = MR_WDT_ACTION_CALLBACK;
    entry->callback = mr_task_wdt_default_handler;

    task_wdt_count++;

    mr_port_restore_interrupts(state);
    return MR_OK;
}

mr_status_t mr_task_wdt_enable(uint32_t timeout_ms)
{
    if (g_current_tcb == NULL) {
        return MR_ERR_STATE;
    }
    return mr_task_wdt_add(g_current_tcb, timeout_ms);
}

void mr_task_wdt_remove(mr_tcb_t *tcb)
{
    mr_task_wdt_entry_t *entry;
    uint32_t state;

    if (tcb == NULL) {
        return;
    }

    state = mr_port_disable_interrupts();

    entry = find_task_wdt_entry(tcb);
    if (entry != NULL) {
        entry->tcb = NULL;
        entry->enabled = false;
        entry->callback = NULL;
        if (task_wdt_count > 0) {
            task_wdt_count--;
        }
    }

    mr_port_restore_interrupts(state);
}

void mr_task_wdt_disable(void)
{
    if (g_current_tcb != NULL) {
        mr_task_wdt_remove(g_current_tcb);
    }
}

void mr_task_wdt_feed_task(mr_tcb_t *tcb)
{
    mr_task_wdt_entry_t *entry;

    if (tcb == NULL) {
        return;
    }

    entry = find_task_wdt_entry(tcb);
    if (entry != NULL && entry->enabled) {
        entry->last_feed_tick = mr_tick_count;
    }
}

void mr_task_wdt_feed(void)
{
    if (g_current_tcb != NULL) {
        mr_task_wdt_feed_task(g_current_tcb);
    }
}

void mr_task_wdt_set_action(mr_tcb_t *tcb, mr_wdt_action_t action)
{
    mr_task_wdt_entry_t *entry;

    if (tcb == NULL) {
        return;
    }

    entry = find_task_wdt_entry(tcb);
    if (entry != NULL) {
        entry->action = action;
    }
}

void mr_task_wdt_set_callback(mr_tcb_t *tcb,
                                 void (*callback)(mr_tcb_t *))
{
    mr_task_wdt_entry_t *entry;

    if (tcb == NULL) {
        return;
    }

    entry = find_task_wdt_entry(tcb);
    if (entry != NULL) {
        entry->callback = callback;
    }
}

void mr_task_wdt_check(void)
{
    uint8_t i;
    uint32_t current_tick;
    uint32_t elapsed;
    mr_task_wdt_entry_t *entry;

    if (task_wdt_count == 0) {
        return;
    }

    current_tick = mr_tick_count;

    for (i = 0; i < MR_WATCHDOG_MAX_TASKS; i++) {
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
                case MR_WDT_ACTION_RESET:
                    /* Force system reset via hardware watchdog */
                    /* Don't feed the hardware watchdog - let it reset */
                    while (1) { }
                    break;

                case MR_WDT_ACTION_SUSPEND_TASK:
                    mr_task_suspend(entry->tcb);
                    entry->enabled = false;  /* Stop monitoring */
                    break;

                case MR_WDT_ACTION_DELETE_TASK:
                    mr_task_delete(entry->tcb);
                    entry->tcb = NULL;
                    entry->enabled = false;
                    if (task_wdt_count > 0) {
                        task_wdt_count--;
                    }
                    break;

                case MR_WDT_ACTION_CALLBACK:
                default:
                    /* Just callback (already done above) */
                    /* Reset the feed time to prevent continuous callbacks */
                    entry->last_feed_tick = current_tick;
                    break;
            }
        }
    }
}

bool mr_task_wdt_is_enabled(mr_tcb_t *tcb)
{
    mr_task_wdt_entry_t *entry;

    if (tcb == NULL) {
        return false;
    }

    entry = find_task_wdt_entry(tcb);
    return (entry != NULL && entry->enabled);
}

uint32_t mr_task_wdt_time_remaining(mr_tcb_t *tcb)
{
    mr_task_wdt_entry_t *entry;
    uint32_t elapsed;
    uint32_t remaining;

    if (tcb == NULL) {
        return 0;
    }

    entry = find_task_wdt_entry(tcb);
    if (entry == NULL || !entry->enabled) {
        return 0;
    }

    elapsed = mr_tick_count - entry->last_feed_tick;

    if (elapsed >= entry->timeout_ticks) {
        return 0;
    }

    remaining = entry->timeout_ticks - elapsed;
    return MR_TICKS_TO_MS(remaining);
}

/**
 * Default handler for task watchdog timeout.
 */
void mr_task_wdt_default_handler(mr_tcb_t *tcb)
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

#if MR_USE_TRACING
    #include "mr_trace.h"
    mr_trace_event(MR_TRACE_STACK_OVERFLOW,
                     (uint8_t)((uintptr_t)tcb >> 2),
                     0, 0);
#endif
}

#endif /* MR_USE_TASK_WATCHDOG */

/*===========================================================================*/
/* Platform-Specific Default Implementations                                  */
/*===========================================================================*/

/**
 * Weak default implementations for platforms without hardware watchdog.
 */
__attribute__((weak))
void mr_port_wdt_init(uint32_t timeout_ms)
{
    (void)timeout_ms;
}

__attribute__((weak))
void mr_port_wdt_enable(void)
{
}

__attribute__((weak))
void mr_port_wdt_disable(void)
{
}

__attribute__((weak))
void mr_port_wdt_feed(void)
{
}

__attribute__((weak))
bool mr_port_wdt_can_disable(void)
{
    return true;
}

#if MR_USE_WATCHDOG
__attribute__((weak))
void mr_wdt_timeout_hook(void)
{
    /* Default: do nothing */
}
#endif

#endif /* MR_USE_WATCHDOG || MR_USE_TASK_WATCHDOG */
