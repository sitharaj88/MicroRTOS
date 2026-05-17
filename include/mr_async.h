/**
 * MicroRTOS - Async/Await Style API
 *
 * Coroutine-like programming pattern using Duff's device technique.
 * Enables cooperative multitasking within a single task or state machines.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef MR_ASYNC_H
#define MR_ASYNC_H

#include "mr_config.h"
#include "mr_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Async Status Codes                                                         */
/*===========================================================================*/

typedef enum {
    MR_ASYNC_PENDING = 0,     /* Not yet complete, call again */
    MR_ASYNC_DONE = 1,        /* Completed successfully */
    MR_ASYNC_ERROR = -1,      /* Completed with error */
    MR_ASYNC_TIMEOUT = -2,    /* Operation timed out */
    MR_ASYNC_CANCELLED = -3,  /* Operation cancelled */
} mr_async_status_t;

/*===========================================================================*/
/* Async State Structure                                                      */
/*===========================================================================*/

/**
 * Async function state.
 * Preserves the continuation point and local data between calls.
 */
typedef struct mr_async_state {
    uint16_t            line;           /* Continuation line number */
    uint16_t            flags;          /* State flags */
    uint32_t            wait_until;     /* Tick for timeout/delay */
    void                *local_data;    /* Pointer to preserved locals */
    mr_async_status_t result;         /* Operation result */
    int32_t             error_code;     /* Error code if result is ERROR */
} mr_async_state_t;

/* State flags */
#define MR_ASYNC_FLAG_STARTED     0x0001
#define MR_ASYNC_FLAG_WAITING     0x0002
#define MR_ASYNC_FLAG_CANCELLED   0x0004

/*===========================================================================*/
/* Async Function Type                                                        */
/*===========================================================================*/

/**
 * Async function signature.
 */
typedef mr_async_status_t (*mr_async_func_t)(mr_async_state_t *state,
                                                  void *arg);

/*===========================================================================*/
/* Core Async Macros                                                          */
/*===========================================================================*/

/**
 * Begin an async function block.
 * Must be placed at the start of the function body.
 */
#define ASYNC_BEGIN(state) \
    do { \
        (state)->flags |= MR_ASYNC_FLAG_STARTED; \
        switch ((state)->line) { \
            case 0:

/**
 * End an async function block.
 * Returns MR_ASYNC_DONE and resets state.
 */
#define ASYNC_END(state) \
        } \
        (state)->line = 0; \
        (state)->result = MR_ASYNC_DONE; \
        return MR_ASYNC_DONE; \
    } while (0)

/**
 * Yield execution and resume at this point next call.
 */
#define ASYNC_YIELD(state) \
    do { \
        (state)->line = __LINE__; \
        return MR_ASYNC_PENDING; \
        case __LINE__: ; \
    } while (0)

/**
 * Wait until a condition becomes true.
 */
#define ASYNC_WAIT_UNTIL(state, cond) \
    do { \
        (state)->line = __LINE__; \
        case __LINE__: \
            if (!(cond)) { \
                return MR_ASYNC_PENDING; \
            } \
    } while (0)

/**
 * Wait while a condition is true.
 */
#define ASYNC_WAIT_WHILE(state, cond) \
    ASYNC_WAIT_UNTIL(state, !(cond))

/*===========================================================================*/
/* Delay and Timeout Macros                                                   */
/*===========================================================================*/

/**
 * Delay for a specified number of milliseconds.
 */
#define ASYNC_DELAY_MS(state, ms) \
    do { \
        (state)->wait_until = mr_tick_get() + MR_MS_TO_TICKS(ms); \
        (state)->line = __LINE__; \
        case __LINE__: \
            if ((int32_t)(mr_tick_get() - (state)->wait_until) < 0) { \
                return MR_ASYNC_PENDING; \
            } \
    } while (0)

/**
 * Delay for a specified number of ticks.
 */
#define ASYNC_DELAY_TICKS(state, ticks) \
    do { \
        (state)->wait_until = mr_tick_get() + (ticks); \
        (state)->line = __LINE__; \
        case __LINE__: \
            if ((int32_t)(mr_tick_get() - (state)->wait_until) < 0) { \
                return MR_ASYNC_PENDING; \
            } \
    } while (0)

/**
 * Wait for condition with timeout (milliseconds).
 * Sets result to MR_ASYNC_TIMEOUT if timeout expires.
 */
#define ASYNC_WAIT_TIMEOUT_MS(state, cond, timeout_ms) \
    do { \
        (state)->wait_until = mr_tick_get() + MR_MS_TO_TICKS(timeout_ms); \
        (state)->line = __LINE__; \
        case __LINE__: \
            if (cond) { \
                break; \
            } \
            if ((int32_t)(mr_tick_get() - (state)->wait_until) >= 0) { \
                (state)->result = MR_ASYNC_TIMEOUT; \
                goto _async_exit_##__LINE__; \
            } \
            return MR_ASYNC_PENDING; \
        _async_exit_##__LINE__: ; \
    } while (0)

/*===========================================================================*/
/* Nested Async Calls                                                         */
/*===========================================================================*/

/**
 * Call another async function and wait for it to complete.
 */
#define ASYNC_CALL(state, func, child_state, arg) \
    do { \
        mr_async_status_t _status; \
        (state)->line = __LINE__; \
        case __LINE__: \
            _status = func((child_state), (arg)); \
            if (_status == MR_ASYNC_PENDING) { \
                return MR_ASYNC_PENDING; \
            } \
            (state)->result = _status; \
    } while (0)

/**
 * Spawn multiple async functions and wait for all to complete.
 * Requires array of states and functions.
 */
