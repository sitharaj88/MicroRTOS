/**
 * MicroRTOS - EDF Deadline Scheduling Implementation
 *
 * Earliest Deadline First scheduler for hard real-time tasks.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos_deadline.h"
#include "rtos_types.h"
#include "rtos_list.h"

#if RTOS_USE_EDF_SCHEDULER

/*===========================================================================*/
/* External References                                                        */
/*===========================================================================*/

extern volatile uint32_t g_tick_count;
extern rtos_tcb_t *g_current_tcb;
extern rtos_list_t g_delayed_list;
extern uint32_t rtos_port_disable_interrupts(void);
extern void rtos_port_restore_interrupts(uint32_t state);
extern void rtos_scheduler_add_ready(rtos_tcb_t *tcb);

/*
 * Locally re-implement the "make ready" sequence used by the IPC and
 * sync modules. Centralizing this in the scheduler would be cleaner,
 * but keeping it here avoids touching the core scheduler surface.
 */
static void deadline_wake_task(rtos_tcb_t *tcb)
{
    if (tcb == NULL) {
        return;
    }
    if (rtos_list_node_is_linked(&tcb->state_node)) {
        rtos_list_remove(&g_delayed_list, &tcb->state_node);
    }
    tcb->state = RTOS_TASK_READY;
    tcb->block_reason = RTOS_BLOCK_NONE;
    tcb->blocked_on = NULL;
    rtos_scheduler_add_ready(tcb);
}

/*===========================================================================*/
/* Module State                                                               */
/*===========================================================================*/

static rtos_edf_scheduler_t edf_scheduler = {
    .task_count = 0,
    .enabled = false,
    .total_utilization = 0,
};

/* Storage for deadline task info */
static rtos_deadline_task_t deadline_tasks[RTOS_EDF_MAX_TASKS];

/*===========================================================================*/
/* Internal Functions                                                         */
/*===========================================================================*/

/**
 * Find deadline task entry for a TCB.
 */
static rtos_deadline_task_t *find_deadline_task(rtos_tcb_t *tcb)
{
    uint8_t i;

    if (tcb == NULL) {
        return NULL;
    }

    for (i = 0; i < RTOS_EDF_MAX_TASKS; i++) {
        if (deadline_tasks[i].tcb == tcb && deadline_tasks[i].enabled) {
            return &deadline_tasks[i];
        }
    }

    return NULL;
}

/**
 * Find free deadline task slot.
 */
static rtos_deadline_task_t *find_free_slot(void)
{
    uint8_t i;

    for (i = 0; i < RTOS_EDF_MAX_TASKS; i++) {
        if (!deadline_tasks[i].enabled) {
            return &deadline_tasks[i];
        }
    }

    return NULL;
}

/**
 * Calculate utilization contribution of a task.
 * Returns utilization * 1000 for fixed-point representation.
 */
static uint32_t calculate_utilization(uint32_t wcet, uint32_t period)
{
    if (period == 0) {
        return 1000;  /* 100% */
    }

    return (wcet * 1000) / period;
}

/**
 * Recalculate total system utilization.
 */
static void recalculate_utilization(void)
{
    uint8_t i;
    uint32_t total = 0;

    for (i = 0; i < RTOS_EDF_MAX_TASKS; i++) {
        if (deadline_tasks[i].enabled) {
            total += calculate_utilization(
                deadline_tasks[i].params.wcet_ticks,
                deadline_tasks[i].params.period_ticks
            );
        }
    }

    edf_scheduler.total_utilization = total;
}

/*===========================================================================*/
/* Deadline Scheduling API                                                    */
/*===========================================================================*/

void rtos_deadline_init(void)
{
    uint8_t i;

    for (i = 0; i < RTOS_EDF_MAX_TASKS; i++) {
        deadline_tasks[i].tcb = NULL;
        deadline_tasks[i].enabled = false;
        deadline_tasks[i].miss_callback = NULL;
        edf_scheduler.tasks[i] = NULL;
    }

    edf_scheduler.task_count = 0;
    edf_scheduler.enabled = true;
    edf_scheduler.total_utilization = 0;
}

rtos_status_t rtos_deadline_task_create(rtos_tcb_t *tcb,
                                         uint32_t period_ms,
                                         uint32_t deadline_ms,
                                         uint32_t wcet_ms)
{
    return rtos_deadline_task_create_ticks(
        tcb,
        RTOS_MS_TO_TICKS(period_ms),
        RTOS_MS_TO_TICKS(deadline_ms),
        RTOS_MS_TO_TICKS(wcet_ms)
    );
}

