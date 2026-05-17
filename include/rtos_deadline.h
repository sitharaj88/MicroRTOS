/**
 * MicroRTOS - Deadline Scheduling (EDF)
 *
 * Earliest Deadline First scheduling for hard real-time tasks.
 * Supports mixed-mode scheduling with fixed-priority tasks.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef RTOS_DEADLINE_H
#define RTOS_DEADLINE_H

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

#ifndef RTOS_USE_EDF_SCHEDULER
#define RTOS_USE_EDF_SCHEDULER          0
#endif

#ifndef RTOS_EDF_MAX_TASKS
#define RTOS_EDF_MAX_TASKS              8
#endif

#ifndef RTOS_EDF_MIXED_MODE
#define RTOS_EDF_MIXED_MODE             1   /* Allow mixing EDF and fixed-priority */
#endif

/*===========================================================================*/
/* Deadline Task Parameters                                                   */
/*===========================================================================*/

/**
 * Deadline task timing parameters.
 */
typedef struct rtos_deadline_params {
    uint32_t period_ticks;          /* Task period (time between jobs) */
    uint32_t deadline_ticks;        /* Relative deadline from job start */
    uint32_t wcet_ticks;            /* Worst-case execution time */
    uint32_t next_deadline;         /* Absolute deadline for current job */
    uint32_t next_release;          /* Next periodic release time */
    uint32_t execution_time;        /* Execution time of current job */
    uint16_t job_count;             /* Number of completed jobs */
    uint16_t deadline_misses;       /* Number of deadline misses */
} rtos_deadline_params_t;

/*===========================================================================*/
/* Deadline Task State                                                        */
/*===========================================================================*/

typedef enum {
    RTOS_DEADLINE_IDLE = 0,         /* No active job */
    RTOS_DEADLINE_READY,            /* Job ready to execute */
    RTOS_DEADLINE_RUNNING,          /* Job currently executing */
    RTOS_DEADLINE_COMPLETED,        /* Job completed, waiting for next period */
    RTOS_DEADLINE_MISSED,           /* Deadline was missed */
} rtos_deadline_state_t;

/*===========================================================================*/
/* Deadline Task Entry                                                        */
/*===========================================================================*/

typedef struct rtos_deadline_task {
    rtos_tcb_t              *tcb;           /* Associated task */
    rtos_deadline_params_t  params;         /* Timing parameters */
    rtos_deadline_state_t   state;          /* Current job state */
    bool                    enabled;        /* Deadline scheduling enabled */
    void (*miss_callback)(struct rtos_deadline_task *);  /* Deadline miss handler */
} rtos_deadline_task_t;

/*===========================================================================*/
/* EDF Scheduler State                                                        */
/*===========================================================================*/

typedef struct {
    rtos_deadline_task_t    *tasks[RTOS_EDF_MAX_TASKS];
    uint8_t                 task_count;
    bool                    enabled;
    uint32_t                total_utilization;  /* Fixed-point U*1000 */
} rtos_edf_scheduler_t;

/*===========================================================================*/
/* Deadline Scheduling API                                                    */
/*===========================================================================*/

/**
 * Initialize the EDF scheduler.
 */
void rtos_deadline_init(void);

/**
 * Register a task for EDF scheduling.
 *
 * @param tcb               Task to register
 * @param period_ms         Period in milliseconds
 * @param deadline_ms       Relative deadline in milliseconds
 * @param wcet_ms           Worst-case execution time in milliseconds
 * @return                  RTOS_OK on success
 */
rtos_status_t rtos_deadline_task_create(rtos_tcb_t *tcb,
                                         uint32_t period_ms,
                                         uint32_t deadline_ms,
                                         uint32_t wcet_ms);

/**
 * Register a task with tick-based timing.
 *
 * @param tcb               Task to register
 * @param period_ticks      Period in ticks
 * @param deadline_ticks    Relative deadline in ticks
 * @param wcet_ticks        Worst-case execution time in ticks
 * @return                  RTOS_OK on success
 */
rtos_status_t rtos_deadline_task_create_ticks(rtos_tcb_t *tcb,
                                               uint32_t period_ticks,
                                               uint32_t deadline_ticks,
                                               uint32_t wcet_ticks);

