/**
 * MicroRTOS - Runtime Tracing Implementation
 *
 * Lightweight ring-buffer based tracing for debugging and analysis.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_trace.h"
#include "mr_types.h"
#include <string.h>

#if MR_USE_TRACING

/*===========================================================================*/
/* External References                                                        */
/*===========================================================================*/

extern volatile uint32_t mr_tick_count;
extern uint32_t mr_port_disable_interrupts(void);
extern void mr_port_restore_interrupts(uint32_t state);

/*===========================================================================*/
/* Module State                                                               */
/*===========================================================================*/

static mr_trace_state_t trace_state = {
    .buffer = NULL,
    .size = 0,
    .head = 0,
    .tail = 0,
    .count = 0,
    .overflows = 0,
    .enabled = false,
    .recording = false,
};

/*===========================================================================*/
/* Trace API Implementation                                                   */
/*===========================================================================*/

void mr_trace_init(mr_trace_entry_t *buffer, uint32_t size)
{
    trace_state.buffer = buffer;
    trace_state.size = size;
    trace_state.head = 0;
    trace_state.tail = 0;
    trace_state.count = 0;
    trace_state.overflows = 0;
    trace_state.enabled = true;
    trace_state.recording = false;

    /* Clear buffer */
    memset(buffer, 0, size * sizeof(mr_trace_entry_t));

    /* Enable all events by default */
    mr_trace_filter_enable_all();
}

void mr_trace_start(void)
{
    if (trace_state.buffer != NULL) {
        trace_state.recording = true;
    }
}

void mr_trace_stop(void)
{
    trace_state.recording = false;
}

void mr_trace_clear(void)
{
    uint32_t state = mr_port_disable_interrupts();

    trace_state.head = 0;
    trace_state.tail = 0;
    trace_state.count = 0;
    trace_state.overflows = 0;

    mr_port_restore_interrupts(state);
}

bool mr_trace_is_recording(void)
{
    return trace_state.recording;
}

/**
 * Check if an event type is enabled in the filter.
 */
static inline bool trace_event_enabled(mr_trace_event_t event)
{
    uint8_t byte_idx = (uint8_t)event / 8;
    uint8_t bit_idx = (uint8_t)event % 8;

    if (byte_idx >= sizeof(trace_state.filter_mask)) {
        return false;
    }

    return (trace_state.filter_mask[byte_idx] & (1 << bit_idx)) != 0;
}

void mr_trace_event(mr_trace_event_t event,
                      uint8_t task_id,
                      uint16_t param1,
                      uint32_t param2)
{
    mr_trace_entry_t *entry;
    uint32_t state;

    if (!trace_state.recording || trace_state.buffer == NULL) {
        return;
    }

    /* Check filter */
    if (!trace_event_enabled(event)) {
        return;
    }

    state = mr_port_disable_interrupts();

    /* Get next entry slot */
    entry = &trace_state.buffer[trace_state.head];

    /* Fill entry */
#if MR_TRACE_USE_TIMESTAMP
    entry->timestamp = mr_tick_count;
#endif
    entry->event = (uint8_t)event;
    entry->task_id = task_id;
    entry->param1 = param1;
    entry->param2 = param2;

    /* Advance head */
    trace_state.head = (trace_state.head + 1) % trace_state.size;

    if (trace_state.count < trace_state.size) {
        trace_state.count++;
    } else {
        /* Buffer full - overwrite oldest */
        trace_state.tail = (trace_state.tail + 1) % trace_state.size;
        trace_state.overflows++;
    }

    mr_port_restore_interrupts(state);
}

void mr_trace_event_simple(mr_trace_event_t event, uint8_t task_id)
{
    mr_trace_event(event, task_id, 0, 0);
}

void mr_trace_user_event(uint8_t user_event_id,
                           uint16_t param1,
                           uint32_t param2)
{
    if (user_event_id > 55) {
        user_event_id = 55;
    }

    mr_trace_event((mr_trace_event_t)(MR_TRACE_USER_EVENT + user_event_id),
                     0, param1, param2);
}

/*===========================================================================*/
/* Event Filtering                                                            */
/*===========================================================================*/

