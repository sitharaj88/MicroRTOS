/**
 * Host-side stubs for kernel globals and port functions.
 *
 * Lets us link modules that depend on the running kernel into host unit
 * tests. The stubs are deliberately inert: critical sections are no-ops,
 * yield is a no-op, the scheduler hooks just track that they were called.
 *
 * Tests that exercise blocking paths must NOT use these stubs alone --
 * they need a fuller fake kernel.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos_types.h"
#include "rtos_port.h"
#include "rtos_list.h"
#include "host_stubs.h"

#include <stdio.h>
#include <stdlib.h>

/*===========================================================================*/
/* Kernel globals expected by core/sync/ipc modules                           */
/*===========================================================================*/

rtos_tcb_t                          *g_current_tcb = NULL;
rtos_list_t                          g_ready_list[RTOS_MAX_PRIORITIES];
rtos_list_t                          g_delayed_list;
volatile uint32_t                    g_tick_count = 0;
volatile rtos_kernel_state_t         g_kernel_state = RTOS_KERNEL_NOT_STARTED;
volatile uint8_t                     g_scheduler_suspended = 0;
volatile bool                        g_yield_pending = false;
volatile uint32_t                    g_ready_priorities = 0;

/*===========================================================================*/
/* Trip counters so tests can assert which hooks fired                        */
/*===========================================================================*/

unsigned host_stub_yield_count = 0;
unsigned host_stub_add_ready_count = 0;
unsigned host_stub_remove_ready_count = 0;

void host_stubs_reset(void)
{
    g_current_tcb = NULL;
    g_tick_count = 0;
    g_kernel_state = RTOS_KERNEL_NOT_STARTED;
    g_scheduler_suspended = 0;
    g_yield_pending = false;
    g_ready_priorities = 0;
    host_stub_yield_count = 0;
    host_stub_add_ready_count = 0;
    host_stub_remove_ready_count = 0;
    rtos_list_init(&g_delayed_list);
    for (uint32_t i = 0; i < RTOS_MAX_PRIORITIES; i++) {
        rtos_list_init(&g_ready_list[i]);
    }
}

/*===========================================================================*/
/* Port shims                                                                 */
/*===========================================================================*/

void rtos_port_init(void) {}
void rtos_port_start_scheduler(void) {}
void rtos_port_yield(void) { host_stub_yield_count++; }
void rtos_port_init_task_stack(rtos_tcb_t *tcb, void (*entry)(void*), void *arg)
{
    (void)tcb; (void)entry; (void)arg;
}
void rtos_port_enter_critical(void) {}
void rtos_port_exit_critical(void) {}
uint32_t rtos_port_disable_interrupts(void) { return 0; }
void rtos_port_restore_interrupts(uint32_t state) { (void)state; }
bool rtos_port_is_in_isr(void) { return false; }
#if RTOS_CHECK_STACK_OVERFLOW
void rtos_port_check_stack_overflow(void) {}
#endif

/*===========================================================================*/
/* Scheduler hooks                                                            */
/*===========================================================================*/

void rtos_scheduler_add_ready(rtos_tcb_t *tcb)
{
    (void)tcb;
    host_stub_add_ready_count++;
}

void rtos_scheduler_remove_ready(rtos_tcb_t *tcb)
{
    (void)tcb;
    host_stub_remove_ready_count++;
}

void rtos_assert_failed(const char *file, int line)
{
    fprintf(stderr, "RTOS_ASSERT failed at %s:%d\n", file, line);
    abort();
}
