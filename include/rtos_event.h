/**
 * MicroRTOS - Event Groups API
 *
 * Event flags for synchronizing multiple tasks.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef RTOS_EVENT_H
#define RTOS_EVENT_H

#include "rtos_types.h"

#if RTOS_USE_EVENT_GROUPS

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Event Groups API                                                           */
/*===========================================================================*/

/**
 * Initialize an event group.
 *
 * @param event Pointer to event group structure
 */
void rtos_event_init(rtos_event_t *event);

/**
 * Wait for event bits to be set.
 *
 * @param event        Pointer to event group
 * @param bits         Bits to wait for (bitmask)
 * @param wait_all     true = wait for ALL bits, false = wait for ANY bit
 * @param clear_on_exit true = clear matching bits when returning
 * @param timeout      Maximum ticks to wait
 *
 * @return Bits that were set when unblocked, or 0 on timeout
 */
uint32_t rtos_event_wait(
    rtos_event_t *event,
    uint32_t bits,
    bool wait_all,
    bool clear_on_exit,
    uint32_t timeout
);

/**
 * Set event bits.
 *
 * Wakes any tasks waiting for these bits.
 *
 * @param event Pointer to event group
 * @param bits  Bits to set (bitmask)
 *
 * @return Current event bits after setting
 */
uint32_t rtos_event_set(rtos_event_t *event, uint32_t bits);

/**
 * Clear event bits.
 *
 * @param event Pointer to event group
 * @param bits  Bits to clear (bitmask)
 *
 * @return Current event bits before clearing
 */
uint32_t rtos_event_clear(rtos_event_t *event, uint32_t bits);

/**
 * Get current event bits without modifying them.
 *
 * @param event Pointer to event group
 *
 * @return Current event bits
 */
uint32_t rtos_event_get(rtos_event_t *event);

/**
 * Set event bits from an ISR.
 *
 * @param event          Pointer to event group
 * @param bits           Bits to set
 * @param yield_required Set to true if context switch should occur
 *
 * @return Current event bits after setting
 */
uint32_t rtos_event_set_from_isr(
    rtos_event_t *event,
    uint32_t bits,
    bool *yield_required
);

/**
 * Synchronize multiple tasks at a rendezvous point.
 *
 * Each task sets its own bit and waits for all others.
 *
 * @param event       Pointer to event group
 * @param set_bits    Bits this task sets (its "arrival" bit)
 * @param wait_bits   Bits to wait for (all tasks' bits)
 * @param timeout     Maximum ticks to wait
 *
 * @return Bits that were set when synchronized, or 0 on timeout
 */
uint32_t rtos_event_sync(
    rtos_event_t *event,
    uint32_t set_bits,
    uint32_t wait_bits,
    uint32_t timeout
);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_USE_EVENT_GROUPS */

#endif /* RTOS_EVENT_H */