void mr_trace_filter_set(mr_trace_event_t event, bool enabled)
{
    uint8_t byte_idx = (uint8_t)event / 8;
    uint8_t bit_idx = (uint8_t)event % 8;

    if (byte_idx >= sizeof(trace_state.filter_mask)) {
        return;
    }

    if (enabled) {
        trace_state.filter_mask[byte_idx] |= (1 << bit_idx);
    } else {
        trace_state.filter_mask[byte_idx] &= ~(1 << bit_idx);
    }
}

void mr_trace_filter_enable_all(void)
{
    memset(trace_state.filter_mask, 0xFF, sizeof(trace_state.filter_mask));
}

void mr_trace_filter_disable_all(void)
{
    memset(trace_state.filter_mask, 0x00, sizeof(trace_state.filter_mask));
}

void mr_trace_filter_tasks_only(void)
{
    mr_trace_filter_disable_all();

    /* Enable task events (0-9) */
    trace_state.filter_mask[0] = 0xFF;
    trace_state.filter_mask[1] = 0x03;
}

void mr_trace_filter_sync_only(void)
{
    mr_trace_filter_disable_all();

    /* Enable mutex, semaphore, queue events (40-69) */
    trace_state.filter_mask[5] = 0xFF;
    trace_state.filter_mask[6] = 0xFF;
    trace_state.filter_mask[7] = 0xFF;
    trace_state.filter_mask[8] = 0x3F;
}

/*===========================================================================*/
/* Trace Retrieval                                                            */
/*===========================================================================*/

uint32_t mr_trace_get_count(void)
{
    return trace_state.count;
}

uint32_t mr_trace_get_overflow_count(void)
{
    return trace_state.overflows;
}

bool mr_trace_read(mr_trace_entry_t *entry)
{
    uint32_t state;

    if (trace_state.count == 0 || entry == NULL) {
        return false;
    }

    state = mr_port_disable_interrupts();

    *entry = trace_state.buffer[trace_state.tail];
    trace_state.tail = (trace_state.tail + 1) % trace_state.size;
    trace_state.count--;

    mr_port_restore_interrupts(state);

    return true;
}

bool mr_trace_peek(mr_trace_entry_t *entry, uint32_t index)
{
    uint32_t actual_index;

    if (index >= trace_state.count || entry == NULL) {
        return false;
    }

    actual_index = (trace_state.tail + index) % trace_state.size;
    *entry = trace_state.buffer[actual_index];

    return true;
}

/*===========================================================================*/
/* Export Functions                                                           */
/*===========================================================================*/

/**
 * Output a single byte for export functions.
 */
static void output_byte(mr_trace_output_fn output, void *context, uint8_t byte)
{
    if (output != NULL) {
        output(byte, context);
    }
}

/**
 * Output a string for export functions.
 */
static void output_string(mr_trace_output_fn output, void *context, const char *str)
{
    while (*str) {
        output_byte(output, context, (uint8_t)*str++);
    }
}

/**
 * Output a decimal number as string.
 */
static void output_decimal(mr_trace_output_fn output, void *context, uint32_t value)
{
    char buf[12];
    int i = 0;

    if (value == 0) {
        output_byte(output, context, '0');
        return;
    }

    while (value > 0 && i < 11) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0) {
        output_byte(output, context, (uint8_t)buf[--i]);
    }
}

