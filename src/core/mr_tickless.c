/**
 * MicroRTOS - Tickless Idle Mode Implementation
 *
 * Dynamic tick suppression for low-power operation.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_tickless.h"
#include "mr_types.h"
#include "mr_list.h"
#include "mr_task.h"

#if MR_USE_TICKLESS_IDLE

/*===========================================================================*/
/* External References                                                        */
/*===========================================================================*/

/* From kernel */
extern volatile uint32_t mr_tick_count;
extern mr_list_t mr_delayed_list;

/* From scheduler */
extern void mr_scheduler_process_delayed(void);

#if MR_USE_TIMERS
extern mr_list_t mr_timer_list;
#endif

/*===========================================================================*/
/* Module State                                                               */
/*===========================================================================*/

static mr_tickless_state_t tickless_state = {
    .enabled = false,
    .sleeping = false,
    .sleep_mode = MR_SLEEP_IDLE,
    .expected_ticks = 0,
    .actual_ticks = 0,
    .sleep_count = 0,
    .abort_count = 0,
    .total_slept_ticks = 0,
};

/*===========================================================================*/
/* Tickless API Implementation                                                */
/*===========================================================================*/

void mr_tickless_init(void)
{
    tickless_state.enabled = false;
    tickless_state.sleeping = false;
    tickless_state.sleep_mode = MR_SLEEP_IDLE;
    tickless_state.expected_ticks = 0;
    tickless_state.actual_ticks = 0;
    tickless_state.sleep_count = 0;
    tickless_state.abort_count = 0;
    tickless_state.total_slept_ticks = 0;
}

void mr_tickless_enable(bool enable)
{
    tickless_state.enabled = enable;
}

bool mr_tickless_is_enabled(void)
{
    return tickless_state.enabled;
}

void mr_tickless_set_sleep_mode(mr_sleep_mode_t mode)
{
    tickless_state.sleep_mode = mode;
}

mr_sleep_mode_t mr_tickless_get_sleep_mode(void)
{
    return tickless_state.sleep_mode;
}

uint32_t mr_tickless_get_sleep_time(void)
{
    uint32_t current_tick;
    uint32_t next_wake_tick;
    uint32_t sleep_ticks;
    mr_list_item_t *item;

    if (!tickless_state.enabled) {
        return 0;
    }

    current_tick = mr_tick_count;
    next_wake_tick = current_tick + MR_TICKLESS_MAX_TICKS;

    /* Check delayed task list for next wake time */
    if (!mr_list_is_empty(&mr_delayed_list)) {
        item = mr_delayed_list.head;
        if (item != NULL) {
            uint32_t task_wake_tick = item->sort_value;
            if ((int32_t)(task_wake_tick - current_tick) > 0) {
                if ((int32_t)(task_wake_tick - next_wake_tick) < 0) {
                    next_wake_tick = task_wake_tick;
                }
            } else {
                /* Task already ready, cannot sleep */
                return 0;
            }
        }
    }

#if MR_USE_TIMERS
    /* Check timer list for next expiration */
    if (!mr_list_is_empty(&mr_timer_list)) {
        item = mr_timer_list.head;
        if (item != NULL) {
            uint32_t timer_tick = item->sort_value;
            if ((int32_t)(timer_tick - current_tick) > 0) {
                if ((int32_t)(timer_tick - next_wake_tick) < 0) {
                    next_wake_tick = timer_tick;
                }
            } else {
                /* Timer already expired, cannot sleep */
                return 0;
            }
        }
    }
#endif

    /* Calculate sleep duration */
    sleep_ticks = next_wake_tick - current_tick;

    /* Apply limits */
    if (sleep_ticks < MR_TICKLESS_MIN_TICKS) {
        return 0;  /* Not worth sleeping */
    }

    if (sleep_ticks > MR_TICKLESS_MAX_TICKS) {
        sleep_ticks = MR_TICKLESS_MAX_TICKS;
    }

    return sleep_ticks;
}

void mr_tickless_enter(uint32_t expected_ticks)
{
    if (!tickless_state.enabled || expected_ticks < MR_TICKLESS_MIN_TICKS) {
        return;
    }

    /* Mark as sleeping */
    tickless_state.sleeping = true;
    tickless_state.expected_ticks = expected_ticks;
    tickless_state.sleep_count++;

#if MR_USE_TICKLESS_HOOKS
    /* Call pre-sleep hook */
    mr_tickless_pre_sleep_hook(expected_ticks);
#endif

    /* Configure hardware for extended sleep */
    mr_port_tickless_setup(expected_ticks);

    /* Enter low-power mode */
    mr_port_sleep_enter(tickless_state.sleep_mode);

    /*
     * Execution resumes here after wakeup.
     * Either the timer expired or an external interrupt occurred.
     */
}