rtos_status_t rtos_deadline_task_create_ticks(rtos_tcb_t *tcb,
                                               uint32_t period_ticks,
                                               uint32_t deadline_ticks,
                                               uint32_t wcet_ticks)
{
    rtos_deadline_task_t *dt;
    uint32_t state;

    if (tcb == NULL || period_ticks == 0) {
        return RTOS_ERR_PARAM;
    }

    /* Deadline defaults to period if not specified */
    if (deadline_ticks == 0) {
        deadline_ticks = period_ticks;
    }

    /* WCET must be less than or equal to deadline */
    if (wcet_ticks > deadline_ticks) {
        return RTOS_ERR_PARAM;
    }

    state = rtos_port_disable_interrupts();

    /* Find or allocate slot */
    dt = find_deadline_task(tcb);
    if (dt == NULL) {
        dt = find_free_slot();
        if (dt == NULL) {
            rtos_port_restore_interrupts(state);
            return RTOS_ERR_NO_MEMORY;
        }
    }

    /* Initialize deadline parameters */
    dt->tcb = tcb;
    dt->params.period_ticks = period_ticks;
    dt->params.deadline_ticks = deadline_ticks;
    dt->params.wcet_ticks = wcet_ticks;
    dt->params.next_deadline = g_tick_count + deadline_ticks;
    dt->params.next_release = g_tick_count;
    dt->params.execution_time = 0;
    dt->params.job_count = 0;
    dt->params.deadline_misses = 0;
    dt->state = RTOS_DEADLINE_READY;
    dt->enabled = true;
    dt->miss_callback = NULL;

    /* Add to scheduler */
    if (edf_scheduler.task_count < RTOS_EDF_MAX_TASKS) {
        edf_scheduler.tasks[edf_scheduler.task_count++] = dt;
    }

    recalculate_utilization();

    rtos_port_restore_interrupts(state);
    return RTOS_OK;
}

void rtos_deadline_task_remove(rtos_tcb_t *tcb)
{
    rtos_deadline_task_t *dt;
    uint32_t state;
    uint8_t i, j;

    if (tcb == NULL) {
        return;
    }

    state = rtos_port_disable_interrupts();

    dt = find_deadline_task(tcb);
    if (dt != NULL) {
        dt->enabled = false;
        dt->tcb = NULL;

        /* Remove from scheduler array */
        for (i = 0; i < edf_scheduler.task_count; i++) {
            if (edf_scheduler.tasks[i] == dt) {
                /* Shift remaining tasks */
                for (j = i; j < edf_scheduler.task_count - 1; j++) {
                    edf_scheduler.tasks[j] = edf_scheduler.tasks[j + 1];
                }
                edf_scheduler.tasks[edf_scheduler.task_count - 1] = NULL;
                edf_scheduler.task_count--;
                break;
            }
        }

        recalculate_utilization();
    }

    rtos_port_restore_interrupts(state);
}

void rtos_deadline_job_complete(void)
{
    rtos_deadline_task_t *dt;
    uint32_t state;

    if (g_current_tcb == NULL) {
        return;
    }

    state = rtos_port_disable_interrupts();

    dt = find_deadline_task(g_current_tcb);
    if (dt != NULL && dt->state == RTOS_DEADLINE_RUNNING) {
        dt->params.job_count++;
        dt->state = RTOS_DEADLINE_COMPLETED;

        /* Set up next release */
        dt->params.next_release += dt->params.period_ticks;
        dt->params.next_deadline = dt->params.next_release +
                                    dt->params.deadline_ticks;
        dt->params.execution_time = 0;
    }

    rtos_port_restore_interrupts(state);
}

void rtos_deadline_set_miss_callback(rtos_tcb_t *tcb,
                                      void (*callback)(rtos_deadline_task_t *))
{
    rtos_deadline_task_t *dt = find_deadline_task(tcb);

    if (dt != NULL) {
        dt->miss_callback = callback;
    }
}

rtos_deadline_task_t *rtos_deadline_get_info(rtos_tcb_t *tcb)
{
    return find_deadline_task(tcb);
}

/*===========================================================================*/
/* Schedulability Analysis                                                    */
/*===========================================================================*/

