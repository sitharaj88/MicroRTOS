/**
 * MicroRTOS - Runtime Tracing and Diagnostics
 *
 * Lightweight tracing infrastructure for debugging and performance analysis.
 * Compatible with SystemView and Tracealyzer visualization tools.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef RTOS_TRACE_H
#define RTOS_TRACE_H

#include "rtos_config.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Configuration                                                              */
/*===========================================================================*/

#ifndef RTOS_USE_TRACING
#define RTOS_USE_TRACING            0
#endif

#ifndef RTOS_TRACE_BUFFER_SIZE
#define RTOS_TRACE_BUFFER_SIZE      256     /* Number of trace entries */
#endif

#ifndef RTOS_TRACE_USE_TIMESTAMP
#define RTOS_TRACE_USE_TIMESTAMP    1       /* Include timestamps */
#endif

#ifndef RTOS_TRACE_INCLUDE_ISR
#define RTOS_TRACE_INCLUDE_ISR      1       /* Trace ISR events */
#endif

/*===========================================================================*/
/* Trace Event Types                                                          */
/*===========================================================================*/

typedef enum {
    /* Task events */
    RTOS_TRACE_TASK_SWITCH_IN = 0,      /* Task started running */
    RTOS_TRACE_TASK_SWITCH_OUT,         /* Task stopped running */
    RTOS_TRACE_TASK_CREATE,             /* Task created */
    RTOS_TRACE_TASK_DELETE,             /* Task deleted */
    RTOS_TRACE_TASK_SUSPEND,            /* Task suspended */
    RTOS_TRACE_TASK_RESUME,             /* Task resumed */
    RTOS_TRACE_TASK_DELAY,              /* Task entered delay */
    RTOS_TRACE_TASK_READY,              /* Task became ready */
    RTOS_TRACE_TASK_PRIORITY_SET,       /* Task priority changed */

    /* ISR events */
    RTOS_TRACE_ISR_ENTER = 20,          /* Entered ISR */
    RTOS_TRACE_ISR_EXIT,                /* Exited ISR */
    RTOS_TRACE_ISR_EXIT_TO_SCHEDULER,   /* ISR triggering context switch */

    /* Synchronization events */
    RTOS_TRACE_MUTEX_LOCK = 40,         /* Mutex acquired */
    RTOS_TRACE_MUTEX_UNLOCK,            /* Mutex released */
    RTOS_TRACE_MUTEX_BLOCK,             /* Blocked on mutex */
    RTOS_TRACE_MUTEX_PRIORITY_INHERIT,  /* Priority inheritance occurred */

    RTOS_TRACE_SEM_TAKE = 50,           /* Semaphore taken */
    RTOS_TRACE_SEM_GIVE,                /* Semaphore given */
    RTOS_TRACE_SEM_BLOCK,               /* Blocked on semaphore */

    /* IPC events */
    RTOS_TRACE_QUEUE_SEND = 60,         /* Message sent to queue */
    RTOS_TRACE_QUEUE_RECEIVE,           /* Message received from queue */
    RTOS_TRACE_QUEUE_SEND_BLOCK,        /* Blocked on queue send */
    RTOS_TRACE_QUEUE_RECEIVE_BLOCK,     /* Blocked on queue receive */

    /* Event group events */
    RTOS_TRACE_EVENT_SET = 70,          /* Event bits set */
    RTOS_TRACE_EVENT_CLEAR,             /* Event bits cleared */
    RTOS_TRACE_EVENT_WAIT,              /* Waiting for events */
    RTOS_TRACE_EVENT_WAKEUP,            /* Woken by events */

    /* Timer events */
    RTOS_TRACE_TIMER_START = 80,        /* Timer started */
    RTOS_TRACE_TIMER_STOP,              /* Timer stopped */
    RTOS_TRACE_TIMER_EXPIRED,           /* Timer expired/callback */

    /* Memory events */
    RTOS_TRACE_MEMPOOL_ALLOC = 90,      /* Block allocated */
    RTOS_TRACE_MEMPOOL_FREE,            /* Block freed */

    /* System events */
    RTOS_TRACE_SYSTEM_TICK = 100,       /* System tick */
    RTOS_TRACE_IDLE_ENTER,              /* Entered idle */
    RTOS_TRACE_IDLE_EXIT,               /* Exited idle */
    RTOS_TRACE_SCHEDULER_SUSPEND,       /* Scheduler suspended */
    RTOS_TRACE_SCHEDULER_RESUME,        /* Scheduler resumed */
    RTOS_TRACE_STACK_OVERFLOW,          /* Stack overflow detected */

    /* Custom/user events */
    RTOS_TRACE_USER_EVENT = 200,        /* User-defined event base */

    RTOS_TRACE_EVENT_MAX = 255
} rtos_trace_event_t;