#define ASYNC_JOIN(state, funcs, states, count, arg) \
    do { \
        bool _all_done; \
        int _i; \
        (state)->line = __LINE__; \
        case __LINE__: \
            _all_done = true; \
            for (_i = 0; _i < (count); _i++) { \
                if ((states)[_i].result == MR_ASYNC_PENDING) { \
                    mr_async_status_t _s = (funcs)[_i](&(states)[_i], (arg)); \
                    if (_s == MR_ASYNC_PENDING) { \
                        _all_done = false; \
                    } \
                } \
            } \
            if (!_all_done) { \
                return MR_ASYNC_PENDING; \
            } \
    } while (0)

/*===========================================================================*/
/* Control Flow                                                               */
/*===========================================================================*/

/**
 * Exit the async function with a specific result.
 */
#define ASYNC_EXIT(state, status) \
    do { \
        (state)->line = 0; \
        (state)->result = (status); \
        return (status); \
    } while (0)

/**
 * Exit with error and error code.
 */
#define ASYNC_ERROR(state, code) \
    do { \
        (state)->error_code = (code); \
        ASYNC_EXIT(state, MR_ASYNC_ERROR); \
    } while (0)

/**
 * Check if cancelled and exit if so.
 */
#define ASYNC_CHECK_CANCELLED(state) \
    do { \
        if ((state)->flags & MR_ASYNC_FLAG_CANCELLED) { \
            ASYNC_EXIT(state, MR_ASYNC_CANCELLED); \
        } \
    } while (0)

/*===========================================================================*/
/* Local Variable Preservation                                                */
/*===========================================================================*/

/**
 * Declare preserved local variables structure.
 * Use at the start of async functions that need to preserve locals.
 *
 * Example:
 *   ASYNC_LOCALS(state, struct { int counter; char buffer[32]; });
 *   locals->counter = 0;
 */
#define ASYNC_LOCALS(state, type) \
    type *locals = (type *)(state)->local_data

/**
 * Allocate and initialize locals structure.
 * Call once before running the async function.
 */
#define ASYNC_LOCALS_ALLOC(state, type) \
    do { \
        static type _locals; \
        (state)->local_data = &_locals; \
    } while (0)

/*===========================================================================*/
/* Async API Functions                                                        */
/*===========================================================================*/

/**
 * Initialize an async state.
 *
 * @param state     State to initialize
 */
static inline void mr_async_init(mr_async_state_t *state)
{
    state->line = 0;
    state->flags = 0;
    state->wait_until = 0;
    state->local_data = NULL;
    state->result = MR_ASYNC_PENDING;
    state->error_code = 0;
}

/**
 * Check if async function is still running.
 *
 * @param state     State to check
 * @return          true if pending, false if complete
 */
static inline bool mr_async_is_pending(mr_async_state_t *state)
{
    return (state->result == MR_ASYNC_PENDING);
}

/**
 * Check if async function completed successfully.
 *
 * @param state     State to check
 * @return          true if done successfully
 */
static inline bool mr_async_is_done(mr_async_state_t *state)
{
    return (state->result == MR_ASYNC_DONE);
}

/**
 * Request cancellation of async function.
 *
 * @param state     State to cancel
 */
static inline void mr_async_cancel(mr_async_state_t *state)
{
    state->flags |= MR_ASYNC_FLAG_CANCELLED;
}

/**
 * Reset state to allow re-running.
 *
 * @param state     State to reset
 */
static inline void mr_async_reset(mr_async_state_t *state)
{
    state->line = 0;
    state->flags = 0;
    state->result = MR_ASYNC_PENDING;
}

/**
 * Get the error code from a failed async function.
 *
 * @param state     State to query
 * @return          Error code, or 0 if no error
 */
static inline int32_t mr_async_get_error(mr_async_state_t *state)
{
    return (state->result == MR_ASYNC_ERROR) ? state->error_code : 0;
}

/*===========================================================================*/
/* Runner for Polling Async Functions                                         */
/*===========================================================================*/

/**
 * Run an async function to completion (blocking).
 * Polls the function until it completes.
 *
 * @param state     Async state
 * @param func      Function to run
 * @param arg       Argument to pass to function
 * @param poll_ms   Milliseconds between polls
 * @return          Final result
 */
mr_async_status_t mr_async_run_blocking(mr_async_state_t *state,
                                             mr_async_func_t func,
                                             void *arg,
                                             uint32_t poll_ms);

/**
 * Step an async function once (non-blocking).
 *
 * @param state     Async state
 * @param func      Function to step
 * @param arg       Argument to pass
 * @return          Current status
 */
static inline mr_async_status_t mr_async_step(mr_async_state_t *state,
                                                   mr_async_func_t func,
                                                   void *arg)
{
    return func(state, arg);
}

/*===========================================================================*/
/* Async Task Integration                                                     */
/*===========================================================================*/

/**
 * Async task context for running multiple async functions.
 */
typedef struct mr_async_runner {
    mr_async_func_t   func;       /* Current function */
    mr_async_state_t  state;      /* Function state */
    void                *arg;       /* Function argument */
    bool                active;     /* Runner is active */
} mr_async_runner_t;

/**
 * Initialize an async runner.
 */
void mr_async_runner_init(mr_async_runner_t *runner);

/**
 * Start a function in the runner.
 */
void mr_async_runner_start(mr_async_runner_t *runner,
                              mr_async_func_t func,
                              void *arg);

/**
 * Poll the runner (call from a task loop).
 *
 * @return  true if function still running, false if complete
 */
bool mr_async_runner_poll(mr_async_runner_t *runner);

/**
 * Get the result of the completed function.
 */
mr_async_status_t mr_async_runner_get_result(mr_async_runner_t *runner);

#ifdef __cplusplus
}
#endif

#endif /* MR_ASYNC_H */