uint32_t mr_trace_export_systemview(mr_trace_output_fn output, void *context)
{
    mr_trace_entry_t entry;
    uint32_t bytes = 0;
    uint32_t count = trace_state.count;
    uint32_t i;

    /*
     * SystemView format is binary.
     * This is a simplified version - full SystemView uses SEGGER RTT.
     */

    /* Header */
    output_byte(output, context, 'S');
    output_byte(output, context, 'V');
    output_byte(output, context, 0x01);  /* Version */
    bytes += 3;

    /* Export entries */
    for (i = 0; i < count; i++) {
        if (!mr_trace_peek(&entry, i)) {
            break;
        }

        /* Entry marker */
        output_byte(output, context, 0xFE);
        bytes++;

        /* Timestamp (4 bytes, little-endian) */
#if MR_TRACE_USE_TIMESTAMP
        output_byte(output, context, (uint8_t)(entry.timestamp & 0xFF));
        output_byte(output, context, (uint8_t)((entry.timestamp >> 8) & 0xFF));
        output_byte(output, context, (uint8_t)((entry.timestamp >> 16) & 0xFF));
        output_byte(output, context, (uint8_t)((entry.timestamp >> 24) & 0xFF));
        bytes += 4;
#endif

        /* Event data */
        output_byte(output, context, entry.event);
        output_byte(output, context, entry.task_id);
        output_byte(output, context, (uint8_t)(entry.param1 & 0xFF));
        output_byte(output, context, (uint8_t)((entry.param1 >> 8) & 0xFF));
        output_byte(output, context, (uint8_t)(entry.param2 & 0xFF));
        output_byte(output, context, (uint8_t)((entry.param2 >> 8) & 0xFF));
        output_byte(output, context, (uint8_t)((entry.param2 >> 16) & 0xFF));
        output_byte(output, context, (uint8_t)((entry.param2 >> 24) & 0xFF));
        bytes += 8;
    }

    /* End marker */
    output_byte(output, context, 0xFF);
    bytes++;

    return bytes;
}

uint32_t mr_trace_export_csv(mr_trace_output_fn output, void *context)
{
    mr_trace_entry_t entry;
    uint32_t bytes = 0;
    uint32_t count = trace_state.count;
    uint32_t i;

    /* CSV header */
#if MR_TRACE_USE_TIMESTAMP
    output_string(output, context, "Timestamp,Event,EventName,TaskID,Param1,Param2\r\n");
    bytes += 48;
#else
    output_string(output, context, "Event,EventName,TaskID,Param1,Param2\r\n");
    bytes += 38;
#endif

    /* Export entries */
    for (i = 0; i < count; i++) {
        if (!mr_trace_peek(&entry, i)) {
            break;
        }

#if MR_TRACE_USE_TIMESTAMP
        output_decimal(output, context, entry.timestamp);
        output_byte(output, context, ',');
#endif

        output_decimal(output, context, entry.event);
        output_byte(output, context, ',');

        output_string(output, context, mr_trace_event_name((mr_trace_event_t)entry.event));
        output_byte(output, context, ',');

        output_decimal(output, context, entry.task_id);
        output_byte(output, context, ',');

        output_decimal(output, context, entry.param1);
        output_byte(output, context, ',');

        output_decimal(output, context, entry.param2);

        output_string(output, context, "\r\n");
    }

    return bytes;
}

/*===========================================================================*/
/* Event Names                                                                */
/*===========================================================================*/

