/**
 * MicroRTOS - Software Timer Implementation
 *
 * Lightweight software timers running from the system tick.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_timer.h"
#include "mr_list.h"
#include "mr_port.h"

#if MR_USE_TIMERS

/*===========================================================================*/
/* External Declarations                                                      */
/*===========================================================================*/

extern volatile uint32_t g_tick_count;

/*===========================================================================*/
/* Local Variables                                                            */
/*===========================================================================*/

/* List of active timers sorted by expiration time */
static mr_list_t g_timer_list;
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
        mr_list_init(&g_timer_list);
        g_timer_list_initialized = true;
    }
}

/**
 * Add a timer to the active list.
 */
static void add_timer_to_list(mr_timer_t *timer)
{
    timer->node.value = timer->expire_tick;
    mr_list_insert_sorted(&g_timer_list, &timer->node);
}

/**
 * Remove a timer from the active list.
 */
static void remove_timer_from_list(mr_timer_t *timer)
{
    if (mr_list_node_is_linked(&timer->node)) {
        mr_list_remove(&g_timer_list, &timer->node);
    }
}

/*===========================================================================*/
/* Timer API Implementation                                                   */
/*===========================================================================*/

void mr_timer_init(
    mr_timer_t *timer,
    const char *name,
    uint32_t period,
    bool auto_reload,
    mr_timer_callback_t callback)
{
    MR_ASSERT(timer != NULL);
    MR_ASSERT(period > 0);
    MR_ASSERT(callback != NULL);

    ensure_timer_list_init();

    mr_list_node_init(&timer->node, timer);
    timer->period_ticks = period;
    timer->expire_tick = 0;
    timer->callback = callback;
    timer->user_data = NULL;
    timer->state = MR_TIMER_STOPPED;
    timer->auto_reload = auto_reload;

#if MR_USE_TASK_NAMES
    timer->name = (name != NULL) ? name : "?";
#else
    (void)name;
#endif
}

mr_status_t mr_timer_start(mr_timer_t *timer)
{
    MR_ASSERT(timer != NULL);

    mr_port_enter_critical();

    /* Remove if already in list */
    remove_timer_from_list(timer);

    /* Calculate expiration time */
    timer->expire_tick = g_tick_count + timer->period_ticks;
    timer->state = MR_TIMER_RUNNING;

    /* Add to timer list */
    add_timer_to_list(timer);

    mr_port_exit_critical();

    return MR_OK;
}

mr_status_t mr_timer_stop(mr_timer_t *timer)
{
    MR_ASSERT(timer != NULL);

    mr_port_enter_critical();

    remove_timer_from_list(timer);
    timer->state = MR_TIMER_STOPPED;

    mr_port_exit_critical();

    return MR_OK;
}

mr_status_t mr_timer_reset(mr_timer_t *timer)
{
    /* Reset is effectively the same as start */
    return mr_timer_start(timer);
}

mr_status_t mr_timer_change_period(mr_timer_t *timer, uint32_t new_period)
{
    MR_ASSERT(timer != NULL);
    MR_ASSERT(new_period > 0);

    mr_port_enter_critical();

    timer->period_ticks = new_period;

    /* If running, restart with new period */
    if (timer->state == MR_TIMER_RUNNING) {
        remove_timer_from_list(timer);
        timer->expire_tick = g_tick_count + new_period;
        add_timer_to_list(timer);
    }

    mr_port_exit_critical();

    return MR_OK;
}

bool mr_timer_is_running(mr_timer_t *timer)
{
    if (timer == NULL) {
        return false;
    }
    return (timer->state == MR_TIMER_RUNNING);
}

uint32_t mr_timer_get_period(mr_timer_t *timer)
{
    if (timer == NULL) {
        return 0;
    }
    return timer->period_ticks;
}

uint32_t mr_timer_get_remaining(mr_timer_t *timer)
{
    int32_t remaining;

    if (timer == NULL || timer->state != MR_TIMER_RUNNING) {
        return 0;
    }

    mr_port_enter_critical();
    remaining = (int32_t)(timer->expire_tick - g_tick_count);
    mr_port_exit_critical();

    return (remaining > 0) ? (uint32_t)remaining : 0;
}

void mr_timer_set_user_data(mr_timer_t *timer, void *data)
{
    if (timer != NULL) {
        timer->user_data = data;
    }
}

void *mr_timer_get_user_data(mr_timer_t *timer)
{
    return (timer != NULL) ? timer->user_data : NULL;
}

const char *mr_timer_get_name(mr_timer_t *timer)
{
#if MR_USE_TASK_NAMES
    return (timer != NULL && timer->name != NULL) ? timer->name : "?";
#else
    (void)timer;
    return "?";
#endif
}

/*===========================================================================*/
/* Timer Processing (called from tick handler)                                */
/*===========================================================================*/

void mr_timer_process(void)
{
    mr_timer_t *timer;
    mr_list_node_t *node;

    if (!g_timer_list_initialized) {
        return;
    }

    /* Process all expired timers */
    while (!mr_list_is_empty(&g_timer_list)) {
        node = mr_list_peek_head(&g_timer_list);
        timer = (mr_timer_t *)node->container;

        /* Check if timer has expired */
        if ((int32_t)(timer->expire_tick - g_tick_count) > 0) {
            /* Not yet expired - list is sorted, so we're done */
            break;
        }

        /* Remove from list */
        mr_list_remove_head(&g_timer_list);

        /* Mark as expired */
        timer->state = MR_TIMER_EXPIRED;

        /* Call callback */
        if (timer->callback != NULL) {
            timer->callback(timer);
        }

        /* Handle auto-reload */
        if (timer->auto_reload && timer->state == MR_TIMER_EXPIRED) {
            timer->expire_tick = g_tick_count + timer->period_ticks;
            timer->state = MR_TIMER_RUNNING;
            add_timer_to_list(timer);
        } else if (timer->state == MR_TIMER_EXPIRED) {
            timer->state = MR_TIMER_STOPPED;
        }
    }
}

#endif /* MR_USE_TIMERS */