/**
 * Remove a task from EDF scheduling.
 *
 * @param tcb               Task to remove
 */
void rtos_deadline_task_remove(rtos_tcb_t *tcb);

/**
 * Signal job completion.
 * Must be called by the task when it finishes its periodic work.
 */
void rtos_deadline_job_complete(void);

/**
 * Set deadline miss callback for a task.
 *
 * @param tcb               Task to configure
 * @param callback          Function to call on deadline miss
 */
void rtos_deadline_set_miss_callback(rtos_tcb_t *tcb,
                                      void (*callback)(rtos_deadline_task_t *));

/**
 * Get deadline task information.
 *
 * @param tcb               Task to query
 * @return                  Pointer to deadline info, or NULL
 */
rtos_deadline_task_t *rtos_deadline_get_info(rtos_tcb_t *tcb);

/*===========================================================================*/
/* Schedulability Analysis                                                    */
/*===========================================================================*/

/**
 * Perform admission test for a new deadline task.
 * Uses the Liu & Layland bound: U <= n(2^(1/n) - 1)
 *
 * @param period_ticks      Period of new task
 * @param wcet_ticks        WCET of new task
 * @return                  true if schedulable
 */
bool rtos_deadline_admission_test(uint32_t period_ticks, uint32_t wcet_ticks);

/**
 * Perform exact schedulability test.
 * Uses processor demand analysis.
 *
 * @param period_ticks      Period of new task
 * @param deadline_ticks    Deadline of new task
 * @param wcet_ticks        WCET of new task
 * @return                  true if schedulable
 */
bool rtos_deadline_exact_test(uint32_t period_ticks,
                               uint32_t deadline_ticks,
                               uint32_t wcet_ticks);

/**
 * Get total system utilization.
 *
 * @return                  Utilization as percentage (0-100)
 */
uint8_t rtos_deadline_get_utilization(void);

/**
 * Get utilization as fixed-point (multiply by 1000).
 *
 * @return                  Utilization * 1000 (e.g., 750 = 75%)
 */
uint32_t rtos_deadline_get_utilization_fp(void);

/*===========================================================================*/
/* Scheduler Integration                                                      */
/*===========================================================================*/

/**
 * Select the next EDF task to run.
 * Called by the main scheduler.
 *
 * @return                  TCB of highest priority EDF task, or NULL
 */
rtos_tcb_t *rtos_deadline_select_task(void);

/**
 * Update deadline scheduler on tick.
 * Handles periodic releases and deadline checking.
 */
void rtos_deadline_tick(void);

/**
 * Check for deadline misses and handle them.
 */
void rtos_deadline_check_misses(void);

/*===========================================================================*/
/* Statistics and Monitoring                                                  */
/*===========================================================================*/

/**
 * Get number of deadline misses for a task.
 *
 * @param tcb               Task to query
 * @return                  Number of deadline misses
 */
uint16_t rtos_deadline_get_miss_count(rtos_tcb_t *tcb);

/**
 * Get number of completed jobs for a task.
 *
 * @param tcb               Task to query
 * @return                  Number of completed jobs
 */
uint16_t rtos_deadline_get_job_count(rtos_tcb_t *tcb);

/**
 * Get slack time until next deadline.
 *
 * @param tcb               Task to query
 * @return                  Slack in ticks (negative if overdue)
 */
int32_t rtos_deadline_get_slack(rtos_tcb_t *tcb);

/**
 * Reset statistics for a task.
 *
 * @param tcb               Task to reset
 */
void rtos_deadline_reset_stats(rtos_tcb_t *tcb);

/**
 * Reset all deadline statistics.
 */
void rtos_deadline_reset_all_stats(void);

/*===========================================================================*/
/* Timing Utilities                                                           */
/*===========================================================================*/

/**
 * Start measuring execution time for current job.
 */
void rtos_deadline_start_timing(void);

/**
 * Stop measuring execution time.
 *
 * @return                  Execution time in ticks
 */
uint32_t rtos_deadline_stop_timing(void);

/**
 * Get remaining time until deadline.
 *
 * @return                  Ticks until deadline (negative if missed)
 */
int32_t rtos_deadline_time_to_deadline(void);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_DEADLINE_H */
