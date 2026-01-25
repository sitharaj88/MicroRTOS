/**
 * MicroRTOS - Software Timer Implementation
 *
 * Lightweight software timers running from the system tick.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos_timer.h"
#include "rtos_list.h"
#include "rtos_port.h"

#if RTOS_USE_TIMERS

/*===========================================================================*/
/* External Declarations                                                      */
/*===========================================================================*/

extern volatile uint32_t g_tick_count;

/*===========================================================================*/
/* Local Variables                                                            */
/*===========================================================================*/

/* List of active timers sorted by expiration time */
static rtos_list_t g_timer_list;
static bool g_timer_list_initialized = false;

/*===========================================================================*/
/* Helper Functions                                                           */
/*===========================================================================*/

/**
 * Ensure timer list is initialized.
 */
static void ensure_timer_list_init(void)
{
    if (!g_timer_list_initialized) {
        rtos_list_init(&g_timer_list);
        g_timer_list_initialized = true;
    }
}

/**
 * Add a timer to the active list.
 */
static void add_timer_to_list(rtos_timer_t *timer)
{
    timer->node.value = timer->expire_tick;
    rtos_list_insert_sorted(&g_timer_list, &timer->node);
}

/**
 * Remove a timer from the active list.
 */
static void remove_timer_from_list(rtos_timer_t *timer)
{
    if (rtos_list_node_is_linked(&timer->node)) {
        rtos_list_remove(&g_timer_list, &timer->node);
    }
}

/*===========================================================================*/
/* Timer API Implementation                                                   */
/*===========================================================================*/

void rtos_timer_init(
    rtos_timer_t *timer,
    const char *name,
    uint32_t period,
    bool auto_reload,
    rtos_timer_callback_t callback)
{
    RTOS_ASSERT(timer != NULL);
    RTOS_ASSERT(period > 0);
    RTOS_ASSERT(callback != NULL);

    ensure_timer_list_init();

    rtos_list_node_init(&timer->node, timer);
    timer->period_ticks = period;
    timer->expire_tick = 0;
    timer->callback = callback;
    timer->user_data = NULL;
    timer->state = RTOS_TIMER_STOPPED;
    timer->auto_reload = auto_reload;

#if RTOS_USE_TASK_NAMES
    timer->name = (name != NULL) ? name : "?";
#else
    (void)name;
#endif
}

rtos_status_t rtos_timer_start(rtos_timer_t *timer)
{
    RTOS_ASSERT(timer != NULL);

    rtos_port_enter_critical();

    /* Remove if already in list */
    remove_timer_from_list(timer);

    /* Calculate expiration time */
    timer->expire_tick = g_tick_count + timer->period_ticks;
    timer->state = RTOS_TIMER_RUNNING;

    /* Add to timer list */
    add_timer_to_list(timer);

    rtos_port_exit_critical();

    return RTOS_OK;
}

rtos_status_t rtos_timer_stop(rtos_timer_t *timer)
{
    RTOS_ASSERT(timer != NULL);

    rtos_port_enter_critical();

    remove_timer_from_list(timer);
    timer->state = RTOS_TIMER_STOPPED;

    rtos_port_exit_critical();

    return RTOS_OK;
}

rtos_status_t rtos_timer_reset(rtos_timer_t *timer)
{
    /* Reset is effectively the same as start */
    return rtos_timer_start(timer);
}

rtos_status_t rtos_timer_change_period(rtos_timer_t *timer, uint32_t new_period)
{
    RTOS_ASSERT(timer != NULL);
    RTOS_ASSERT(new_period > 0);

    rtos_port_enter_critical();

    timer->period_ticks = new_period;

    /* If running, restart with new period */
    if (timer->state == RTOS_TIMER_RUNNING) {
        remove_timer_from_list(timer);
        timer->expire_tick = g_tick_count + new_period;
        add_timer_to_list(timer);
    }

    rtos_port_exit_critical();

    return RTOS_OK;
}

bool rtos_timer_is_running(rtos_timer_t *timer)
{
    if (timer == NULL) {
        return false;
    }
    return (timer->state == RTOS_TIMER_RUNNING);
}

uint32_t rtos_timer_get_period(rtos_timer_t *timer)
{
    if (timer == NULL) {
        return 0;
    }
    return timer->period_ticks;
}

uint32_t rtos_timer_get_remaining(rtos_timer_t *timer)
{
    int32_t remaining;

    if (timer == NULL || timer->state != RTOS_TIMER_RUNNING) {
        return 0;
    }

    rtos_port_enter_critical();
    remaining = (int32_t)(timer->expire_tick - g_tick_count);
    rtos_port_exit_critical();

    return (remaining > 0) ? (uint32_t)remaining : 0;
}

void rtos_timer_set_user_data(rtos_timer_t *timer, void *data)
{
    if (timer != NULL) {
        timer->user_data = data;
    }
}

void *rtos_timer_get_user_data(rtos_timer_t *timer)
{
    return (timer != NULL) ? timer->user_data : NULL;
}

const char *rtos_timer_get_name(rtos_timer_t *timer)
{
#if RTOS_USE_TASK_NAMES
    return (timer != NULL && timer->name != NULL) ? timer->name : "?";
#else
    (void)timer;
    return "?";
#endif
}

/*===========================================================================*/
/* Timer Processing (called from tick handler)                                */
/*===========================================================================*/

void rtos_timer_process(void)
{
    rtos_timer_t *timer;
    rtos_list_node_t *node;

    if (!g_timer_list_initialized) {
        return;
    }

    /* Process all expired timers */
    while (!rtos_list_is_empty(&g_timer_list)) {
        node = rtos_list_peek_head(&g_timer_list);
        timer = (rtos_timer_t *)node->container;

        /* Check if timer has expired */
        if ((int32_t)(timer->expire_tick - g_tick_count) > 0) {
            /* Not yet expired - list is sorted, so we're done */
            break;
        }

        /* Remove from list */
        rtos_list_remove_head(&g_timer_list);

        /* Mark as expired */
        timer->state = RTOS_TIMER_EXPIRED;

        /* Call callback */
        if (timer->callback != NULL) {
            timer->callback(timer);
        }

        /* Handle auto-reload */
        if (timer->auto_reload && timer->state == RTOS_TIMER_EXPIRED) {
            timer->expire_tick = g_tick_count + timer->period_ticks;
            timer->state = RTOS_TIMER_RUNNING;
            add_timer_to_list(timer);
        } else if (timer->state == RTOS_TIMER_EXPIRED) {
            timer->state = RTOS_TIMER_STOPPED;
        }
    }
}

#endif /* RTOS_USE_TIMERS */
