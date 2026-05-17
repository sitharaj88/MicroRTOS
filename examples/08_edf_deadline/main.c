/**
 * MicroRTOS Example - Earliest Deadline First (EDF) Scheduling
 *
 * Demonstrates registering periodic tasks with the EDF scheduler and
 * signalling job completion. Each task represents a hard real-time
 * activity with a period, deadline, and worst-case execution time.
 *
 * Hardware: Any Arduino board (LED on pin 13 shows liveness).
 *
 * Build with: make example8
 *   (requires -DRTOS_USE_EDF_SCHEDULER=1, already enabled in this example)
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#define MR_USE_EDF_SCHEDULER  1

#include "micrortos.h"
#include "mr_deadline.h"

#ifdef __AVR__
#include <avr/io.h>
#define LED_PIN     PB5
#define LED_DDR     DDRB
#define LED_PORT    PORTB
#define LED_TOGGLE() (LED_PORT ^= (1 << LED_PIN))
#else
#define LED_TOGGLE()
#endif

/*===========================================================================*/
/* Task Storage                                                               */
/*===========================================================================*/

static mr_tcb_t fast_tcb;
static uint8_t    fast_stack[160];

static mr_tcb_t slow_tcb;
static uint8_t    slow_stack[160];

static volatile uint16_t fast_jobs;
static volatile uint16_t slow_jobs;
static volatile uint16_t deadline_misses;

/*===========================================================================*/
/* Deadline miss handler                                                      */
/*===========================================================================*/

static void on_deadline_miss(mr_deadline_task_t *task)
{
    (void)task;
    deadline_misses++;
}

/*===========================================================================*/
/* Periodic Tasks                                                             */
/*===========================================================================*/

/*
 * Fast task: 50 ms period, 40 ms deadline, ~5 ms WCET.
 * Runs a short workload then declares the job complete; the EDF
 * scheduler holds it off until the next periodic release.
 */
static void fast_task(void *arg)
{
    (void)arg;
    while (1) {
        LED_TOGGLE();

        /* Simulate work shorter than the WCET. */
        mr_task_delay(MR_MS_TO_TICKS(3));

        fast_jobs++;
        mr_deadline_job_complete();
    }
}

/*
 * Slow task: 200 ms period, 200 ms deadline, ~20 ms WCET.
 */
static void slow_task(void *arg)
{
    (void)arg;
    while (1) {
        mr_task_delay(MR_MS_TO_TICKS(15));

        slow_jobs++;
        mr_deadline_job_complete();
    }
}

/*===========================================================================*/
/* Hardware                                                                   */
/*===========================================================================*/

static void hardware_init(void)
{
#ifdef __AVR__
    LED_DDR |= (1 << LED_PIN);
#endif
}

/*===========================================================================*/
/* main                                                                       */
/*===========================================================================*/

int main(void)
{
    hardware_init();
    mr_kernel_init();
    mr_deadline_init();

    mr_task_create(&fast_tcb, "Fast", fast_task, NULL, 2,
                     fast_stack, sizeof(fast_stack));
    mr_task_create(&slow_tcb, "Slow", slow_task, NULL, 2,
                     slow_stack, sizeof(slow_stack));

    /* Register periodic timing parameters with the EDF scheduler. */
    mr_deadline_task_create(&fast_tcb, /*period*/50,  /*deadline*/40,  /*wcet*/5);
    mr_deadline_task_create(&slow_tcb, /*period*/200, /*deadline*/200, /*wcet*/20);

    mr_deadline_set_miss_callback(&fast_tcb, on_deadline_miss);
    mr_deadline_set_miss_callback(&slow_tcb, on_deadline_miss);

    mr_kernel_start();
    return 0;
}
