/**
 * MicroRTOS - Atomic Operations
 *
 * Platform-abstracted atomic operations for lock-free programming.
 * Provides memory barriers and atomic read-modify-write operations.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef RTOS_ATOMIC_H
#define RTOS_ATOMIC_H

#include "rtos_config.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Memory Ordering                                                            */
/*===========================================================================*/

typedef enum {
    RTOS_MEMORY_ORDER_RELAXED = 0,
    RTOS_MEMORY_ORDER_ACQUIRE = 1,
    RTOS_MEMORY_ORDER_RELEASE = 2,
    RTOS_MEMORY_ORDER_ACQ_REL = 3,
    RTOS_MEMORY_ORDER_SEQ_CST = 4,
} rtos_memory_order_t;

/*===========================================================================*/
/* Memory Barriers                                                            */
/*===========================================================================*/

/**
 * Full memory barrier (compiler + hardware).
 * Prevents reordering of memory operations across the barrier.
 */
static inline void rtos_memory_barrier(void)
{
#if defined(__GNUC__)
    #if defined(__arm__) || defined(__ARM_ARCH)
        __asm__ __volatile__("dmb" ::: "memory");
    #elif defined(__AVR__)
        __asm__ __volatile__("" ::: "memory");
    #else
        __sync_synchronize();
    #endif
#else
    /* Compiler barrier only for unknown compilers */
#endif
}

/**
 * Compiler-only barrier (prevents compiler reordering).
 */
static inline void rtos_compiler_barrier(void)
{
#if defined(__GNUC__)
    __asm__ __volatile__("" ::: "memory");
#endif
}

/**
 * Data synchronization barrier (ARM).
 */
static inline void rtos_dsb(void)
{
#if defined(__arm__) || defined(__ARM_ARCH)
    __asm__ __volatile__("dsb" ::: "memory");
#else
    rtos_memory_barrier();
#endif
}

/**
 * Instruction synchronization barrier (ARM).
 */
static inline void rtos_isb(void)
{
#if defined(__arm__) || defined(__ARM_ARCH)
    __asm__ __volatile__("isb" ::: "memory");
#endif
}

/*===========================================================================*/
/* Atomic Load/Store (32-bit)                                                 */
/*===========================================================================*/

/**
 * Atomic load with acquire semantics.
 */
static inline uint32_t rtos_atomic_load(volatile uint32_t *ptr)
{
#if defined(__GNUC__) && (__GNUC__ >= 5 || defined(__clang__))
    return __atomic_load_n(ptr, __ATOMIC_ACQUIRE);
#elif defined(__GNUC__)
    uint32_t val = *ptr;
    rtos_compiler_barrier();
    return val;
#else
    return *ptr;
#endif
}

/**
 * Atomic load with explicit memory ordering.
 */
static inline uint32_t rtos_atomic_load_explicit(volatile uint32_t *ptr,
                                                  rtos_memory_order_t order)
{
#if defined(__GNUC__) && (__GNUC__ >= 5 || defined(__clang__))
    int gcc_order = (order == RTOS_MEMORY_ORDER_RELAXED) ? __ATOMIC_RELAXED :
                    (order == RTOS_MEMORY_ORDER_ACQUIRE) ? __ATOMIC_ACQUIRE :
                    __ATOMIC_SEQ_CST;
    return __atomic_load_n(ptr, gcc_order);
#else
    (void)order;
    return rtos_atomic_load(ptr);
#endif
}

/**
 * Atomic store with release semantics.
 */
static inline void rtos_atomic_store(volatile uint32_t *ptr, uint32_t val)
{
#if defined(__GNUC__) && (__GNUC__ >= 5 || defined(__clang__))
    __atomic_store_n(ptr, val, __ATOMIC_RELEASE);
#elif defined(__GNUC__)
    rtos_compiler_barrier();
    *ptr = val;
#else
    *ptr = val;
#endif
}

/**
 * Atomic store with explicit memory ordering.
 */
static inline void rtos_atomic_store_explicit(volatile uint32_t *ptr,
                                               uint32_t val,
                                               rtos_memory_order_t order)
{
#if defined(__GNUC__) && (__GNUC__ >= 5 || defined(__clang__))
    int gcc_order = (order == RTOS_MEMORY_ORDER_RELAXED) ? __ATOMIC_RELAXED :
                    (order == RTOS_MEMORY_ORDER_RELEASE) ? __ATOMIC_RELEASE :
                    __ATOMIC_SEQ_CST;
    __atomic_store_n(ptr, val, gcc_order);
#else
    (void)order;
    rtos_atomic_store(ptr, val);
#endif
}

/*===========================================================================*/
/* Atomic Exchange                                                            */
/*===========================================================================*/

/**
 * Atomic exchange - swap value and return old value.
 */
