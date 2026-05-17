/**
 * MicroRTOS - SMP Implementation
 *
 * Symmetric Multi-Processing support for multi-core systems.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_smp.h"
#include "mr_types.h"
#include "mr_list.h"
#include "mr_atomic.h"
#include "mr_port.h"

#if MR_USE_SMP

/*===========================================================================*/
/* External References                                                        */
/*===========================================================================*/

extern volatile uint32_t mr_tick_count;
extern void mr_port_yield(void);

/*===========================================================================*/
/* Module State                                                               */
/*===========================================================================*/

/* Per-core state */
static mr_core_state_t core_states[MR_SMP_MAX_CORES];

/* Global scheduler lock for SMP */
static mr_smp_spinlock_t scheduler_lock = MR_SMP_SPINLOCK_INIT;

/* Number of active cores */
static volatile uint8_t active_core_count = 1;

/* SMP initialized flag */
static volatile bool smp_initialized = false;

/*===========================================================================*/
/* SMP Initialization                                                         */
/*===========================================================================*/

void mr_smp_init(void)
{
    uint8_t i, j;

    for (i = 0; i < MR_SMP_MAX_CORES; i++) {
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
        for (j = 0; j < MR_MAX_PRIORITIES; j++) {
            mr_list_init(&core_states[i].ready_list[j]);
        }
    }

    mr_smp_spinlock_init(&scheduler_lock);
    smp_initialized = true;
}

void mr_smp_start_cores(void)
{
    uint8_t i;

    /* Start secondary cores */
    for (i = 1; i < MR_SMP_MAX_CORES; i++) {
        /* Platform-specific core startup */
        mr_port_smp_start_core(i, mr_smp_core_entry, NULL);
        active_core_count++;
    }
}

void mr_smp_core_entry(void)
{
    uint8_t core_id = mr_port_smp_get_core_id();

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

uint8_t mr_smp_get_core_id(void)
{
    return mr_port_smp_get_core_id();
}

uint8_t mr_smp_get_core_count(void)
{
    return active_core_count;
}

bool mr_smp_is_active(void)
{
    return (smp_initialized && active_core_count > 1);
}

mr_core_state_t *mr_smp_get_core_state(uint8_t core_id)
{
    if (core_id >= MR_SMP_MAX_CORES) {
        return NULL;
    }
    return &core_states[core_id];
}

mr_core_state_t *mr_smp_get_current_core(void)
{
    return &core_states[mr_port_smp_get_core_id()];
}

/*===========================================================================*/
/* Task Affinity                                                              */
/*===========================================================================*/

mr_status_t mr_smp_set_affinity(mr_tcb_t *tcb, uint8_t affinity)
{
    uint32_t state;

    if (tcb == NULL) {
        return MR_ERR_PARAM;
    }

    state = mr_smp_critical_enter();

    /* Store affinity in TCB (would need to add field) */
    /* tcb->core_affinity = affinity; */

    mr_smp_critical_exit(state);

    return MR_OK;
}

uint8_t mr_smp_get_affinity(mr_tcb_t *tcb)
{
    if (tcb == NULL) {
        return MR_AFFINITY_ALL;
    }

    /* Return affinity from TCB */
    /* return tcb->core_affinity; */
    return MR_AFFINITY_ALL;
}

bool mr_smp_can_run_on_core(mr_tcb_t *tcb, uint8_t core_id)
{
    uint8_t affinity;

    if (tcb == NULL || core_id >= MR_SMP_MAX_CORES) {
        return false;
    }

    affinity = mr_smp_get_affinity(tcb);

    if (affinity == MR_CORE_ANY) {
        return true;
    }

    return (affinity & MR_AFFINITY_CORE(core_id)) != 0;
}

/*===========================================================================*/
/* SMP Spinlock Implementation                                                */
/*===========================================================================*/

void mr_smp_spinlock_init(mr_smp_spinlock_t *lock)
{
    if (lock == NULL) {
        return;
    }

    lock->lock.value = 0;
    lock->owner_core = 0xFF;
    lock->nesting = 0;
    lock->irq_state = 0;
}

void mr_smp_spinlock_acquire(mr_smp_spinlock_t *lock)
{
    uint32_t irq_state;
    uint8_t core_id;

    if (lock == NULL) {
        return;
    }

    /* Disable interrupts first */
    irq_state = mr_port_disable_interrupts();
    core_id = mr_port_smp_get_core_id();

    /* Check for recursive lock by same core */
    if (lock->owner_core == core_id) {
        lock->nesting++;
        return;
    }

    /* Acquire the spinlock */
    mr_spinlock_acquire(&lock->lock);

    /* Record ownership */
    lock->owner_core = core_id;
    lock->nesting = 1;
    lock->irq_state = irq_state;
}

void mr_smp_spinlock_release(mr_smp_spinlock_t *lock)
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
        mr_spinlock_release(&lock->lock);

        /* Restore interrupts */
        mr_port_restore_interrupts(irq_state);
    }
}

