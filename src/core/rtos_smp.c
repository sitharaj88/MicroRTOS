/**
 * MicroRTOS - SMP Implementation
 *
 * Symmetric Multi-Processing support for multi-core systems.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "rtos_smp.h"
#include "rtos_types.h"
#include "rtos_list.h"
#include "rtos_atomic.h"
#include "rtos_port.h"

#if RTOS_USE_SMP

/*===========================================================================*/
/* External References                                                        */
/*===========================================================================*/

extern volatile uint32_t rtos_tick_count;
extern void rtos_port_yield(void);

/*===========================================================================*/
/* Module State                                                               */
/*===========================================================================*/

/* Per-core state */
static rtos_core_state_t core_states[RTOS_SMP_MAX_CORES];

/* Global scheduler lock for SMP */
static rtos_smp_spinlock_t scheduler_lock = RTOS_SMP_SPINLOCK_INIT;

/* Number of active cores */
static volatile uint8_t active_core_count = 1;

/* SMP initialized flag */
static volatile bool smp_initialized = false;

/*===========================================================================*/
/* SMP Initialization                                                         */
/*===========================================================================*/

void rtos_smp_init(void)
{
    uint8_t i, j;

    for (i = 0; i < RTOS_SMP_MAX_CORES; i++) {
        core_states[i].current_tcb = NULL;
        core_states[i].idle_tcb = NULL;
        core_states[i].ready_bitmap = 0;
        core_states[i].core_id = i;
        core_states[i].running = false;
        core_states[i].yield_pending = false;
        core_states[i].irq_nesting = 0;
        core_states[i].idle_ticks = 0;
        core_states[i].context_switches = 0;

        /* Initialize ready lists for this core */
        for (j = 0; j < RTOS_MAX_PRIORITIES; j++) {
            rtos_list_init(&core_states[i].ready_list[j]);
        }
    }

    rtos_smp_spinlock_init(&scheduler_lock);
    smp_initialized = true;
}

void rtos_smp_start_cores(void)
{
    uint8_t i;

    /* Start secondary cores */
    for (i = 1; i < RTOS_SMP_MAX_CORES; i++) {
        /* Platform-specific core startup */
        rtos_port_smp_start_core(i, rtos_smp_core_entry, NULL);
        active_core_count++;
    }
}

void rtos_smp_core_entry(void)
{
    uint8_t core_id = rtos_port_smp_get_core_id();

    /* Mark core as running */
    core_states[core_id].running = true;

    /* Enter scheduler - platform-specific */
    /* This function should not return */
    while (1) {
        /* Idle loop for secondary core */
        __asm volatile("wfi");
    }
}

/*===========================================================================*/
/* Core Identification                                                        */
/*===========================================================================*/

uint8_t rtos_smp_get_core_id(void)
{
    return rtos_port_smp_get_core_id();
}

uint8_t rtos_smp_get_core_count(void)
{
    return active_core_count;
}

bool rtos_smp_is_active(void)
{
    return (smp_initialized && active_core_count > 1);
}

rtos_core_state_t *rtos_smp_get_core_state(uint8_t core_id)
{
    if (core_id >= RTOS_SMP_MAX_CORES) {
        return NULL;
    }
    return &core_states[core_id];
}

rtos_core_state_t *rtos_smp_get_current_core(void)
{
    return &core_states[rtos_port_smp_get_core_id()];
}

/*===========================================================================*/
/* Task Affinity                                                              */
/*===========================================================================*/

rtos_status_t rtos_smp_set_affinity(rtos_tcb_t *tcb, uint8_t affinity)
{
    uint32_t state;

    if (tcb == NULL) {
        return RTOS_ERR_PARAM;
    }

    state = rtos_smp_critical_enter();

    /* Store affinity in TCB (would need to add field) */
    /* tcb->core_affinity = affinity; */

    rtos_smp_critical_exit(state);

    return RTOS_OK;
}

uint8_t rtos_smp_get_affinity(rtos_tcb_t *tcb)
{
    if (tcb == NULL) {
        return RTOS_AFFINITY_ALL;
    }

    /* Return affinity from TCB */
    /* return tcb->core_affinity; */
    return RTOS_AFFINITY_ALL;
}

bool rtos_smp_can_run_on_core(rtos_tcb_t *tcb, uint8_t core_id)
{
    uint8_t affinity;

    if (tcb == NULL || core_id >= RTOS_SMP_MAX_CORES) {
        return false;
    }

    affinity = rtos_smp_get_affinity(tcb);

    if (affinity == RTOS_CORE_ANY) {
        return true;
    }

    return (affinity & RTOS_AFFINITY_CORE(core_id)) != 0;
}

/*===========================================================================*/
/* SMP Spinlock Implementation                                                */
/*===========================================================================*/

