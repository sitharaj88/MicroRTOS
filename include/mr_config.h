/**
 * MicroRTOS - Configuration Header
 *
 * This file contains all configurable parameters for MicroRTOS.
 * Users can modify these values to customize the RTOS behavior
 * and optimize for their specific hardware and requirements.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef MR_CONFIG_H
#define MR_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Platform Selection                                                         */
/*===========================================================================*/

/**
 * Select the target platform. Uncomment one of the following:
 * - MR_PLATFORM_AVR: For ATmega328P, ATmega2560, etc.
 * - MR_PLATFORM_ARM: For ARM Cortex-M (Arduino Due, Zero, MKR)
 */
#if !defined(MR_PLATFORM_AVR) && !defined(MR_PLATFORM_ARM) && !defined(MR_PLATFORM_HOST)
    #if defined(__AVR__)
        #define MR_PLATFORM_AVR       1
    #elif defined(__arm__) || defined(__ARM_ARCH)
        #define MR_PLATFORM_ARM       1
    #else
        #error "Unsupported platform. Define MR_PLATFORM_AVR, MR_PLATFORM_ARM, or MR_PLATFORM_HOST"
    #endif
#endif

/*===========================================================================*/
/* Scheduler Configuration                                                    */
/*===========================================================================*/

/**
 * Maximum number of priority levels (1-32).
 * Priority 0 is the highest, (MR_MAX_PRIORITIES - 1) is the lowest.
 * Reducing this value saves RAM (1 ready list per priority level).
 */
#ifndef MR_MAX_PRIORITIES
    #define MR_MAX_PRIORITIES         8
#endif

/**
 * System tick rate in Hz.
 * Higher values provide finer timing resolution but increase overhead.
 * Typical values: 100Hz (10ms tick), 1000Hz (1ms tick)
 */
#ifndef MR_TICK_RATE_HZ
    #define MR_TICK_RATE_HZ           1000
#endif

/**
 * Enable time-slicing (round-robin) for equal priority tasks.
 * Set to 1 to enable, 0 to disable.
 */
#ifndef MR_USE_TIMESLICING
    #define MR_USE_TIMESLICING        1
#endif

/**
 * Time slice duration in ticks for round-robin scheduling.
 * Only used if MR_USE_TIMESLICING is enabled.
 */
#ifndef MR_TIMESLICE_TICKS
    #define MR_TIMESLICE_TICKS        10
#endif

/**
 * Enable preemptive scheduling.
 * Set to 1 for preemptive, 0 for purely cooperative.
 */
#ifndef MR_USE_PREEMPTION
    #define MR_USE_PREEMPTION         1
#endif

/*===========================================================================*/
/* Task Configuration                                                         */
/*===========================================================================*/

/**
 * Minimum stack size in bytes.
 * This is the absolute minimum for the simplest tasks.
 */
#ifndef MR_MINIMAL_STACK_SIZE
    #if MR_PLATFORM_AVR
        /*
         * Same ~100 B budget as MR_IDLE_STACK_SIZE: enough to absorb
         * the fake-ISR initial frame plus a real Timer0 ISR landing on
         * the same task.
         */
        #define MR_MINIMAL_STACK_SIZE     128
    #else
        #define MR_MINIMAL_STACK_SIZE     128
    #endif
#endif

/**
 * Idle task stack size in bytes.
 * The idle task runs when no other task is ready.
 */
#ifndef MR_IDLE_STACK_SIZE
    #if MR_PLATFORM_AVR
        /*
         * AVR sizing budget:
         *   ~35 B for the fake-ISR frame placed by mr_port_init_task_stack
         *   ~35 B for the Timer0 ISR's hardware PC push + SAVE_CONTEXT
         *   ~30 B for nested calls inside the tick handler (process_delays,
         *          timer_process, timeslice_tick)
         * 128 B leaves comfortable headroom; 48 B was demonstrably too small
         * (idle's stack overflowed on the very first tick).
         */
        #define MR_IDLE_STACK_SIZE        128
    #else
        #define MR_IDLE_STACK_SIZE        128
    #endif
#endif

/**
 * Enable task names for debugging.
 * Set to 0 to save RAM by removing task name pointers.
 */
#ifndef MR_USE_TASK_NAMES
    #define MR_USE_TASK_NAMES         1