/*===========================================================================*/
/* Trace Entry Structure                                                      */
/*===========================================================================*/

typedef struct {
#if RTOS_TRACE_USE_TIMESTAMP
    uint32_t timestamp;             /* Tick count or high-res timer */
#endif
    uint8_t  event;                 /* Event type (rtos_trace_event_t) */
    uint8_t  task_id;               /* Task ID (0 for ISR/system) */
    uint16_t param1;                /* Event-specific parameter */
    uint32_t param2;                /* Event-specific parameter */
} rtos_trace_entry_t;

/*===========================================================================*/
/* Trace Buffer State                                                         */
/*===========================================================================*/

typedef struct {
    rtos_trace_entry_t *buffer;     /* Pointer to trace buffer */
    uint32_t size;                  /* Buffer size (number of entries) */
    volatile uint32_t head;         /* Write position */
    volatile uint32_t tail;         /* Read position */
    volatile uint32_t count;        /* Number of entries */
    volatile uint32_t overflows;    /* Number of lost entries */
    volatile bool enabled;          /* Tracing enabled flag */
    volatile bool recording;        /* Currently recording */
    uint8_t filter_mask[32];        /* Event filter (bit per event type) */
} rtos_trace_state_t;

/*===========================================================================*/
/* Trace API                                                                  */
/*===========================================================================*/

/**
 * Initialize the trace system.
 *
 * @param buffer    Pointer to trace entry buffer
 * @param size      Number of entries in buffer
 */
void rtos_trace_init(rtos_trace_entry_t *buffer, uint32_t size);

/**
 * Start recording trace events.
 */
void rtos_trace_start(void);

/**
 * Stop recording trace events.
 */
void rtos_trace_stop(void);

/**
 * Clear all trace data.
 */
void rtos_trace_clear(void);

/**
 * Check if tracing is active.
 *
 * @return true if recording, false otherwise
 */
bool rtos_trace_is_recording(void);

/**
 * Record a trace event.
 *
 * @param event     Event type
 * @param task_id   Task ID (0 for system/ISR)
 * @param param1    First parameter
 * @param param2    Second parameter
 */
void rtos_trace_event(rtos_trace_event_t event,
                      uint8_t task_id,
                      uint16_t param1,
                      uint32_t param2);

/**
 * Record a trace event with only event type and task.
 */
void rtos_trace_event_simple(rtos_trace_event_t event, uint8_t task_id);

/**
 * Record a user-defined event.
 *
 * @param user_event_id     User event ID (0-55)
 * @param param1            First parameter
 * @param param2            Second parameter
 */
void rtos_trace_user_event(uint8_t user_event_id,
                           uint16_t param1,
                           uint32_t param2);

/*===========================================================================*/
/* Event Filtering                                                            */
/*===========================================================================*/

/**
 * Enable or disable a specific event type.
 *
 * @param event     Event type to configure
 * @param enabled   true to record, false to ignore
 */
void rtos_trace_filter_set(rtos_trace_event_t event, bool enabled);

/**
 * Enable all event types.
 */
void rtos_trace_filter_enable_all(void);

/**
 * Disable all event types.
 */
void rtos_trace_filter_disable_all(void);

/**
 * Enable only task-related events.
 */
void rtos_trace_filter_tasks_only(void);

/**
 * Enable only synchronization events.
 */
void rtos_trace_filter_sync_only(void);

/*===========================================================================*/
/* Trace Retrieval                                                            */
/*===========================================================================*/

/**
 * Get number of trace entries available.
 *
 * @return Number of entries in buffer
 */
uint32_t rtos_trace_get_count(void);

/**
 * Get number of lost entries due to overflow.
 *
 * @return Number of overflowed entries
 */
uint32_t rtos_trace_get_overflow_count(void);

/**
 * Read a trace entry.
 *
 * @param entry     Pointer to store entry
 * @return          true if entry read, false if buffer empty
 */
bool rtos_trace_read(rtos_trace_entry_t *entry);

/**
 * Peek at a trace entry without removing it.
 *
 * @param entry     Pointer to store entry
 * @param index     Index from oldest entry (0 = oldest)
 * @return          true if entry exists, false otherwise
 */
bool rtos_trace_peek(rtos_trace_entry_t *entry, uint32_t index);

/*===========================================================================*/
/* Export Functions                                                           */
/*===========================================================================*/

/**
 * Export trace data in SystemView format.
 *
 * @param output    Callback function for output
 * @param context   User context passed to callback
 * @return          Number of bytes exported
 */
typedef void (*rtos_trace_output_fn)(uint8_t byte, void *context);
uint32_t rtos_trace_export_systemview(rtos_trace_output_fn output, void *context);

/**
 * Export trace data as CSV text.
 *
 * @param output    Callback function for output
 * @param context   User context passed to callback
 * @return          Number of bytes exported
 */
