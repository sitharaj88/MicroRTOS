/**
 * MicroRTOS - Platform Abstraction Layer Interface
 *
 * This file defines the interface that must be implemented by
 * each platform port (AVR, ARM, etc.).
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef RTOS_PORT_H
#define RTOS_PORT_H

#include "rtos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Port Initialization                                                        */
/*===========================================================================*/

/**
 * Initialize platform-specific hardware.
 *
 * Sets up the system tick timer and any other platform-specific
 * initialization required before the scheduler starts.
 */
void rtos_port_init(void);

/**
 * Start the scheduler and run the first task.
 *
 * This function does not return. It performs the initial context
 * load and starts executing the first task.
 */
void rtos_port_start_scheduler(void);

/*===========================================================================*/
/* Context Switching                                                          */
/*===========================================================================*/

/**
 * Trigger a context switch.
 *
 * This is typically implemented as a software interrupt or
 * pending a PendSV (on ARM).
 */
void rtos_port_yield(void);

/**
 * Initialize a task's stack with initial context.
 *
 * Sets up the stack so that when the task is first switched to,
 * it will begin executing at the entry function with the given argument.
 *
 * @param tcb   Pointer to task control block
 * @param entry Task entry function
 * @param arg   Argument to pass to entry function
 */
void rtos_port_init_task_stack(rtos_tcb_t *tcb, void (*entry)(void*), void *arg);

/*===========================================================================*/
/* Interrupt Control                                                          */
/*===========================================================================*/

/**
 * Enter a critical section (disable interrupts).
 *
 * This function supports nesting - interrupts are only re-enabled
 * when the nesting count reaches zero.
 */
void rtos_port_enter_critical(void);

/**
 * Exit a critical section (re-enable interrupts).
 */
void rtos_port_exit_critical(void);

/**
 * Disable interrupts and return previous state.
 *
 * @return Previous interrupt state (for restoration)
 */
uint32_t rtos_port_disable_interrupts(void);

/**
 * Restore interrupts to a previous state.
 *
 * @param state Previous state from rtos_port_disable_interrupts
 */
void rtos_port_restore_interrupts(uint32_t state);

/**
 * Check if currently executing in ISR context.
 *
 * @return true if in ISR, false if in thread context
 */
bool rtos_port_is_in_isr(void);

/*===========================================================================*/
/* Stack Overflow Checking                                                    */
/*===========================================================================*/

#if RTOS_CHECK_STACK_OVERFLOW

/**
 * Check for stack overflow on the current task.
 *
 * Called during context switch. If overflow is detected,
 * calls rtos_stack_overflow_hook().
 */
void rtos_port_check_stack_overflow(void);

#endif

/*===========================================================================*/
/* Platform-Specific Includes                                                 */
/*===========================================================================*/

#if defined(RTOS_PLATFORM_HOST) && RTOS_PLATFORM_HOST
    /* Host build (unit tests). Pull in no chip headers. */
    #define RTOS_INTERRUPT_STATE_TYPE   uint32_t

#elif defined(RTOS_PLATFORM_AVR) && RTOS_PLATFORM_AVR
    /* AVR-specific declarations */
    #include <avr/io.h>
    #include <avr/interrupt.h>

    /* AVR uses 8-bit status register */
    #define RTOS_INTERRUPT_STATE_TYPE   uint8_t

#elif defined(RTOS_PLATFORM_ARM) && RTOS_PLATFORM_ARM
    /* ARM Cortex-M specific declarations */

    /* ARM uses 32-bit PRIMASK/BASEPRI */
    #define RTOS_INTERRUPT_STATE_TYPE   uint32_t

    /**
     * ARM-specific: Set BASEPRI to mask interrupts below threshold.
     */
    void rtos_port_set_basepri(uint32_t basepri);

    /**
     * ARM-specific: Get current BASEPRI value.
     */
    uint32_t rtos_port_get_basepri(void);

#else
    #define RTOS_INTERRUPT_STATE_TYPE   uint32_t
#endif

/*===========================================================================*/
/* CPU Frequency (for timing calculations)                                    */
/*===========================================================================*/

#ifndef F_CPU
    #if defined(RTOS_PLATFORM_AVR) && RTOS_PLATFORM_AVR
        #define F_CPU   16000000UL  /* 16 MHz default for Arduino */
    #elif defined(RTOS_PLATFORM_ARM) && RTOS_PLATFORM_ARM
        #define F_CPU   84000000UL  /* 84 MHz default for Arduino Due */
    #else
        #define F_CPU   16000000UL
    #endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* RTOS_PORT_H */
