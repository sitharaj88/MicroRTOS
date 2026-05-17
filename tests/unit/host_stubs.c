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

#include "mr_types.h"
#include "mr_port.h"
#include "mr_list.h"
#include "host_stubs.h"

#include <stdio.h>
#include <stdlib.h>

/*===========================================================================*/
/* Kernel globals expected by core/sync/ipc modules                           */
/*===========================================================================*/

mr_tcb_t                          *g_current_tcb = NULL;
mr_list_t                          g_ready_list[MR_MAX_PRIORITIES];
mr_list_t                          g_delayed_list;
volatile uint32_t                    g_tick_count = 0;
volatile mr_kernel_state_t         g_kernel_state = MR_KERNEL_NOT_STARTED;
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
    g_kernel_state = MR_KERNEL_NOT_STARTED;
    g_scheduler_suspended = 0;
    g_yield_pending = false;
    g_ready_priorities = 0;
    host_stub_yield_count = 0;
    host_stub_add_ready_count = 0;
    host_stub_remove_ready_count = 0;
    mr_list_init(&g_delayed_list);
    for (uint32_t i = 0; i < MR_MAX_PRIORITIES; i++) {
        mr_list_init(&g_ready_list[i]);
    }
}

/*===========================================================================*/
/* Port shims                                                                 */
/*===========================================================================*/

void mr_port_init(void) {}
void mr_port_start_scheduler(void) {}
void mr_port_yield(void) { host_stub_yield_count++; }
void mr_port_init_task_stack(mr_tcb_t *tcb, void (*entry)(void*), void *arg)
{
    (void)tcb; (void)entry; (void)arg;
}
void mr_port_enter_critical(void) {}
void mr_port_exit_critical(void) {}
uint32_t mr_port_disable_interrupts(void) { return 0; }
void mr_port_restore_interrupts(uint32_t state) { (void)state; }
bool mr_port_is_in_isr(void) { return false; }
#if MR_CHECK_STACK_OVERFLOW
void mr_port_check_stack_overflow(void) {}
#endif

/*===========================================================================*/
/* Scheduler hooks                                                            */
/*===========================================================================*/

void mr_scheduler_add_ready(mr_tcb_t *tcb)
{
    (void)tcb;
    host_stub_add_ready_count++;
}

void mr_scheduler_remove_ready(mr_tcb_t *tcb)
{
    (void)tcb;
    host_stub_remove_ready_count++;
}

void mr_assert_failed(const char *file, int line)
{
    fprintf(stderr, "MR_ASSERT failed at %s:%d\n", file, line);
    abort();
}
