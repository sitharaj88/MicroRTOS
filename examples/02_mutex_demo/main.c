/**
 * MicroRTOS Example - Mutex with Priority Inheritance
 *
 * This example demonstrates:
 * - Mutex for protecting shared resources
 * - Priority inheritance to prevent priority inversion
 * - Three tasks showing the classic priority inversion scenario
 *
 * Scenario:
 * - Low priority task acquires mutex
 * - High priority task blocks on mutex
 * - Without priority inheritance, medium priority task would preempt low
 *   priority task indefinitely, blocking the high priority task
 * - With priority inheritance, low priority task is boosted to high
 *   priority until it releases the mutex
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos.h"

/*===========================================================================*/
/* Shared Resource                                                            */
/*===========================================================================*/

static rtos_mutex_t resource_mutex;
static volatile uint32_t shared_resource = 0;

/*===========================================================================*/
/* Task Definitions                                                           */
/*===========================================================================*/

/* Low priority task */
static rtos_tcb_t low_prio_tcb;
static uint8_t low_prio_stack[192];

/* Medium priority task */
static rtos_tcb_t med_prio_tcb;
static uint8_t med_prio_stack[192];

/* High priority task */
static rtos_tcb_t high_prio_tcb;
static uint8_t high_prio_stack[192];

/* Semaphore to sync task startup */
static rtos_sem_t start_sem;

/*===========================================================================*/
/* Low Priority Task                                                          */
/*===========================================================================*/

static void low_priority_task(void *arg)
{
    (void)arg;

    while (1) {
        /* Acquire the mutex */
        rtos_mutex_lock(&resource_mutex, RTOS_WAIT_FOREVER);

        /* Signal that we have the mutex */
        rtos_sem_give(&start_sem);

        /* Simulate work while holding mutex */
        for (volatile int i = 0; i < 10000; i++) {
            shared_resource++;
        }

        /* Release the mutex */
        rtos_mutex_unlock(&resource_mutex);

        /* Wait before next iteration */
        rtos_task_delay(RTOS_MS_TO_TICKS(500));
    }
}

/*===========================================================================*/
/* Medium Priority Task                                                       */
/*===========================================================================*/

static void medium_priority_task(void *arg)
{
    (void)arg;

    /* Wait for low priority task to acquire mutex */
    rtos_sem_take(&start_sem, RTOS_WAIT_FOREVER);

    while (1) {
        /*
         * This task has higher priority than the low priority task
         * but doesn't need the mutex.
         *
         * Without priority inheritance:
         * - This task would preempt the low priority task
         * - The high priority task would be blocked indefinitely
         *
         * With priority inheritance:
         * - Low priority task is boosted to high priority
         * - This task doesn't preempt until mutex is released
         */

        /* Simulate CPU-intensive work */
        for (volatile int i = 0; i < 5000; i++) {
            /* Busy work */
        }

        /* Yield to other tasks */
        rtos_task_delay(RTOS_MS_TO_TICKS(100));
    }
}

/*===========================================================================*/
/* High Priority Task                                                         */
/*===========================================================================*/

static void high_priority_task(void *arg)
{
    (void)arg;

    /* Wait for low priority task to acquire mutex first */
    rtos_sem_take(&start_sem, RTOS_WAIT_FOREVER);

    /* Give the semaphore back for medium priority task */
    rtos_sem_give(&start_sem);

    /* Small delay to let medium priority task start */
    rtos_task_delay(RTOS_MS_TO_TICKS(10));

    while (1) {
        /*
         * Try to acquire the mutex.
         * This will trigger priority inheritance on the current holder.
         */
        rtos_mutex_lock(&resource_mutex, RTOS_WAIT_FOREVER);

        /* Access the shared resource */
        shared_resource += 100;

        /* Release immediately */
        rtos_mutex_unlock(&resource_mutex);

        /* Wait before next iteration */
        rtos_task_delay(RTOS_MS_TO_TICKS(200));
    }
}

/*===========================================================================*/
/* Main Entry Point                                                           */
/*===========================================================================*/

int main(void)
{
    /* Initialize the RTOS kernel */
    rtos_kernel_init();

    /* Initialize the mutex */
    rtos_mutex_init(&resource_mutex);

    /* Initialize synchronization semaphore */
    rtos_sem_init(&start_sem, 0, 2);

    /* Create low priority task (priority 4) */
    rtos_task_create(
        &low_prio_tcb,
        "LowPrio",
        low_priority_task,
        NULL,
        4,  /* Lowest priority of the three */
        low_prio_stack,
        sizeof(low_prio_stack)
    );

    /* Create medium priority task (priority 3) */
    rtos_task_create(
        &med_prio_tcb,
        "MedPrio",
        medium_priority_task,
        NULL,
        3,  /* Medium priority */
        med_prio_stack,
        sizeof(med_prio_stack)
    );

    /* Create high priority task (priority 2) */
    rtos_task_create(
        &high_prio_tcb,
        "HighPrio",
        high_priority_task,
        NULL,
        2,  /* Highest priority of the three */
        high_prio_stack,
        sizeof(high_prio_stack)
    );

    /* Start the scheduler */
    rtos_kernel_start();

    return 0;
}