const char *mr_trace_event_name(mr_trace_event_t event)
{
    switch (event) {
        /* Task events */
        case MR_TRACE_TASK_SWITCH_IN:     return "TASK_SWITCH_IN";
        case MR_TRACE_TASK_SWITCH_OUT:    return "TASK_SWITCH_OUT";
        case MR_TRACE_TASK_CREATE:        return "TASK_CREATE";
        case MR_TRACE_TASK_DELETE:        return "TASK_DELETE";
        case MR_TRACE_TASK_SUSPEND:       return "TASK_SUSPEND";
        case MR_TRACE_TASK_RESUME:        return "TASK_RESUME";
        case MR_TRACE_TASK_DELAY:         return "TASK_DELAY";
        case MR_TRACE_TASK_READY:         return "TASK_READY";
        case MR_TRACE_TASK_PRIORITY_SET:  return "TASK_PRIORITY_SET";

        /* ISR events */
        case MR_TRACE_ISR_ENTER:          return "ISR_ENTER";
        case MR_TRACE_ISR_EXIT:           return "ISR_EXIT";
        case MR_TRACE_ISR_EXIT_TO_SCHEDULER: return "ISR_EXIT_SCHED";

        /* Mutex events */
        case MR_TRACE_MUTEX_LOCK:         return "MUTEX_LOCK";
        case MR_TRACE_MUTEX_UNLOCK:       return "MUTEX_UNLOCK";
        case MR_TRACE_MUTEX_BLOCK:        return "MUTEX_BLOCK";
        case MR_TRACE_MUTEX_PRIORITY_INHERIT: return "MUTEX_PRI_INH";

        /* Semaphore events */
        case MR_TRACE_SEM_TAKE:           return "SEM_TAKE";
        case MR_TRACE_SEM_GIVE:           return "SEM_GIVE";
        case MR_TRACE_SEM_BLOCK:          return "SEM_BLOCK";

        /* Queue events */
        case MR_TRACE_QUEUE_SEND:         return "QUEUE_SEND";
        case MR_TRACE_QUEUE_RECEIVE:      return "QUEUE_RECEIVE";
        case MR_TRACE_QUEUE_SEND_BLOCK:   return "QUEUE_SEND_BLK";
        case MR_TRACE_QUEUE_RECEIVE_BLOCK: return "QUEUE_RECV_BLK";

        /* Event group events */
        case MR_TRACE_EVENT_SET:          return "EVENT_SET";
        case MR_TRACE_EVENT_CLEAR:        return "EVENT_CLEAR";
        case MR_TRACE_EVENT_WAIT:         return "EVENT_WAIT";
        case MR_TRACE_EVENT_WAKEUP:       return "EVENT_WAKEUP";

        /* Timer events */
        case MR_TRACE_TIMER_START:        return "TIMER_START";
        case MR_TRACE_TIMER_STOP:         return "TIMER_STOP";
        case MR_TRACE_TIMER_EXPIRED:      return "TIMER_EXPIRED";

        /* Memory events */
        case MR_TRACE_MEMPOOL_ALLOC:      return "MEMPOOL_ALLOC";
        case MR_TRACE_MEMPOOL_FREE:       return "MEMPOOL_FREE";

        /* System events */
        case MR_TRACE_SYSTEM_TICK:        return "SYSTEM_TICK";
        case MR_TRACE_IDLE_ENTER:         return "IDLE_ENTER";
        case MR_TRACE_IDLE_EXIT:          return "IDLE_EXIT";
        case MR_TRACE_SCHEDULER_SUSPEND:  return "SCHED_SUSPEND";
        case MR_TRACE_SCHEDULER_RESUME:   return "SCHED_RESUME";
        case MR_TRACE_STACK_OVERFLOW:     return "STACK_OVERFLOW";

        default:
            if (event >= MR_TRACE_USER_EVENT) {
                return "USER_EVENT";
            }
            return "UNKNOWN";
    }
}

#else /* !MR_USE_TRACING */

/*===========================================================================*/
/* Stub Implementation                                                        */
/*===========================================================================*/

void mr_trace_init(mr_trace_entry_t *buffer, uint32_t size)
{
    (void)buffer;
    (void)size;
}

void mr_trace_start(void) {}
void mr_trace_stop(void) {}
void mr_trace_clear(void) {}
bool mr_trace_is_recording(void) { return false; }

void mr_trace_event(mr_trace_event_t event, uint8_t task_id,
                      uint16_t param1, uint32_t param2)
{
    (void)event; (void)task_id; (void)param1; (void)param2;
}

void mr_trace_event_simple(mr_trace_event_t event, uint8_t task_id)
{
    (void)event; (void)task_id;
}

void mr_trace_user_event(uint8_t user_event_id, uint16_t param1, uint32_t param2)
{
    (void)user_event_id; (void)param1; (void)param2;
}

void mr_trace_filter_set(mr_trace_event_t event, bool enabled)
{
    (void)event; (void)enabled;
}

void mr_trace_filter_enable_all(void) {}
void mr_trace_filter_disable_all(void) {}
void mr_trace_filter_tasks_only(void) {}
void mr_trace_filter_sync_only(void) {}

uint32_t mr_trace_get_count(void) { return 0; }
uint32_t mr_trace_get_overflow_count(void) { return 0; }

bool mr_trace_read(mr_trace_entry_t *entry)
{
    (void)entry;
    return false;
}

bool mr_trace_peek(mr_trace_entry_t *entry, uint32_t index)
{
    (void)entry; (void)index;
    return false;
}

uint32_t mr_trace_export_systemview(mr_trace_output_fn output, void *context)
{
    (void)output; (void)context;
    return 0;
}

uint32_t mr_trace_export_csv(mr_trace_output_fn output, void *context)
{
    (void)output; (void)context;
    return 0;
}

const char *mr_trace_event_name(mr_trace_event_t event)
{
    (void)event;
    return "DISABLED";
}

#endif /* MR_USE_TRACING */