bool rtos_deadline_admission_test(uint32_t period_ticks, uint32_t wcet_ticks)
{
    uint32_t new_util;
    uint32_t total_util;
    uint32_t bound;
    uint8_t n;

    if (period_ticks == 0) {
        return false;
    }

    new_util = calculate_utilization(wcet_ticks, period_ticks);
    total_util = edf_scheduler.total_utilization + new_util;
    n = edf_scheduler.task_count + 1;

    /*
     * Liu & Layland bound: U <= n(2^(1/n) - 1)
     * For EDF, the bound is simply U <= 1 (100%)
     * We use 1000 as fixed-point representation
     */
    if (n == 0) {
        bound = 1000;
    } else {
        /* EDF can achieve 100% utilization with implicit deadlines */
        bound = 1000;
    }

    return (total_util <= bound);
}

bool rtos_deadline_exact_test(uint32_t period_ticks,
                               uint32_t deadline_ticks,
                               uint32_t wcet_ticks)
{
    /* For EDF with D <= P, the test is simply U <= 1 */
    uint32_t new_util;
    uint32_t total_util;

    if (period_ticks == 0 || deadline_ticks > period_ticks) {
        return false;
    }

    new_util = calculate_utilization(wcet_ticks, period_ticks);
    total_util = edf_scheduler.total_utilization + new_util;

    return (total_util <= 1000);
}

uint8_t rtos_deadline_get_utilization(void)
{
    return (uint8_t)(edf_scheduler.total_utilization / 10);
}

uint32_t rtos_deadline_get_utilization_fp(void)
{
    return edf_scheduler.total_utilization;
}

/*===========================================================================*/
/* Scheduler Integration                                                      */
/*===========================================================================*/

rtos_tcb_t *rtos_deadline_select_task(void)
{
    rtos_deadline_task_t *earliest = NULL;
    uint32_t earliest_deadline = 0xFFFFFFFF;
    uint8_t i;

    if (!edf_scheduler.enabled || edf_scheduler.task_count == 0) {
        return NULL;
    }

    /* Find task with earliest deadline that is ready */
    for (i = 0; i < edf_scheduler.task_count; i++) {
        rtos_deadline_task_t *dt = edf_scheduler.tasks[i];

        if (dt == NULL || !dt->enabled) {
            continue;
        }

        if (dt->state == RTOS_DEADLINE_READY ||
            dt->state == RTOS_DEADLINE_RUNNING) {

            /* Check if task's TCB is ready to run */
            if (dt->tcb->state == RTOS_TASK_READY ||
                dt->tcb->state == RTOS_TASK_RUNNING) {

                if (dt->params.next_deadline < earliest_deadline) {
                    earliest_deadline = dt->params.next_deadline;
                    earliest = dt;
                }
            }
        }
    }

    if (earliest != NULL) {
        earliest->state = RTOS_DEADLINE_RUNNING;
        return earliest->tcb;
    }

    return NULL;
}

void rtos_deadline_tick(void)
{
    uint32_t current_tick;
    uint8_t i;

    if (!edf_scheduler.enabled) {
        return;
    }

    current_tick = g_tick_count;

    for (i = 0; i < edf_scheduler.task_count; i++) {
        rtos_deadline_task_t *dt = edf_scheduler.tasks[i];

        if (dt == NULL || !dt->enabled) {
            continue;
        }

        /* Check for periodic release */
        if (dt->state == RTOS_DEADLINE_COMPLETED) {
            if ((int32_t)(current_tick - dt->params.next_release) >= 0) {
                /* New period started - release job */
                dt->state = RTOS_DEADLINE_READY;
                dt->params.execution_time = 0;

                /* Wake up the task */
                deadline_wake_task(dt->tcb);
            }
        }

        /* Track execution time for running task */
        if (dt->state == RTOS_DEADLINE_RUNNING) {
            dt->params.execution_time++;
        }
    }

    /* Check for deadline misses */
    rtos_deadline_check_misses();
}