#endif

/**
 * Enable stack overflow detection.
 * Methods: 0=disabled, 1=canary pattern, 2=full fill check
 */
#ifndef MR_CHECK_STACK_OVERFLOW
    #define MR_CHECK_STACK_OVERFLOW   1
#endif

/**
 * Stack canary pattern for overflow detection.
 */
#ifndef MR_STACK_CANARY_VALUE
    #define MR_STACK_CANARY_VALUE     0xDEADBEEF
#endif

/*===========================================================================*/
/* Feature Toggles                                                            */
/*===========================================================================*/

/**
 * Enable mutex support with priority inheritance.
 */
#ifndef MR_USE_MUTEXES
    #define MR_USE_MUTEXES            1
#endif

/**
 * Enable semaphore support (binary and counting).
 */
#ifndef MR_USE_SEMAPHORES
    #define MR_USE_SEMAPHORES         1
#endif

/**
 * Enable message queue support.
 */
#ifndef MR_USE_QUEUES
    #define MR_USE_QUEUES             1
#endif

/**
 * Enable software timer support.
 */
#ifndef MR_USE_TIMERS
    #define MR_USE_TIMERS             1
#endif

/**
 * Enable event group support.
 */
#ifndef MR_USE_EVENT_GROUPS
    #define MR_USE_EVENT_GROUPS       1
#endif

/**
 * Enable memory pool support.
 */
#ifndef MR_USE_MEMORY_POOLS
    #define MR_USE_MEMORY_POOLS       1
#endif

/**
 * Enable task notifications (lightweight signaling).
 */
#ifndef MR_USE_TASK_NOTIFICATIONS
    #define MR_USE_TASK_NOTIFICATIONS 1
#endif

/*===========================================================================*/
/* Advanced / Optional Features                                               */
/*===========================================================================*/
/*
 * These features add code and RAM cost and are disabled by default.
 * Flip to 1 here (or pass -DRTOS_USE_xxx=1 on the command line) to enable.
 * Each feature's public header also declares the same default as a fallback.
 */

/**
 * Enable Earliest-Deadline-First (EDF) scheduler extensions.
 * See include/mr_deadline.h.
 */
#ifndef MR_USE_EDF_SCHEDULER
    #define MR_USE_EDF_SCHEDULER      0
#endif

/**
 * Enable Memory Protection Unit (MPU) support.
 * ARM Cortex-M only. See include/mr_mpu.h.
 */
#ifndef MR_USE_MPU
    #define MR_USE_MPU                0
#endif

/**
 * Enable Symmetric Multi-Processing (SMP) support.
 * Multi-core targets only (ESP32, RP2040). See include/mr_smp.h.
 */
#ifndef MR_USE_SMP
    #define MR_USE_SMP                0
#endif

/**
 * Enable system-level hardware watchdog support.
 * See include/mr_watchdog.h.
 */
#ifndef MR_USE_WATCHDOG
    #define MR_USE_WATCHDOG           0
#endif

/**
 * Enable per-task software watchdog support.
 * See include/mr_watchdog.h.
 */
#ifndef MR_USE_TASK_WATCHDOG
    #define MR_USE_TASK_WATCHDOG      0
#endif

/**
 * Enable kernel event tracing (SystemView-style).
 * See include/mr_trace.h.
 */
#ifndef MR_USE_TRACING
    #define MR_USE_TRACING            0
#endif

/**
 * Enable zero-copy IPC buffers.
 * See include/mr_zerocopy.h.
 */
#ifndef MR_USE_ZEROCOPY
    #define MR_USE_ZEROCOPY           0
#endif

/*===========================================================================*/
/* Low Power / Tickless Configuration                                         */
/*===========================================================================*/

/**
 * Enable tickless idle mode for low power operation.
 * When enabled, the system suppresses tick interrupts during idle
 * to reduce power consumption.
 */
#ifndef MR_USE_TICKLESS_IDLE
    #define MR_USE_TICKLESS_IDLE      0
#endif

/**
 * Minimum number of ticks worth entering tickless sleep.
 * If the next wake time is less than this, normal idle is used.
 */
#ifndef MR_TICKLESS_MIN_TICKS
    #define MR_TICKLESS_MIN_TICKS     2
#endif

