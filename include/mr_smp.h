/**
 * MicroRTOS - Symmetric Multi-Processing (SMP) Support
 *
 * Multi-core scheduling for dual-core processors like ESP32 and RP2040.
 * Provides per-core scheduler state, spinlocks, and task affinity.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef MR_SMP_H
#define MR_SMP_H

#include "mr_config.h"
#include "mr_types.h"
#include "mr_atomic.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Configuration                                                              */
/*===========================================================================*/

#ifndef MR_USE_SMP
#define MR_USE_SMP                0
#endif

#ifndef MR_SMP_MAX_CORES
#define MR_SMP_MAX_CORES          2       /* Typically 2 for embedded */
#endif

/*===========================================================================*/
/* Core Affinity                                                              */
/*===========================================================================*/

#define MR_CORE_ANY               0xFF    /* Run on any core */
#define MR_CORE_0                 0       /* Pin to core 0 */
#define MR_CORE_1                 1       /* Pin to core 1 */

/* Affinity mask macros */
#define MR_AFFINITY_CORE(n)       (1U << (n))
#define MR_AFFINITY_ALL           ((1U << MR_SMP_MAX_CORES) - 1)

/*===========================================================================*/
/* Spinlock Types                                                             */
/*===========================================================================*/

/**
 * SMP spinlock with owner tracking.
 */
typedef struct mr_smp_spinlock {
    mr_spinlock_t     lock;           /* Base spinlock */
    volatile uint8_t    owner_core;     /* Owning core (for debugging) */
    volatile uint8_t    nesting;        /* Nesting depth */
    volatile uint32_t   irq_state;      /* Saved interrupt state */
} mr_smp_spinlock_t;

#define MR_SMP_SPINLOCK_INIT      { MR_SPINLOCK_INIT, 0xFF, 0, 0 }

/*===========================================================================*/
/* Per-Core State                                                             */
/*===========================================================================*/

/**
 * Per-core scheduler state.
 */
typedef struct mr_core_state {
    mr_tcb_t          *current_tcb;       /* Currently running task */
    mr_tcb_t          *idle_tcb;          /* This core's idle task */
    mr_list_t         ready_list[MR_MAX_PRIORITIES];
    uint32_t            ready_bitmap;       /* Priority bitmap */
    uint8_t             core_id;            /* Core identifier */
    volatile bool       running;            /* Core is running scheduler */
    volatile bool       yield_pending;      /* Context switch needed */
    volatile uint32_t   irq_nesting;        /* IRQ nesting depth */
    uint32_t            idle_ticks;         /* Ticks spent in idle */
    uint32_t            context_switches;   /* Number of context switches */
} mr_core_state_t;

/*===========================================================================*/
/* SMP Initialization                                                         */
/*===========================================================================*/

/**
 * Initialize SMP support.
 * Called early in boot, before scheduler starts.
 */
void mr_smp_init(void);

/**
 * Start scheduler on secondary cores.
 * Called after primary core has initialized kernel.
 */
void mr_smp_start_cores(void);

/**
 * Entry point for secondary cores.
 * Platform-specific startup calls this.
 */
void mr_smp_core_entry(void);

/*===========================================================================*/
/* Core Identification                                                        */
/*===========================================================================*/

/**
 * Get the ID of the current core.
 *
 * @return  Core ID (0 to MR_SMP_MAX_CORES-1)
 */
uint8_t mr_smp_get_core_id(void);

/**
 * Get the number of active cores.
 *
 * @return  Number of cores
 */
uint8_t mr_smp_get_core_count(void);

/**
 * Check if SMP is active (multiple cores running).
 *
 * @return  true if SMP is active
 */
bool mr_smp_is_active(void);

/**
 * Get per-core state structure.
 *
 * @param core_id   Core ID
 * @return          Pointer to core state
 */
mr_core_state_t *mr_smp_get_core_state(uint8_t core_id);

/**
 * Get current core's state.
 *
 * @return  Pointer to current core state
 */
mr_core_state_t *mr_smp_get_current_core(void);

/*===========================================================================*/
/* Task Affinity                                                              */
/*===========================================================================*/

/**
 * Set task core affinity.
 *
 * @param tcb           Task to configure
 * @param affinity      Core mask (MR_AFFINITY_CORE() or MR_AFFINITY_ALL)
 * @return              MR_OK on success
 */
mr_status_t mr_smp_set_affinity(mr_tcb_t *tcb, uint8_t affinity);

/**
 * Get task core affinity.
 *
 * @param tcb           Task to query
 * @return              Affinity mask
 */
uint8_t mr_smp_get_affinity(mr_tcb_t *tcb);

/**
 * Check if task can run on a specific core.
 *
 * @param tcb           Task to check
 * @param core_id       Core to check against
 * @return              true if task can run on core
 */
bool mr_smp_can_run_on_core(mr_tcb_t *tcb, uint8_t core_id);