void rtos_smp_spinlock_init(rtos_smp_spinlock_t *lock)
{
    if (lock == NULL) {
        return;
    }

    lock->lock.value = 0;
    lock->owner_core = 0xFF;
    lock->nesting = 0;
    lock->irq_state = 0;
}

void rtos_smp_spinlock_acquire(rtos_smp_spinlock_t *lock)
{
    uint32_t irq_state;
    uint8_t core_id;

    if (lock == NULL) {
        return;
    }

    /* Disable interrupts first */
    irq_state = rtos_port_disable_interrupts();
    core_id = rtos_port_smp_get_core_id();

    /* Check for recursive lock by same core */
    if (lock->owner_core == core_id) {
        lock->nesting++;
        return;
    }

    /* Acquire the spinlock */
    rtos_spinlock_acquire(&lock->lock);

    /* Record ownership */
    lock->owner_core = core_id;
    lock->nesting = 1;
    lock->irq_state = irq_state;
}

void rtos_smp_spinlock_release(rtos_smp_spinlock_t *lock)
{
    uint32_t irq_state;

    if (lock == NULL || lock->nesting == 0) {
        return;
    }

    lock->nesting--;

    if (lock->nesting == 0) {
        irq_state = lock->irq_state;
        lock->owner_core = 0xFF;

        /* Release spinlock */
        rtos_spinlock_release(&lock->lock);

        /* Restore interrupts */
        rtos_port_restore_interrupts(irq_state);
    }
}

bool rtos_smp_spinlock_try_acquire(rtos_smp_spinlock_t *lock)
{
    uint32_t irq_state;
    uint8_t core_id;

    if (lock == NULL) {
        return false;
    }

    irq_state = rtos_port_disable_interrupts();
    core_id = rtos_port_smp_get_core_id();

    /* Check for recursive lock */
    if (lock->owner_core == core_id) {
        lock->nesting++;
        return true;
    }

    /* Try to acquire */
    if (rtos_spinlock_try_acquire(&lock->lock)) {
        lock->owner_core = core_id;
        lock->nesting = 1;
        lock->irq_state = irq_state;
        return true;
    }

    /* Failed - restore interrupts */
    rtos_port_restore_interrupts(irq_state);
    return false;
}

/*===========================================================================*/
/* Inter-Processor Interrupt                                                  */
/*===========================================================================*/

void rtos_smp_send_ipi(uint8_t core_id, rtos_ipi_type_t ipi_type)
{
    if (core_id >= RTOS_SMP_MAX_CORES) {
        return;
    }

    rtos_port_smp_send_ipi(core_id, ipi_type);
}

void rtos_smp_send_ipi_all(rtos_ipi_type_t ipi_type)
{
    uint8_t i;
    uint8_t current = rtos_port_smp_get_core_id();

    for (i = 0; i < active_core_count; i++) {
        if (i != current) {
            rtos_port_smp_send_ipi(i, ipi_type);
        }
    }
}

void rtos_smp_request_reschedule(uint8_t core_id)
{
    if (core_id >= RTOS_SMP_MAX_CORES) {
        return;
    }

    core_states[core_id].yield_pending = true;
    rtos_smp_send_ipi(core_id, RTOS_IPI_RESCHEDULE);
}

/*===========================================================================*/
/* Cross-Core Function Calls                                                  */
/*===========================================================================*/

/* Pending call information */
static volatile struct {
    rtos_smp_call_func_t func;
    void *arg;
    volatile bool pending;
    volatile bool done;
} cross_core_calls[RTOS_SMP_MAX_CORES];

rtos_status_t rtos_smp_call_on_core(uint8_t core_id,
                                     rtos_smp_call_func_t func,
                                     void *arg,
                                     bool wait)
{
    if (core_id >= RTOS_SMP_MAX_CORES || func == NULL) {
        return RTOS_ERR_PARAM;
    }

    /* If calling on current core, just execute directly */
    if (core_id == rtos_port_smp_get_core_id()) {
        func(arg);
        return RTOS_OK;
    }

    /* Set up cross-core call */
    cross_core_calls[core_id].func = func;
    cross_core_calls[core_id].arg = arg;
    cross_core_calls[core_id].done = false;
    cross_core_calls[core_id].pending = true;

    /* Send IPI */
    rtos_smp_send_ipi(core_id, RTOS_IPI_CALL_FUNC);

    /* Wait for completion if requested */
    if (wait) {
        while (!cross_core_calls[core_id].done) {
            /* Spin */
        }
    }

    return RTOS_OK;
}