/**
 * Maximum number of ticks the system can sleep in tickless mode.
 * Limited by hardware timer capabilities.
 */
#ifndef MR_TICKLESS_MAX_TICKS
    #if MR_PLATFORM_AVR
        #define MR_TICKLESS_MAX_TICKS     1000    /* ~1 second max on AVR */
    #else
        #define MR_TICKLESS_MAX_TICKS     0xFFFFFF /* ~16M ticks on ARM */
    #endif
#endif

/**
 * Enable tickless idle hooks (pre-sleep and post-sleep callbacks).
 */
#ifndef MR_USE_TICKLESS_HOOKS
    #define MR_USE_TICKLESS_HOOKS     0
#endif

/*===========================================================================*/
/* Runtime Statistics                                                         */
/*===========================================================================*/

/**
 * Enable runtime statistics collection.
 * Tracks CPU usage per task. Adds overhead.
 */
#ifndef MR_USE_RUNTIME_STATS
    #define MR_USE_RUNTIME_STATS      1
#endif

/**
 * Enable idle time tracking.
 * Useful for power management and load monitoring.
 */
#ifndef MR_USE_IDLE_HOOK
    #define MR_USE_IDLE_HOOK          0
#endif

/**
 * Enable tick hook callback.
 * Called from the tick interrupt handler.
 */
#ifndef MR_USE_TICK_HOOK
    #define MR_USE_TICK_HOOK          0
#endif

/*===========================================================================*/
/* Timer Configuration                                                        */
/*===========================================================================*/

/**
 * Timer task priority.
 * Timer callbacks run in the context of this task.
 */
#ifndef MR_TIMER_TASK_PRIORITY
    #define MR_TIMER_TASK_PRIORITY    1
#endif

/**
 * Timer task stack size.
 */
#ifndef MR_TIMER_TASK_STACK_SIZE
    #if MR_PLATFORM_AVR
        #define MR_TIMER_TASK_STACK_SIZE  96
    #else
        #define MR_TIMER_TASK_STACK_SIZE  256
    #endif
#endif

/**
 * Maximum number of pending timer commands.
 */
#ifndef MR_TIMER_QUEUE_LENGTH
    #define MR_TIMER_QUEUE_LENGTH     8
#endif

/*===========================================================================*/
/* Debug Configuration                                                        */
/*===========================================================================*/

/**
 * Enable assertions for development.
 * Disable in production for smaller code size.
 */
#ifndef MR_USE_ASSERT
    #define MR_USE_ASSERT             1
#endif

/**
 * Enable trace output for debugging.
 */
#ifndef MR_USE_TRACE
    #define MR_USE_TRACE              0
#endif

/*===========================================================================*/
/* Timeout Values                                                             */
/*===========================================================================*/

/**
 * Value indicating no timeout (wait forever).
 */
#define MR_WAIT_FOREVER               ((uint32_t)0xFFFFFFFF)

/**
 * Value indicating no wait (return immediately).
 */
#define MR_NO_WAIT                    ((uint32_t)0)

/*===========================================================================*/
/* Platform-Specific Overrides                                                */
/*===========================================================================*/

#if MR_PLATFORM_AVR
    /* AVR-specific adjustments */
    #ifndef MR_CRITICAL_NESTING_DEPTH
        #define MR_CRITICAL_NESTING_DEPTH 8
    #endif
#endif

#if MR_PLATFORM_ARM
    /* ARM Cortex-M specific */
    #ifndef MR_MAX_SYSCALL_PRIORITY
        #define MR_MAX_SYSCALL_PRIORITY   5
    #endif
#endif

/*===========================================================================*/
/* Compile-Time Validation                                                    */
/*===========================================================================*/

#if MR_MAX_PRIORITIES < 1 || MR_MAX_PRIORITIES > 32
    #error "MR_MAX_PRIORITIES must be between 1 and 32"
#endif

#if MR_TICK_RATE_HZ < 1 || MR_TICK_RATE_HZ > 10000
    #error "MR_TICK_RATE_HZ must be between 1 and 10000"
#endif

#if MR_MINIMAL_STACK_SIZE < 32
    #error "MR_MINIMAL_STACK_SIZE must be at least 32 bytes"
#endif

#ifdef __cplusplus
}
#endif

#endif /* MR_CONFIG_H */