/*===========================================================================*/
/* SMP Spinlock API                                                           */
/*===========================================================================*/

/**
 * Initialize an SMP spinlock.
 *
 * @param lock          Spinlock to initialize
 */
void mr_smp_spinlock_init(mr_smp_spinlock_t *lock);

/**
 * Acquire SMP spinlock (disables interrupts).
 *
 * @param lock          Spinlock to acquire
 */
void mr_smp_spinlock_acquire(mr_smp_spinlock_t *lock);

/**
 * Release SMP spinlock (restores interrupts).
 *
 * @param lock          Spinlock to release
 */
void mr_smp_spinlock_release(mr_smp_spinlock_t *lock);

/**
 * Try to acquire SMP spinlock without blocking.
 *
 * @param lock          Spinlock to try
 * @return              true if acquired, false if contended
 */
bool mr_smp_spinlock_try_acquire(mr_smp_spinlock_t *lock);

/*===========================================================================*/
/* Inter-Processor Interrupt (IPI)                                            */
/*===========================================================================*/

/**
 * IPI types.
 */
typedef enum {
    MR_IPI_RESCHEDULE = 0,        /* Request reschedule */
    MR_IPI_CALL_FUNC,             /* Call function on core */
    MR_IPI_HALT,                  /* Halt core (debugging) */
} mr_ipi_type_t;

/**
 * Send IPI to a specific core.
 *
 * @param core_id       Target core
 * @param ipi_type      Type of IPI
 */
void mr_smp_send_ipi(uint8_t core_id, mr_ipi_type_t ipi_type);

/**
 * Send IPI to all other cores.
 *
 * @param ipi_type      Type of IPI
 */
void mr_smp_send_ipi_all(mr_ipi_type_t ipi_type);

/**
 * Request reschedule on a specific core.
 *
 * @param core_id       Target core
 */
void mr_smp_request_reschedule(uint8_t core_id);

/*===========================================================================*/
/* Cross-Core Function Calls                                                  */
/*===========================================================================*/

/**
 * Function to call on remote core.
 */
typedef void (*mr_smp_call_func_t)(void *arg);

/**
 * Call a function on a specific core.
 *
 * @param core_id       Target core
 * @param func          Function to call
 * @param arg           Argument to pass
 * @param wait          true to wait for completion
 * @return              MR_OK on success
 */
mr_status_t mr_smp_call_on_core(uint8_t core_id,
                                     mr_smp_call_func_t func,
                                     void *arg,
                                     bool wait);

/**
 * Call a function on all cores.
 *
 * @param func          Function to call
 * @param arg           Argument to pass
 * @param wait          true to wait for completion
 * @return              MR_OK on success
 */
mr_status_t mr_smp_call_on_all(mr_smp_call_func_t func,
                                    void *arg,
                                    bool wait);

/*===========================================================================*/
/* Load Balancing                                                             */
/*===========================================================================*/

/**
 * Get task load on a core.
 *
 * @param core_id       Core to query
 * @return              Number of ready tasks
 */
uint16_t mr_smp_get_core_load(uint8_t core_id);

/**
 * Find the least loaded core for a new task.
 *
 * @param affinity      Affinity constraint
 * @return              Core ID to use
 */
uint8_t mr_smp_find_least_loaded(uint8_t affinity);

/**
 * Migrate a task to a different core.
 *
 * @param tcb           Task to migrate
 * @param target_core   Target core
 * @return              MR_OK on success
 */
mr_status_t mr_smp_migrate_task(mr_tcb_t *tcb, uint8_t target_core);

/*===========================================================================*/
/* Critical Sections (SMP-safe)                                               */
/*===========================================================================*/

/**
 * Enter SMP-safe critical section.
 * Disables interrupts and acquires global scheduler lock.
 *
 * @return  State for exit
 */
uint32_t mr_smp_critical_enter(void);

/**
 * Exit SMP-safe critical section.
 *
 * @param state     State from enter
 */
void mr_smp_critical_exit(uint32_t state);

/*===========================================================================*/
/* Platform-Specific Functions (implement per platform)                       */
/*===========================================================================*/

/**
 * Start a secondary core.
 *
 * @param core_id       Core to start
 * @param entry         Entry function
 * @param stack         Stack pointer
 */
extern void mr_port_smp_start_core(uint8_t core_id,
                                      void (*entry)(void),
                                      void *stack);

/**
 * Get the current core ID (platform-specific).
 *
 * @return  Core ID
 */
extern uint8_t mr_port_smp_get_core_id(void);

/**
 * Send IPI to a core (platform-specific).
 *
 * @param core_id       Target core
 * @param ipi_type      IPI type
 */
extern void mr_port_smp_send_ipi(uint8_t core_id, mr_ipi_type_t ipi_type);

#ifdef __cplusplus
}
#endif

#endif /* MR_SMP_H */
