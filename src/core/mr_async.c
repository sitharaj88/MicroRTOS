/**
 * MicroRTOS - Async/Await Implementation
 *
 * Runtime support for async/await pattern.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_async.h"
#include "mr_types.h"

/*===========================================================================*/
/* External References                                                        */
/*===========================================================================*/

extern volatile uint32_t mr_tick_count;
extern void mr_task_delay(uint32_t ticks);

/*===========================================================================*/
/* Async API Implementation                                                   */
/*===========================================================================*/

mr_async_status_t mr_async_run_blocking(mr_async_state_t *state,
                                             mr_async_func_t func,
                                             void *arg,
                                             uint32_t poll_ms)
{
    mr_async_status_t status;

    if (state == NULL || func == NULL) {
        return MR_ASYNC_ERROR;
    }

    /* Initialize if not already started */
    if (!(state->flags & MR_ASYNC_FLAG_STARTED)) {
        mr_async_init(state);
    }

    /* Poll until complete */
    do {
        status = func(state, arg);

        if (status == MR_ASYNC_PENDING && poll_ms > 0) {
            mr_task_delay(MR_MS_TO_TICKS(poll_ms));
        }
    } while (status == MR_ASYNC_PENDING);

    return status;
}

/*===========================================================================*/
/* Async Runner Implementation                                                */
/*===========================================================================*/

void mr_async_runner_init(mr_async_runner_t *runner)
{
    if (runner == NULL) {
        return;
    }

    runner->func = NULL;
    runner->arg = NULL;
    runner->active = false;
    mr_async_init(&runner->state);
}

void mr_async_runner_start(mr_async_runner_t *runner,
                              mr_async_func_t func,
                              void *arg)
{
    if (runner == NULL || func == NULL) {
        return;
    }

    /* Reset state for new function */
    mr_async_init(&runner->state);

    runner->func = func;
    runner->arg = arg;
    runner->active = true;
}

bool mr_async_runner_poll(mr_async_runner_t *runner)
{
    mr_async_status_t status;

    if (runner == NULL || !runner->active || runner->func == NULL) {
        return false;
    }

    /* Step the async function */
    status = runner->func(&runner->state, runner->arg);

    if (status != MR_ASYNC_PENDING) {
        runner->active = false;
    }

    return runner->active;
}

mr_async_status_t mr_async_runner_get_result(mr_async_runner_t *runner)
{
    if (runner == NULL) {
        return MR_ASYNC_ERROR;
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
bool mr_async_delay_elapsed(mr_async_state_t *state)
{
    int32_t diff = (int32_t)(mr_tick_count - state->wait_until);
    return (diff >= 0);
}

/**
 * Helper: Set delay target time.
 */
void mr_async_delay_set(mr_async_state_t *state, uint32_t ticks)
{
    state->wait_until = mr_tick_count + ticks;
}
