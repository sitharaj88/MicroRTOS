/**
 * MicroRTOS Example - Timers and Event Groups
 *
 * This example demonstrates:
 * - Software timers (one-shot and periodic)
 * - Event groups for multi-task synchronization
 * - Timer callbacks
 * - Using events for task coordination
 *
 * Scenario:
 * - Three worker tasks wait for their respective event bits
 * - A periodic timer sets event bits at regular intervals
 * - A coordinator task waits for all workers to complete
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "micrortos.h"

/*===========================================================================*/
/* Event Bits                                                                 */
/*===========================================================================*/

#define EVENT_WORKER1_START     (1 << 0)
#define EVENT_WORKER2_START     (1 << 1)
#define EVENT_WORKER3_START     (1 << 2)
#define EVENT_ALL_START         (EVENT_WORKER1_START | EVENT_WORKER2_START | EVENT_WORKER3_START)

#define EVENT_WORKER1_DONE      (1 << 3)
#define EVENT_WORKER2_DONE      (1 << 4)
#define EVENT_WORKER3_DONE      (1 << 5)
#define EVENT_ALL_DONE          (EVENT_WORKER1_DONE | EVENT_WORKER2_DONE | EVENT_WORKER3_DONE)

/*===========================================================================*/
/* Event Group and Timers                                                     */
/*===========================================================================*/

static mr_event_t task_events;

/* Periodic timer to trigger workers */
static mr_timer_t trigger_timer;

/* One-shot timer for timeout */
static mr_timer_t timeout_timer;

/* Statistics */
static volatile uint32_t cycles_completed = 0;

/*===========================================================================*/
/* Task Definitions                                                           */
/*===========================================================================*/

static mr_tcb_t worker1_tcb;
static uint8_t worker1_stack[192];

static mr_tcb_t worker2_tcb;
static uint8_t worker2_stack[192];

static mr_tcb_t worker3_tcb;
static uint8_t worker3_stack[192];

static mr_tcb_t coordinator_tcb;
static uint8_t coordinator_stack[256];

/*===========================================================================*/
/* Timer Callbacks                                                            */
/*===========================================================================*/

/**
 * Periodic timer callback - triggers all workers.
 */
static void trigger_timer_callback(mr_timer_t *timer)
{
    (void)timer;

    /* Set start events for all workers */
    mr_event_set(&task_events, EVENT_ALL_START);
}

/**
 * Timeout timer callback - called if workers take too long.
 */
static void timeout_timer_callback(mr_timer_t *timer)
{
    (void)timer;

    /* In a real application, you might:
     * - Log an error
     * - Reset the system
     * - Set an error flag
     */
}

/*===========================================================================*/
/* Worker Tasks                                                               */
/*===========================================================================*/

static void worker_task(void *arg)
{
    uint8_t worker_id = (uint8_t)(uintptr_t)arg;
    uint32_t start_bit = (1 << (worker_id - 1));
    uint32_t done_bit = (1 << (worker_id + 2));
    uint32_t events;

    while (1) {
        /* Wait for our start event */
        events = mr_event_wait(
            &task_events,
            start_bit,
            false,          /* Wait for any (just our bit) */
            true,           /* Clear on exit */
            MR_WAIT_FOREVER
        );

        if (events & start_bit) {
            /* Simulate work - duration varies by worker */
            mr_task_delay(MR_MS_TO_TICKS(50 * worker_id));

            /* Signal completion */
            mr_event_set(&task_events, done_bit);
        }
    }
}

/*===========================================================================*/
/* Coordinator Task                                                           */
/*===========================================================================*/

static void coordinator_task(void *arg)
{
    (void)arg;
    uint32_t events;

    /* Start the periodic trigger timer (every 500ms) */
    mr_timer_start(&trigger_timer);

    while (1) {
        /* Wait for all workers to complete */
        events = mr_event_wait(
            &task_events,
            EVENT_ALL_DONE,
            true,           /* Wait for ALL bits */
            true,           /* Clear on exit */
            MR_MS_TO_TICKS(1000)  /* 1 second timeout */
        );

        if (events == 0) {
            /* Timeout - workers didn't complete in time */
            /* Handle error */
        } else {
            /* All workers completed */
            cycles_completed++;

            /* Could do something with the result here */
        }
    }
}

/*===========================================================================*/
/* Main Entry Point                                                           */
/*===========================================================================*/

int main(void)
{
    /* Initialize the RTOS kernel */
    mr_kernel_init();

    /* Initialize event group */
    mr_event_init(&task_events);

    /* Initialize periodic trigger timer (500ms period) */
    mr_timer_init(
        &trigger_timer,
        "Trigger",
        MR_MS_TO_TICKS(500),
        true,   /* auto-reload (periodic) */
        trigger_timer_callback
    );

    /* Initialize one-shot timeout timer (2 second timeout) */
    mr_timer_init(
        &timeout_timer,
        "Timeout",
        MR_MS_TO_TICKS(2000),
        false,  /* one-shot */
        timeout_timer_callback
    );

    /* Create worker tasks (all same priority) */
    mr_task_create(
        &worker1_tcb,
        "Worker1",
        worker_task,
        (void *)1,
        3,
        worker1_stack,
        sizeof(worker1_stack)
    );

    mr_task_create(
        &worker2_tcb,
        "Worker2",
        worker_task,
        (void *)2,
        3,
        worker2_stack,
        sizeof(worker2_stack)
    );

    mr_task_create(
        &worker3_tcb,
        "Worker3",
        worker_task,
        (void *)3,
        3,
        worker3_stack,
        sizeof(worker3_stack)
    );

    /* Create coordinator (higher priority) */
    mr_task_create(
        &coordinator_tcb,
        "Coord",
        coordinator_task,
        NULL,
        2,
        coordinator_stack,
        sizeof(coordinator_stack)
    );

    /* Start the scheduler */
    mr_kernel_start();

    return 0;
}