uint32_t rtos_trace_export_csv(rtos_trace_output_fn output, void *context);

/*===========================================================================*/
/* Instrumentation Macros                                                     */
/*===========================================================================*/

#if RTOS_USE_TRACING

/*
 * Synthesize a stable 8-bit task identifier from the TCB pointer.
 * Why: TCBs have no dedicated id field; their address is the canonical
 * identity. Shifting drops alignment bits so adjacent TCBs map to
 * distinct ids for small task counts.
 */
#define RTOS_TRACE_TASK_ID(tcb)     ((uint8_t)((uintptr_t)(tcb) >> 2))

/* Task events */
#define RTOS_TRACE_TASK_SWITCHED_IN(tcb) \
    rtos_trace_event(RTOS_TRACE_TASK_SWITCH_IN, RTOS_TRACE_TASK_ID(tcb), 0, 0)

#define RTOS_TRACE_TASK_SWITCHED_OUT(tcb) \
    rtos_trace_event(RTOS_TRACE_TASK_SWITCH_OUT, RTOS_TRACE_TASK_ID(tcb), 0, 0)

#define RTOS_TRACE_TASK_CREATED(tcb) \
    rtos_trace_event(RTOS_TRACE_TASK_CREATE, RTOS_TRACE_TASK_ID(tcb), (tcb)->priority, 0)

#define RTOS_TRACE_TASK_DELAYED(tcb, ticks) \
    rtos_trace_event(RTOS_TRACE_TASK_DELAY, RTOS_TRACE_TASK_ID(tcb), 0, (ticks))

/* ISR events */
#define RTOS_TRACE_ISR_ENTER_M(isr_id) \
    rtos_trace_event(RTOS_TRACE_ISR_ENTER, 0, (isr_id), 0)

#define RTOS_TRACE_ISR_EXIT_M() \
    rtos_trace_event_simple(RTOS_TRACE_ISR_EXIT, 0)

/* Mutex events */
#define RTOS_TRACE_MUTEX_ACQUIRED(mutex, tcb) \
    rtos_trace_event(RTOS_TRACE_MUTEX_LOCK, RTOS_TRACE_TASK_ID(tcb), 0, (uint32_t)(uintptr_t)(mutex))

#define RTOS_TRACE_MUTEX_RELEASED(mutex, tcb) \
    rtos_trace_event(RTOS_TRACE_MUTEX_UNLOCK, RTOS_TRACE_TASK_ID(tcb), 0, (uint32_t)(uintptr_t)(mutex))

/* Queue events */
#define RTOS_TRACE_QUEUE_SENT(queue, tcb) \
    rtos_trace_event(RTOS_TRACE_QUEUE_SEND, RTOS_TRACE_TASK_ID(tcb), 0, (uint32_t)(uintptr_t)(queue))

#define RTOS_TRACE_QUEUE_RECEIVED(queue, tcb) \
    rtos_trace_event(RTOS_TRACE_QUEUE_RECEIVE, RTOS_TRACE_TASK_ID(tcb), 0, (uint32_t)(uintptr_t)(queue))

/* System events */
#define RTOS_TRACE_TICK() \
    rtos_trace_event_simple(RTOS_TRACE_SYSTEM_TICK, 0)

#define RTOS_TRACE_IDLE() \
    rtos_trace_event_simple(RTOS_TRACE_IDLE_ENTER, 0)

#else /* !RTOS_USE_TRACING */

/* Disabled - compile to nothing */
#define RTOS_TRACE_TASK_SWITCHED_IN(tcb)            ((void)0)
#define RTOS_TRACE_TASK_SWITCHED_OUT(tcb)           ((void)0)
#define RTOS_TRACE_TASK_CREATED(tcb)                ((void)0)
#define RTOS_TRACE_TASK_DELAYED(tcb, ticks)         ((void)0)
#define RTOS_TRACE_ISR_ENTER_M(isr_id)              ((void)0)
#define RTOS_TRACE_ISR_EXIT_M()                     ((void)0)
#define RTOS_TRACE_MUTEX_ACQUIRED(mutex, tcb)       ((void)0)
#define RTOS_TRACE_MUTEX_RELEASED(mutex, tcb)       ((void)0)
#define RTOS_TRACE_QUEUE_SENT(queue, tcb)           ((void)0)
#define RTOS_TRACE_QUEUE_RECEIVED(queue, tcb)       ((void)0)
#define RTOS_TRACE_TICK()                           ((void)0)
#define RTOS_TRACE_IDLE()                           ((void)0)

#endif /* RTOS_USE_TRACING */

/*===========================================================================*/
/* Event Name Lookup                                                          */
/*===========================================================================*/

/**
 * Get human-readable name for an event type.
 *
 * @param event     Event type
 * @return          Pointer to event name string
 */
const char *rtos_trace_event_name(rtos_trace_event_t event);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_TRACE_H */
