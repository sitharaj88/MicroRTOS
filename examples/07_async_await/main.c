/**
 * MicroRTOS Example - Async / Await
 *
 * Demonstrates a coroutine-style state machine running cooperatively
 * inside a single RTOS task. The async function yields between steps
 * without blocking the task, which is useful for I/O sequences and
 * polling loops.
 *
 * Hardware: Any Arduino board (uses pin 13 LED for activity).
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "micrortos.h"
#include "mr_async.h"

#ifdef __AVR__
#include <avr/io.h>
#define LED_PIN     PB5
#define LED_DDR     DDRB
#define LED_PORT    PORTB
#define LED_TOGGLE() (LED_PORT ^= (1 << LED_PIN))
#define LED_ON()     (LED_PORT |= (1 << LED_PIN))
#define LED_OFF()    (LED_PORT &= ~(1 << LED_PIN))
#else
#define LED_TOGGLE()
#define LED_ON()
#define LED_OFF()
#endif

/*===========================================================================*/
/* Async State Machine                                                        */
/*===========================================================================*/

/*
 * A four-step blink pattern driven by an async state machine.
 * Step 1: turn LED on for 100 ms
 * Step 2: turn LED off for 100 ms
 * Step 3: turn LED on for 400 ms
 * Step 4: turn LED off for 400 ms
 * Then the function completes; the runner restarts it.
 */
static mr_async_status_t blink_pattern(mr_async_state_t *state, void *arg)
{
    (void)arg;

    ASYNC_BEGIN(state);

        LED_ON();
        ASYNC_DELAY_MS(state, 100);

        LED_OFF();
        ASYNC_DELAY_MS(state, 100);

        LED_ON();
        ASYNC_DELAY_MS(state, 400);

        LED_OFF();
        ASYNC_DELAY_MS(state, 400);

    ASYNC_END(state);
}

/*===========================================================================*/
/* Task driving the async runner                                              */
/*===========================================================================*/

static mr_tcb_t runner_tcb;
static uint8_t runner_stack[192];

static volatile uint32_t cycles_completed = 0;

static void runner_task(void *arg)
{
    (void)arg;

    mr_async_state_t state;
    mr_async_init(&state);

    while (1) {
        mr_async_status_t s = blink_pattern(&state, NULL);

        if (s == MR_ASYNC_DONE) {
            cycles_completed++;
            mr_async_reset(&state);
        }

        /* Step the state machine at 10 ms granularity. */
        mr_task_delay(MR_MS_TO_TICKS(10));
    }
}

/*===========================================================================*/
/* Hardware                                                                   */
/*===========================================================================*/

static void hardware_init(void)
{
#ifdef __AVR__
    LED_DDR |= (1 << LED_PIN);
    LED_OFF();
#endif
}

/*===========================================================================*/
/* main                                                                       */
/*===========================================================================*/

int main(void)
{
    hardware_init();
    mr_kernel_init();

    mr_task_create(&runner_tcb,
                     "AsyncRunner",
                     runner_task,
                     NULL,
                     2,
                     runner_stack,
                     sizeof(runner_stack));

    mr_kernel_start();
    return 0;
}