void rtos_deadline_check_misses(void)
{
    uint32_t current_tick;
    uint8_t i;

    current_tick = g_tick_count;

    for (i = 0; i < edf_scheduler.task_count; i++) {
        rtos_deadline_task_t *dt = edf_scheduler.tasks[i];

        if (dt == NULL || !dt->enabled) {
            continue;
        }

        /* Check if deadline has passed for active job */
        if (dt->state == RTOS_DEADLINE_READY ||
            dt->state == RTOS_DEADLINE_RUNNING) {

            if ((int32_t)(current_tick - dt->params.next_deadline) >= 0) {
                /* Deadline missed! */
                dt->params.deadline_misses++;
                dt->state = RTOS_DEADLINE_MISSED;

                /* Call miss callback if registered */
                if (dt->miss_callback != NULL) {
                    dt->miss_callback(dt);
                }

                /* Force completion and set up next period */
                dt->params.next_release += dt->params.period_ticks;
                dt->params.next_deadline = dt->params.next_release +
                                            dt->params.deadline_ticks;
                dt->state = RTOS_DEADLINE_COMPLETED;
            }
        }
    }
}

/*===========================================================================*/
/* Statistics and Monitoring                                                  */
/*===========================================================================*/

uint16_t rtos_deadline_get_miss_count(rtos_tcb_t *tcb)
{
    rtos_deadline_task_t *dt = find_deadline_task(tcb);
    return (dt != NULL) ? dt->params.deadline_misses : 0;
}

uint16_t rtos_deadline_get_job_count(rtos_tcb_t *tcb)
{
    rtos_deadline_task_t *dt = find_deadline_task(tcb);
    return (dt != NULL) ? dt->params.job_count : 0;
}

int32_t rtos_deadline_get_slack(rtos_tcb_t *tcb)
{
    rtos_deadline_task_t *dt = find_deadline_task(tcb);

    if (dt == NULL) {
        return 0;
    }

    return (int32_t)(dt->params.next_deadline - g_tick_count);
}

void rtos_deadline_reset_stats(rtos_tcb_t *tcb)
{
    rtos_deadline_task_t *dt = find_deadline_task(tcb);

    if (dt != NULL) {
        dt->params.job_count = 0;
        dt->params.deadline_misses = 0;
    }
}

void rtos_deadline_reset_all_stats(void)
{
    uint8_t i;

    for (i = 0; i < RTOS_EDF_MAX_TASKS; i++) {
        if (deadline_tasks[i].enabled) {
            deadline_tasks[i].params.job_count = 0;
            deadline_tasks[i].params.deadline_misses = 0;
        }
    }
}

/*===========================================================================*/
/* Timing Utilities                                                           */
/*===========================================================================*/

static uint32_t timing_start_tick = 0;

void rtos_deadline_start_timing(void)
{
    timing_start_tick = g_tick_count;
}

uint32_t rtos_deadline_stop_timing(void)
{
    return g_tick_count - timing_start_tick;
}

int32_t rtos_deadline_time_to_deadline(void)
{
    rtos_deadline_task_t *dt;

    if (g_current_tcb == NULL) {
        return 0;
    }

    dt = find_deadline_task(g_current_tcb);
    if (dt == NULL) {
        return 0;
    }

    return (int32_t)(dt->params.next_deadline - g_tick_count);
}

#else /* !RTOS_USE_EDF_SCHEDULER */

/* Stub implementations when EDF is disabled */
void rtos_deadline_init(void) {}

rtos_status_t rtos_deadline_task_create(rtos_tcb_t *tcb, uint32_t period_ms,
                                         uint32_t deadline_ms, uint32_t wcet_ms)
{
    (void)tcb; (void)period_ms; (void)deadline_ms; (void)wcet_ms;
    return RTOS_ERR_NOT_SUPPORTED;
}

rtos_status_t rtos_deadline_task_create_ticks(rtos_tcb_t *tcb,
                                               uint32_t period_ticks,
                                               uint32_t deadline_ticks,
                                               uint32_t wcet_ticks)
{
    (void)tcb; (void)period_ticks; (void)deadline_ticks; (void)wcet_ticks;
    return RTOS_ERR_NOT_SUPPORTED;
}

void rtos_deadline_task_remove(rtos_tcb_t *tcb) { (void)tcb; }
void rtos_deadline_job_complete(void) {}
rtos_deadline_task_t *rtos_deadline_get_info(rtos_tcb_t *tcb)
{
    (void)tcb;
    return NULL;
}

bool rtos_deadline_admission_test(uint32_t period, uint32_t wcet)
{
    (void)period; (void)wcet;
    return false;
}

uint8_t rtos_deadline_get_utilization(void) { return 0; }
rtos_tcb_t *rtos_deadline_select_task(void) { return NULL; }
void rtos_deadline_tick(void) {}

#endif /* RTOS_USE_EDF_SCHEDULER */