bool mr_smp_spinlock_try_acquire(mr_smp_spinlock_t *lock)
{
    uint32_t irq_state;
    uint8_t core_id;

    if (lock == NULL) {
        return false;
    }

    irq_state = mr_port_disable_interrupts();
    core_id = mr_port_smp_get_core_id();

    /* Check for recursive lock */
    if (lock->owner_core == core_id) {
        lock->nesting++;
        return true;
    }

    /* Try to acquire */
    if (mr_spinlock_try_acquire(&lock->lock)) {
        lock->owner_core = core_id;
        lock->nesting = 1;
        lock->irq_state = irq_state;
        return true;
    }

    /* Failed - restore interrupts */
    mr_port_restore_interrupts(irq_state);
    return false;
}

/*===========================================================================*/
/* Inter-Processor Interrupt                                                  */
/*===========================================================================*/

void mr_smp_send_ipi(uint8_t core_id, mr_ipi_type_t ipi_type)
{
    if (core_id >= MR_SMP_MAX_CORES) {
        return;
    }

    mr_port_smp_send_ipi(core_id, ipi_type);
}

void mr_smp_send_ipi_all(mr_ipi_type_t ipi_type)
{
    uint8_t i;
    uint8_t current = mr_port_smp_get_core_id();

    for (i = 0; i < active_core_count; i++) {
        if (i != current) {
            mr_port_smp_send_ipi(i, ipi_type);
        }
    }
}

void mr_smp_request_reschedule(uint8_t core_id)
{
    if (core_id >= MR_SMP_MAX_CORES) {
        return;
    }

    core_states[core_id].yield_pending = true;
    mr_smp_send_ipi(core_id, MR_IPI_RESCHEDULE);
}

/*===========================================================================*/
/* Cross-Core Function Calls                                                  */
/*===========================================================================*/

/* Pending call information */
static volatile struct {
    mr_smp_call_func_t func;
    void *arg;
    volatile bool pending;
    volatile bool done;
} cross_core_calls[MR_SMP_MAX_CORES];

mr_status_t mr_smp_call_on_core(uint8_t core_id,
                                     mr_smp_call_func_t func,
                                     void *arg,
                                     bool wait)
{
    if (core_id >= MR_SMP_MAX_CORES || func == NULL) {
        return MR_ERR_PARAM;
    }

    /* If calling on current core, just execute directly */
    if (core_id == mr_port_smp_get_core_id()) {
        func(arg);
        return MR_OK;
    }

    /* Set up cross-core call */
    cross_core_calls[core_id].func = func;
    cross_core_calls[core_id].arg = arg;
    cross_core_calls[core_id].done = false;
    cross_core_calls[core_id].pending = true;

    /* Send IPI */
    mr_smp_send_ipi(core_id, MR_IPI_CALL_FUNC);

    /* Wait for completion if requested */
    if (wait) {
        while (!cross_core_calls[core_id].done) {
            /* Spin */
        }
    }

    return MR_OK;
}

mr_status_t mr_smp_call_on_all(mr_smp_call_func_t func,
                                    void *arg,
                                    bool wait)
{
    uint8_t i;
    uint8_t current = mr_port_smp_get_core_id();
    mr_status_t status = MR_OK;

    for (i = 0; i < active_core_count; i++) {
        if (i != current) {
            mr_status_t s = mr_smp_call_on_core(i, func, arg, wait);
            if (s != MR_OK) {
                status = s;
            }
        }
    }

    /* Execute on current core too */
    func(arg);

    return status;
}