void mr_tickless_exit(uint32_t slept_ticks)
{
    if (!tickless_state.sleeping) {
        return;
    }

    tickless_state.sleeping = false;
    tickless_state.actual_ticks = slept_ticks;
    tickless_state.total_slept_ticks += slept_ticks;

    /* Restore normal tick operation */
    mr_port_tickless_restore();

    /* Compensate tick counter */
    mr_tickless_compensate(slept_ticks);

#if MR_USE_TICKLESS_HOOKS
    /* Call post-sleep hook */
    mr_tickless_post_sleep_hook(slept_ticks);
#endif
}

void mr_tickless_compensate(uint32_t slept_ticks)
{
    if (slept_ticks == 0) {
        return;
    }

    /* Advance the system tick counter */
    mr_tick_count += slept_ticks;

    /*
     * Process any tasks or timers that became ready during sleep.
     * The scheduler will handle delayed list processing on next tick
     * or we can process it here immediately.
     */
    mr_scheduler_process_delayed();
}

void mr_tickless_abort(void)
{
    uint32_t elapsed;

    if (!tickless_state.sleeping) {
        return;
    }

    tickless_state.abort_count++;

    /* Get actual elapsed time from hardware */
    elapsed = mr_port_tickless_get_elapsed();

    /* Exit tickless mode with actual elapsed time */
    mr_tickless_exit(elapsed);
}

/*===========================================================================*/
/* Statistics                                                                 */
/*===========================================================================*/

uint32_t mr_tickless_get_sleep_count(void)
{
    return tickless_state.sleep_count;
}

uint32_t mr_tickless_get_total_slept(void)
{
    return tickless_state.total_slept_ticks;
}

uint32_t mr_tickless_get_abort_count(void)
{
    return tickless_state.abort_count;
}

void mr_tickless_reset_stats(void)
{
    tickless_state.sleep_count = 0;
    tickless_state.abort_count = 0;
    tickless_state.total_slept_ticks = 0;
}

/*===========================================================================*/
/* Idle Task Integration                                                      */
/*===========================================================================*/

/**
 * Tickless idle handler - called from idle task.
 * Determines if system can sleep and for how long.
 */
void mr_tickless_idle(void)
{
    uint32_t sleep_ticks;

    if (!tickless_state.enabled) {
        /* Standard idle - just WFI */
#if defined(__arm__) || defined(__ARM_ARCH)
        __asm__ __volatile__("wfi");
#elif defined(__AVR__)
        /* AVR sleep - handled differently */
#endif
        return;
    }

    /* Calculate maximum sleep time */
    sleep_ticks = mr_tickless_get_sleep_time();

    if (sleep_ticks >= MR_TICKLESS_MIN_TICKS) {
        /* Worth sleeping - enter tickless mode */
        mr_tickless_enter(sleep_ticks);

        /* We've woken up - get actual elapsed time */
        if (tickless_state.sleeping) {
            uint32_t elapsed = mr_port_tickless_get_elapsed();
            mr_tickless_exit(elapsed);
        }
    } else {
        /* Not worth sleeping - just wait for interrupt */
#if defined(__arm__) || defined(__ARM_ARCH)
        __asm__ __volatile__("wfi");
#endif
    }
}

#else /* !MR_USE_TICKLESS_IDLE */

/*===========================================================================*/
/* Stub Implementation when Tickless is Disabled                              */
/*===========================================================================*/

void mr_tickless_init(void) {}
void mr_tickless_enable(bool enable) { (void)enable; }
bool mr_tickless_is_enabled(void) { return false; }
void mr_tickless_set_sleep_mode(mr_sleep_mode_t mode) { (void)mode; }
mr_sleep_mode_t mr_tickless_get_sleep_mode(void) { return MR_SLEEP_IDLE; }
uint32_t mr_tickless_get_sleep_time(void) { return 0; }
void mr_tickless_enter(uint32_t expected_ticks) { (void)expected_ticks; }
void mr_tickless_exit(uint32_t slept_ticks) { (void)slept_ticks; }
void mr_tickless_compensate(uint32_t slept_ticks) { (void)slept_ticks; }
void mr_tickless_abort(void) {}
uint32_t mr_tickless_get_sleep_count(void) { return 0; }
uint32_t mr_tickless_get_total_slept(void) { return 0; }
uint32_t mr_tickless_get_abort_count(void) { return 0; }
void mr_tickless_reset_stats(void) {}

#endif /* MR_USE_TICKLESS_IDLE */