static inline uint32_t rtos_atomic_exchange(volatile uint32_t *ptr, uint32_t val)
{
#if defined(__GNUC__) && (__GNUC__ >= 5 || defined(__clang__))
    return __atomic_exchange_n(ptr, val, __ATOMIC_ACQ_REL);
#elif defined(__GNUC__)
    return __sync_lock_test_and_set(ptr, val);
#else
    uint32_t old = *ptr;
    *ptr = val;
    return old;
#endif
}

/*===========================================================================*/
/* Atomic Compare-and-Swap (CAS)                                              */
/*===========================================================================*/

/**
 * Atomic compare and exchange (strong).
 *
 * If *ptr == *expected, atomically sets *ptr = desired and returns true.
 * Otherwise, sets *expected = *ptr and returns false.
 */
static inline bool rtos_atomic_compare_exchange(volatile uint32_t *ptr,
                                                 uint32_t *expected,
                                                 uint32_t desired)
{
#if defined(__GNUC__) && (__GNUC__ >= 5 || defined(__clang__))
    return __atomic_compare_exchange_n(ptr, expected, desired, false,
                                       __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
#elif defined(__GNUC__)
    uint32_t old = __sync_val_compare_and_swap(ptr, *expected, desired);
    if (old == *expected) {
        return true;
    }
    *expected = old;
    return false;
#else
    if (*ptr == *expected) {
        *ptr = desired;
        return true;
    }
    *expected = *ptr;
    return false;
#endif
}

/**
 * Atomic compare and exchange (weak).
 * May spuriously fail even if *ptr == *expected. Use in loops.
 */
static inline bool rtos_atomic_compare_exchange_weak(volatile uint32_t *ptr,
                                                      uint32_t *expected,
                                                      uint32_t desired)
{
#if defined(__GNUC__) && (__GNUC__ >= 5 || defined(__clang__))
    return __atomic_compare_exchange_n(ptr, expected, desired, true,
                                       __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
#else
    return rtos_atomic_compare_exchange(ptr, expected, desired);
#endif
}

/*===========================================================================*/
/* Atomic Arithmetic Operations                                               */
/*===========================================================================*/

/**
 * Atomic fetch-and-add. Returns old value.
 */
static inline uint32_t rtos_atomic_fetch_add(volatile uint32_t *ptr, uint32_t val)
{
#if defined(__GNUC__) && (__GNUC__ >= 5 || defined(__clang__))
    return __atomic_fetch_add(ptr, val, __ATOMIC_ACQ_REL);
#elif defined(__GNUC__)
    return __sync_fetch_and_add(ptr, val);
#else
    uint32_t old = *ptr;
    *ptr += val;
    return old;
#endif
}

/**
 * Atomic fetch-and-subtract. Returns old value.
 */
static inline uint32_t rtos_atomic_fetch_sub(volatile uint32_t *ptr, uint32_t val)
{
#if defined(__GNUC__) && (__GNUC__ >= 5 || defined(__clang__))
    return __atomic_fetch_sub(ptr, val, __ATOMIC_ACQ_REL);
#elif defined(__GNUC__)
    return __sync_fetch_and_sub(ptr, val);
#else
    uint32_t old = *ptr;
    *ptr -= val;
    return old;
#endif
}

/**
 * Atomic add-and-fetch. Returns new value.
 */
static inline uint32_t rtos_atomic_add_fetch(volatile uint32_t *ptr, uint32_t val)
{
#if defined(__GNUC__) && (__GNUC__ >= 5 || defined(__clang__))
    return __atomic_add_fetch(ptr, val, __ATOMIC_ACQ_REL);
#elif defined(__GNUC__)
    return __sync_add_and_fetch(ptr, val);
#else
    *ptr += val;
    return *ptr;
#endif
}

/**
 * Atomic subtract-and-fetch. Returns new value.
 */
static inline uint32_t rtos_atomic_sub_fetch(volatile uint32_t *ptr, uint32_t val)
{
#if defined(__GNUC__) && (__GNUC__ >= 5 || defined(__clang__))
    return __atomic_sub_fetch(ptr, val, __ATOMIC_ACQ_REL);
#elif defined(__GNUC__)
    return __sync_sub_and_fetch(ptr, val);
#else
    *ptr -= val;
    return *ptr;
#endif
}

/*===========================================================================*/
/* Atomic Bitwise Operations                                                  */
/*===========================================================================*/

/**
 * Atomic fetch-and-OR. Returns old value.
 */
static inline uint32_t rtos_atomic_fetch_or(volatile uint32_t *ptr, uint32_t val)
{
#if defined(__GNUC__) && (__GNUC__ >= 5 || defined(__clang__))
    return __atomic_fetch_or(ptr, val, __ATOMIC_ACQ_REL);
#elif defined(__GNUC__)
    return __sync_fetch_and_or(ptr, val);
#else
    uint32_t old = *ptr;
    *ptr |= val;
    return old;
#endif
}

/**
 * Atomic fetch-and-AND. Returns old value.
 */
static inline uint32_t rtos_atomic_fetch_and(volatile uint32_t *ptr, uint32_t val)
{
#if defined(__GNUC__) && (__GNUC__ >= 5 || defined(__clang__))
    return __atomic_fetch_and(ptr, val, __ATOMIC_ACQ_REL);
#elif defined(__GNUC__)
    return __sync_fetch_and_and(ptr, val);
#else
    uint32_t old = *ptr;
    *ptr &= val;
    return old;
#endif
}

/**
 * Atomic fetch-and-XOR. Returns old value.
 */
static inline uint32_t rtos_atomic_fetch_xor(volatile uint32_t *ptr, uint32_t val)
{
#if defined(__GNUC__) && (__GNUC__ >= 5 || defined(__clang__))
    return __atomic_fetch_xor(ptr, val, __ATOMIC_ACQ_REL);
#elif defined(__GNUC__)
    return __sync_fetch_and_xor(ptr, val);
#else
    uint32_t old = *ptr;
    *ptr ^= val;
    return old;
#endif
}

/*===========================================================================*/
/* Atomic Flag (Test-and-Set)                                                 */
/*===========================================================================*/

typedef struct {
    volatile uint32_t value;
} rtos_atomic_flag_t;

#define RTOS_ATOMIC_FLAG_INIT { 0 }

/**
 * Atomic test-and-set. Returns previous value.
 */
static inline bool rtos_atomic_flag_test_and_set(rtos_atomic_flag_t *flag)
{
    return (rtos_atomic_exchange(&flag->value, 1) != 0);
}

/**
 * Atomic clear.
 */
static inline void rtos_atomic_flag_clear(rtos_atomic_flag_t *flag)
{
    rtos_atomic_store(&flag->value, 0);
}

/**
 * Atomic test (non-modifying).
 */
static inline bool rtos_atomic_flag_test(rtos_atomic_flag_t *flag)
{
    return (rtos_atomic_load(&flag->value) != 0);
}

/*===========================================================================*/
/* Spinlock (using atomic flag)                                               */
/*===========================================================================*/

typedef rtos_atomic_flag_t rtos_spinlock_t;

#define RTOS_SPINLOCK_INIT RTOS_ATOMIC_FLAG_INIT

/**
 * Acquire spinlock (busy-wait).
 */
static inline void rtos_spinlock_acquire(rtos_spinlock_t *lock)
{
    while (rtos_atomic_flag_test_and_set(lock)) {
        /* Spin - could add yield hint here for hyperthreading CPUs */
#if defined(__arm__) || defined(__ARM_ARCH)
        __asm__ __volatile__("wfe");  /* Wait for event - saves power */
#endif
    }
    rtos_memory_barrier();
}

/**
 * Try to acquire spinlock without blocking.
 *
 * @return true if acquired, false if already locked
 */
static inline bool rtos_spinlock_try_acquire(rtos_spinlock_t *lock)
{
    if (!rtos_atomic_flag_test_and_set(lock)) {
        rtos_memory_barrier();
        return true;
    }
    return false;
}

/**
 * Release spinlock.
 */
static inline void rtos_spinlock_release(rtos_spinlock_t *lock)
{
    rtos_memory_barrier();
    rtos_atomic_flag_clear(lock);
#if defined(__arm__) || defined(__ARM_ARCH)
    __asm__ __volatile__("sev");  /* Send event - wake waiting cores */
#endif
}

/*===========================================================================*/
/* 8-bit and 16-bit Atomic Operations (for memory-constrained platforms)      */
/*===========================================================================*/

/**
 * Atomic load 8-bit.
 */
static inline uint8_t rtos_atomic_load8(volatile uint8_t *ptr)
{
#if defined(__GNUC__) && (__GNUC__ >= 5 || defined(__clang__))
    return __atomic_load_n(ptr, __ATOMIC_ACQUIRE);
#else
    return *ptr;
#endif
}

/**
 * Atomic store 8-bit.
 */
static inline void rtos_atomic_store8(volatile uint8_t *ptr, uint8_t val)
{
#if defined(__GNUC__) && (__GNUC__ >= 5 || defined(__clang__))
    __atomic_store_n(ptr, val, __ATOMIC_RELEASE);
#else
    *ptr = val;
#endif
}

/**
 * Atomic load 16-bit.
 */
static inline uint16_t rtos_atomic_load16(volatile uint16_t *ptr)
{
#if defined(__GNUC__) && (__GNUC__ >= 5 || defined(__clang__))
    return __atomic_load_n(ptr, __ATOMIC_ACQUIRE);
#else
    return *ptr;
#endif
}

/**
 * Atomic store 16-bit.
 */
static inline void rtos_atomic_store16(volatile uint16_t *ptr, uint16_t val)
{
#if defined(__GNUC__) && (__GNUC__ >= 5 || defined(__clang__))
    __atomic_store_n(ptr, val, __ATOMIC_RELEASE);
#else
    *ptr = val;
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* RTOS_ATOMIC_H */