/* Called from IPI handler on target core */
void mr_smp_handle_call_ipi(void)
{
    uint8_t core_id = mr_port_smp_get_core_id();

    if (cross_core_calls[core_id].pending) {
        cross_core_calls[core_id].func(cross_core_calls[core_id].arg);
        cross_core_calls[core_id].pending = false;
        cross_core_calls[core_id].done = true;
    }
}

/*===========================================================================*/
/* Load Balancing                                                             */
/*===========================================================================*/

uint16_t mr_smp_get_core_load(uint8_t core_id)
{
    uint16_t count = 0;
    uint8_t i;

    if (core_id >= MR_SMP_MAX_CORES) {
        return 0;
    }

    /* Count tasks in all ready lists */
    for (i = 0; i < MR_MAX_PRIORITIES; i++) {
        count += core_states[core_id].ready_list[i].count;
    }

    return count;
}

uint8_t mr_smp_find_least_loaded(uint8_t affinity)
{
    uint8_t i;
    uint8_t best_core = 0;
    uint16_t min_load = 0xFFFF;

    for (i = 0; i < active_core_count; i++) {
        if (affinity != MR_CORE_ANY &&
            !(affinity & MR_AFFINITY_CORE(i))) {
            continue;
        }

        uint16_t load = mr_smp_get_core_load(i);
        if (load < min_load) {
            min_load = load;
            best_core = i;
        }
    }

    return best_core;
}

mr_status_t mr_smp_migrate_task(mr_tcb_t *tcb, uint8_t target_core)
{
    uint32_t state;

    if (tcb == NULL || target_core >= MR_SMP_MAX_CORES) {
        return MR_ERR_PARAM;
    }

    state = mr_smp_critical_enter();

    /* Remove from current core's ready list */
    /* Add to target core's ready list */
    /* Update task's core assignment */

    /* Request reschedule on both cores */
    mr_smp_request_reschedule(target_core);

    mr_smp_critical_exit(state);

    return MR_OK;
}

/*===========================================================================*/
/* Critical Sections                                                          */
/*===========================================================================*/

uint32_t mr_smp_critical_enter(void)
{
    uint32_t state = mr_port_disable_interrupts();
    mr_smp_spinlock_acquire(&scheduler_lock);
    return state;
}

void mr_smp_critical_exit(uint32_t state)
{
    mr_smp_spinlock_release(&scheduler_lock);
    mr_port_restore_interrupts(state);
}

/*===========================================================================*/
/* Default Platform Implementations                                           */
/*===========================================================================*/

__attribute__((weak))
void mr_port_smp_start_core(uint8_t core_id, void (*entry)(void), void *stack)
{
    (void)core_id;
    (void)entry;
    (void)stack;
    /* Platform-specific implementation required */
}

__attribute__((weak))
uint8_t mr_port_smp_get_core_id(void)
{
    /* Default: single core (core 0) */
    return 0;
}

__attribute__((weak))
void mr_port_smp_send_ipi(uint8_t core_id, mr_ipi_type_t ipi_type)
{
    (void)core_id;
    (void)ipi_type;
    /* Platform-specific implementation required */
}

#else /* !MR_USE_SMP */

/* Stub implementations for single-core builds */
void mr_smp_init(void) {}
void mr_smp_start_cores(void) {}
uint8_t mr_smp_get_core_id(void) { return 0; }
uint8_t mr_smp_get_core_count(void) { return 1; }
bool mr_smp_is_active(void) { return false; }

mr_status_t mr_smp_set_affinity(mr_tcb_t *t, uint8_t a)
{
    (void)t; (void)a;
    return MR_OK;
}

uint8_t mr_smp_get_affinity(mr_tcb_t *t)
{
    (void)t;
    return MR_CORE_ANY;
}

void mr_smp_spinlock_init(mr_smp_spinlock_t *l) { (void)l; }
void mr_smp_spinlock_acquire(mr_smp_spinlock_t *l) { (void)l; }
void mr_smp_spinlock_release(mr_smp_spinlock_t *l) { (void)l; }

uint32_t mr_smp_critical_enter(void)
{
    return mr_port_disable_interrupts();
}

void mr_smp_critical_exit(uint32_t s)
{
    mr_port_restore_interrupts(s);
}

#endif /* MR_USE_SMP */
