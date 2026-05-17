/**
 * MicroRTOS - Async/Await Implementation
 *
 * Runtime support for async/await pattern.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos_async.h"
#include "rtos_types.h"

/*===========================================================================*/
/* External References                                                        */
/*===========================================================================*/

extern volatile uint32_t rtos_tick_count;
extern void rtos_task_delay(uint32_t ticks);

/*===========================================================================*/
/* Async API Implementation                                                   */
/*===========================================================================*/

rtos_async_status_t rtos_async_run_blocking(rtos_async_state_t *state,
                                             rtos_async_func_t func,
                                             void *arg,
                                             uint32_t poll_ms)
{
    rtos_async_status_t status;

    if (state == NULL || func == NULL) {
        return RTOS_ASYNC_ERROR;
    }

    /* Initialize if not already started */
    if (!(state->flags & RTOS_ASYNC_FLAG_STARTED)) {
        rtos_async_init(state);
    }

    /* Poll until complete */
    do {
        status = func(state, arg);

        if (status == RTOS_ASYNC_PENDING && poll_ms > 0) {
            rtos_task_delay(RTOS_MS_TO_TICKS(poll_ms));
        }
    } while (status == RTOS_ASYNC_PENDING);

    return status;
}

/*===========================================================================*/
/* Async Runner Implementation                                                */
/*===========================================================================*/

void rtos_async_runner_init(rtos_async_runner_t *runner)
{
    if (runner == NULL) {
        return;
    }

    runner->func = NULL;
    runner->arg = NULL;
    runner->active = false;
    rtos_async_init(&runner->state);
}

void rtos_async_runner_start(rtos_async_runner_t *runner,
                              rtos_async_func_t func,
                              void *arg)
{
    if (runner == NULL || func == NULL) {
        return;
    }

    /* Reset state for new function */
    rtos_async_init(&runner->state);

    runner->func = func;
    runner->arg = arg;
    runner->active = true;
}

bool rtos_async_runner_poll(rtos_async_runner_t *runner)
{
    rtos_async_status_t status;

    if (runner == NULL || !runner->active || runner->func == NULL) {
        return false;
    }

    /* Step the async function */
    status = runner->func(&runner->state, runner->arg);

    if (status != RTOS_ASYNC_PENDING) {
        runner->active = false;
    }

    return runner->active;
}

rtos_async_status_t rtos_async_runner_get_result(rtos_async_runner_t *runner)
{
    if (runner == NULL) {
        return RTOS_ASYNC_ERROR;
    }

    return runner->state.result;
}

/*===========================================================================*/
/* Utility Functions                                                          */
/*===========================================================================*/

/**
 * Helper: Check if delay has elapsed.
 * Used internally by ASYNC_DELAY macros.
 */
bool rtos_async_delay_elapsed(rtos_async_state_t *state)
{
    int32_t diff = (int32_t)(rtos_tick_count - state->wait_until);
    return (diff >= 0);
}

/**
 * Helper: Set delay target time.
 */
void rtos_async_delay_set(rtos_async_state_t *state, uint32_t ticks)
{
    state->wait_until = rtos_tick_count + ticks;
}