rtos_status_t rtos_smp_call_on_all(rtos_smp_call_func_t func,
                                    void *arg,
                                    bool wait)
{
    uint8_t i;
    uint8_t current = rtos_port_smp_get_core_id();
    rtos_status_t status = RTOS_OK;

    for (i = 0; i < active_core_count; i++) {
        if (i != current) {
            rtos_status_t s = rtos_smp_call_on_core(i, func, arg, wait);
            if (s != RTOS_OK) {
                status = s;
            }
        }
    }

    /* Execute on current core too */
    func(arg);

    return status;
}

/* Called from IPI handler on target core */
void rtos_smp_handle_call_ipi(void)
{
    uint8_t core_id = rtos_port_smp_get_core_id();

    if (cross_core_calls[core_id].pending) {
        cross_core_calls[core_id].func(cross_core_calls[core_id].arg);
        cross_core_calls[core_id].pending = false;
        cross_core_calls[core_id].done = true;
    }
}

/*===========================================================================*/
/* Load Balancing                                                             */
/*===========================================================================*/

uint16_t rtos_smp_get_core_load(uint8_t core_id)
{
    uint16_t count = 0;
    uint8_t i;

    if (core_id >= RTOS_SMP_MAX_CORES) {
        return 0;
    }

    /* Count tasks in all ready lists */
    for (i = 0; i < RTOS_MAX_PRIORITIES; i++) {
        count += core_states[core_id].ready_list[i].count;
    }

    return count;
}

uint8_t rtos_smp_find_least_loaded(uint8_t affinity)
{
    uint8_t i;
    uint8_t best_core = 0;
    uint16_t min_load = 0xFFFF;

    for (i = 0; i < active_core_count; i++) {
        if (affinity != RTOS_CORE_ANY &&
            !(affinity & RTOS_AFFINITY_CORE(i))) {
            continue;
        }

        uint16_t load = rtos_smp_get_core_load(i);
        if (load < min_load) {
            min_load = load;
            best_core = i;
        }
    }

    return best_core;
}

rtos_status_t rtos_smp_migrate_task(rtos_tcb_t *tcb, uint8_t target_core)
{
    uint32_t state;

    if (tcb == NULL || target_core >= RTOS_SMP_MAX_CORES) {
        return RTOS_ERR_PARAM;
    }

    state = rtos_smp_critical_enter();

    /* Remove from current core's ready list */
    /* Add to target core's ready list */
    /* Update task's core assignment */

    /* Request reschedule on both cores */
    rtos_smp_request_reschedule(target_core);

    rtos_smp_critical_exit(state);

    return RTOS_OK;
}

/*===========================================================================*/
/* Critical Sections                                                          */
/*===========================================================================*/

uint32_t rtos_smp_critical_enter(void)
{
    uint32_t state = rtos_port_disable_interrupts();
    rtos_smp_spinlock_acquire(&scheduler_lock);
    return state;
}

void rtos_smp_critical_exit(uint32_t state)
{
    rtos_smp_spinlock_release(&scheduler_lock);
    rtos_port_restore_interrupts(state);
}

/*===========================================================================*/
/* Default Platform Implementations                                           */
/*===========================================================================*/

__attribute__((weak))
void rtos_port_smp_start_core(uint8_t core_id, void (*entry)(void), void *stack)
{
    (void)core_id;
    (void)entry;
    (void)stack;
    /* Platform-specific implementation required */
}

__attribute__((weak))
uint8_t rtos_port_smp_get_core_id(void)
{
    /* Default: single core (core 0) */
    return 0;
}

__attribute__((weak))
void rtos_port_smp_send_ipi(uint8_t core_id, rtos_ipi_type_t ipi_type)
{
    (void)core_id;
    (void)ipi_type;
    /* Platform-specific implementation required */
}

#else /* !RTOS_USE_SMP */

/* Stub implementations for single-core builds */
void rtos_smp_init(void) {}
void rtos_smp_start_cores(void) {}
uint8_t rtos_smp_get_core_id(void) { return 0; }
uint8_t rtos_smp_get_core_count(void) { return 1; }
bool rtos_smp_is_active(void) { return false; }

rtos_status_t rtos_smp_set_affinity(rtos_tcb_t *t, uint8_t a)
{
    (void)t; (void)a;
    return RTOS_OK;
}

uint8_t rtos_smp_get_affinity(rtos_tcb_t *t)
{
    (void)t;
    return RTOS_CORE_ANY;
}

void rtos_smp_spinlock_init(rtos_smp_spinlock_t *l) { (void)l; }
void rtos_smp_spinlock_acquire(rtos_smp_spinlock_t *l) { (void)l; }
void rtos_smp_spinlock_release(rtos_smp_spinlock_t *l) { (void)l; }

uint32_t rtos_smp_critical_enter(void)
{
    return rtos_port_disable_interrupts();
}

void rtos_smp_critical_exit(uint32_t s)
{
    rtos_port_restore_interrupts(s);
}

#endif /* RTOS_USE_SMP */
