/**
 * MicroRTOS Example - Basic Tasks
 *
 * This example demonstrates:
 * - Creating multiple tasks with different priorities
 * - Using task delays for periodic execution
 * - LED blinking with two independent tasks
 *
 * Hardware: Arduino Uno or similar
 * - LED on pin 13 (built-in)
 * - Optional: LED on pin 12
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "micrortos.h"

#ifdef __AVR__
#include <avr/io.h>
#define LED_PIN     PB5     /* Pin 13 on Arduino Uno */
#define LED2_PIN    PB4     /* Pin 12 on Arduino Uno */
#define LED_DDR     DDRB
#define LED_PORT    PORTB
#endif

/*===========================================================================*/
/* Task Definitions                                                           */
/*===========================================================================*/

/* Task 1: Blinks LED at 500ms interval */
static mr_tcb_t task1_tcb;
static uint8_t task1_stack[128];

/* Task 2: Blinks LED2 at 250ms interval (higher priority) */
static mr_tcb_t task2_tcb;
static uint8_t task2_stack[128];

/*===========================================================================*/
/* Task Functions                                                             */
/*===========================================================================*/

/**
 * Task 1: Slow blink on LED (pin 13)
 */
static void task1_func(void *arg)
{
    (void)arg;

    while (1) {
        /* Toggle LED */
#ifdef __AVR__
        LED_PORT ^= (1 << LED_PIN);
#endif

        /* Delay 500ms */
        mr_task_delay(MR_MS_TO_TICKS(500));
    }
}

/**
 * Task 2: Fast blink on LED2 (pin 12)
 */
static void task2_func(void *arg)
{
    (void)arg;

    while (1) {
        /* Toggle LED2 */
#ifdef __AVR__
        LED_PORT ^= (1 << LED2_PIN);
#endif

        /* Delay 250ms */
        mr_task_delay(MR_MS_TO_TICKS(250));
    }
}

/*===========================================================================*/
/* Hardware Setup                                                             */
/*===========================================================================*/

static void hardware_init(void)
{
#ifdef __AVR__
    /* Set LED pins as outputs */
    LED_DDR |= (1 << LED_PIN) | (1 << LED2_PIN);

    /* Start with LEDs off */
    LED_PORT &= ~((1 << LED_PIN) | (1 << LED2_PIN));
#endif
}

/*===========================================================================*/
/* Main Entry Point                                                           */
/*===========================================================================*/

int main(void)
{
    /* Initialize hardware */
    hardware_init();

    /* Initialize the RTOS kernel */
    mr_kernel_init();

    /* Create Task 1: Slow blink, priority 2 (lower) */
    mr_task_create(
        &task1_tcb,
        "Blink1",
        task1_func,
        NULL,
        2,              /* Priority 2 */
        task1_stack,
        sizeof(task1_stack)
    );

    /* Create Task 2: Fast blink, priority 1 (higher) */
    mr_task_create(
        &task2_tcb,
        "Blink2",
        task2_func,
        NULL,
        1,              /* Priority 1 (higher) */
        task2_stack,
        sizeof(task2_stack)
    );

    /* Start the scheduler - this function never returns */
    mr_kernel_start();

    /* Should never reach here */
    return 0;
}
