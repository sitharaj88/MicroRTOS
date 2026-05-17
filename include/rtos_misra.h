/**
 * MicroRTOS - MISRA C:2012 Compliance Header
 *
 * This header provides MISRA C:2012 compliance infrastructure including:
 * - Compiler-agnostic attribute macros
 * - Static analysis annotations
 * - Type-safe casting macros
 * - Defensive programming utilities
 *
 * Compliance Standards:
 * - MISRA C:2012 (Motor Industry Software Reliability Association)
 * - IEC 61508 (Functional Safety)
 * - ISO 26262 (Automotive Safety)
 * - DO-178C (Aerospace Software)
 * - CERT C Coding Standard
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef RTOS_MISRA_H
#define RTOS_MISRA_H

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* MISRA C:2012 Version Identification                                        */
/*===========================================================================*/

#define RTOS_MISRA_VERSION_MAJOR    2012U
#define RTOS_MISRA_VERSION_MINOR    3U
#define RTOS_MISRA_COMPLIANCE_LEVEL 2U    /* 1=Advisory, 2=Required, 3=Mandatory */

/*===========================================================================*/
/* Compiler Detection and Feature Macros                                      */
/*===========================================================================*/

/* Compiler identification */
#if defined(__GNUC__) && !defined(__clang__)
    #define RTOS_COMPILER_GCC       1
    #define RTOS_GCC_VERSION        (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#elif defined(__clang__)
    #define RTOS_COMPILER_CLANG     1
    #define RTOS_CLANG_VERSION      (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(_MSC_VER)
    #define RTOS_COMPILER_MSVC      1
    #define RTOS_MSVC_VERSION       _MSC_VER
#elif defined(__ICCARM__)
    #define RTOS_COMPILER_IAR       1
#elif defined(__ARMCC_VERSION)
    #define RTOS_COMPILER_ARMCC     1
#else
    #define RTOS_COMPILER_UNKNOWN   1
#endif

/* C Standard version detection */
#if defined(__STDC_VERSION__)
    #if __STDC_VERSION__ >= 201112L
        #define RTOS_C11_AVAILABLE  1
    #elif __STDC_VERSION__ >= 199901L
        #define RTOS_C99_AVAILABLE  1
    #endif
#endif

/*===========================================================================*/
/* Static Analysis Annotations (MISRA Rule 1.2)                               */
/*===========================================================================*/

/**
 * Annotation for functions that never return (MISRA Rule 17.10)
 */
#if defined(RTOS_COMPILER_GCC) || defined(RTOS_COMPILER_CLANG)
    #define RTOS_NORETURN           __attribute__((noreturn))
#elif defined(RTOS_C11_AVAILABLE)
    #define RTOS_NORETURN           _Noreturn
#else
    #define RTOS_NORETURN           /* empty */
#endif

/**
 * Annotation for unused parameters (MISRA Rule 2.7)
 */
#if defined(RTOS_COMPILER_GCC) || defined(RTOS_COMPILER_CLANG)
    #define RTOS_UNUSED             __attribute__((unused))
#else
    #define RTOS_UNUSED             /* empty */
#endif

/**
 * Annotation for deprecated functions (MISRA Rule 1.1)
 */
#if defined(RTOS_COMPILER_GCC) || defined(RTOS_COMPILER_CLANG)
    #define RTOS_DEPRECATED(msg)    __attribute__((deprecated(msg)))
#elif defined(RTOS_COMPILER_MSVC)
    #define RTOS_DEPRECATED(msg)    __declspec(deprecated(msg))
#else
    #define RTOS_DEPRECATED(msg)    /* empty */
#endif

/**
 * Annotation for pure functions (no side effects, result depends only on args)
 */
#if defined(RTOS_COMPILER_GCC) || defined(RTOS_COMPILER_CLANG)
    #define RTOS_PURE               __attribute__((pure))
#else
    #define RTOS_PURE               /* empty */
#endif

/**
 * Annotation for const functions (stricter than pure, no memory reads)
 */
#if defined(RTOS_COMPILER_GCC) || defined(RTOS_COMPILER_CLANG)
    #define RTOS_CONST              __attribute__((const))
#else
    #define RTOS_CONST              /* empty */
#endif

/**
 * Force function inlining (MISRA Rule 8.10)
 */
#if defined(RTOS_COMPILER_GCC) || defined(RTOS_COMPILER_CLANG)
    #define RTOS_INLINE             static inline __attribute__((always_inline))
#elif defined(RTOS_COMPILER_MSVC)
    #define RTOS_INLINE             static __forceinline
#else
    #define RTOS_INLINE             static inline
#endif

/**
 * Prevent function inlining
 */
#if defined(RTOS_COMPILER_GCC) || defined(RTOS_COMPILER_CLANG)
    #define RTOS_NOINLINE           __attribute__((noinline))
#elif defined(RTOS_COMPILER_MSVC)
    #define RTOS_NOINLINE           __declspec(noinline)
#else
    #define RTOS_NOINLINE           /* empty */
#endif

/**
 * Weak symbol linkage for optional hooks
 */
#if defined(RTOS_COMPILER_GCC) || defined(RTOS_COMPILER_CLANG)
    #define RTOS_WEAK               __attribute__((weak))
#else
    #define RTOS_WEAK               /* empty */
#endif

/**
 * Naked function (no prologue/epilogue) for assembly routines
 */
#if defined(RTOS_COMPILER_GCC) || defined(RTOS_COMPILER_CLANG)
    #define RTOS_NAKED              __attribute__((naked))
#else
    #define RTOS_NAKED              /* empty */
#endif

/**
 * Section placement for linker control
 */
#if defined(RTOS_COMPILER_GCC) || defined(RTOS_COMPILER_CLANG)
    #define RTOS_SECTION(name)      __attribute__((section(name)))
#elif defined(RTOS_COMPILER_IAR)
    #define RTOS_SECTION(name)      @ name
#else
    #define RTOS_SECTION(name)      /* empty */
#endif

/**
 * Alignment specification (MISRA Rule 11.3)
 */
#if defined(RTOS_COMPILER_GCC) || defined(RTOS_COMPILER_CLANG)
    #define RTOS_ALIGNED(n)         __attribute__((aligned(n)))
#elif defined(RTOS_COMPILER_MSVC)
    #define RTOS_ALIGNED(n)         __declspec(align(n))
#else
    #define RTOS_ALIGNED(n)         /* empty */
#endif

/**
 * Pack structure without padding (MISRA Rule 6.1)
 */
#if defined(RTOS_COMPILER_GCC) || defined(RTOS_COMPILER_CLANG)
    #define RTOS_PACKED             __attribute__((packed))
#elif defined(RTOS_COMPILER_MSVC)
    #define RTOS_PACKED             /* Use #pragma pack instead */
#else
    #define RTOS_PACKED             /* empty */
#endif

/**
 * Mark function return value as requiring check (MISRA Rule 17.7)
 */
#if defined(RTOS_COMPILER_GCC) && (RTOS_GCC_VERSION >= 30400)
    #define RTOS_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#elif defined(RTOS_COMPILER_CLANG)
    #define RTOS_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
    #define RTOS_WARN_UNUSED_RESULT /* empty */
#endif

/**
 * Non-null pointer parameter annotation
 */
#if defined(RTOS_COMPILER_GCC) || defined(RTOS_COMPILER_CLANG)
    #define RTOS_NONNULL(...)       __attribute__((nonnull(__VA_ARGS__)))
    #define RTOS_RETURNS_NONNULL    __attribute__((returns_nonnull))
#else
    #define RTOS_NONNULL(...)       /* empty */
    #define RTOS_RETURNS_NONNULL    /* empty */
#endif

/*===========================================================================*/
/* Static Assertions (MISRA Rule 1.1, C11 Compliance)                         */
/*===========================================================================*/

/**
 * Compile-time assertion macro (MISRA Directive 4.6)
 */
#if defined(RTOS_C11_AVAILABLE)
    #define RTOS_STATIC_ASSERT(cond, msg)   _Static_assert(cond, msg)
#elif defined(RTOS_COMPILER_GCC) || defined(RTOS_COMPILER_CLANG)
    #define RTOS_STATIC_ASSERT(cond, msg) \
        typedef char rtos_static_assert_##__LINE__[(cond) ? 1 : -1] RTOS_UNUSED
#else
    #define RTOS_STATIC_ASSERT(cond, msg) \
        typedef char rtos_static_assert_##__LINE__[(cond) ? 1 : -1]
#endif

/*===========================================================================*/
/* Type-Safe Casting Macros (MISRA Rules 10.x, 11.x)                          */
/*===========================================================================*/

/**
 * Cast with explicit size verification (MISRA Rule 10.3)
 * These macros make narrowing conversions explicit and traceable
 */
#define RTOS_CAST_U8(x)     ((uint8_t)((x) & 0xFFU))
#define RTOS_CAST_U16(x)    ((uint16_t)((x) & 0xFFFFU))
#define RTOS_CAST_U32(x)    ((uint32_t)(x))
#define RTOS_CAST_S8(x)     ((int8_t)(x))
#define RTOS_CAST_S16(x)    ((int16_t)(x))
#define RTOS_CAST_S32(x)    ((int32_t)(x))

/**
 * Safe pointer casting macros (MISRA Rules 11.1-11.8)
 */
#define RTOS_PTR_TO_UINT(ptr)   ((uintptr_t)(const void *)(ptr))
#define RTOS_UINT_TO_PTR(val)   ((void *)(uintptr_t)(val))

/**
 * Volatile cast for memory-mapped registers (MISRA Rule 11.3)
 */
#define RTOS_VOLATILE_U32(addr) (*(volatile uint32_t *)(uintptr_t)(addr))
#define RTOS_VOLATILE_U16(addr) (*(volatile uint16_t *)(uintptr_t)(addr))
#define RTOS_VOLATILE_U8(addr)  (*(volatile uint8_t *)(uintptr_t)(addr))

/*===========================================================================*/
/* Boolean Type Safety (MISRA Rules 10.1, 14.4)                               */
/*===========================================================================*/

/**
 * Explicit boolean conversion (MISRA Rule 14.4)
 */
#define RTOS_BOOL(expr)         ((expr) != 0)
#define RTOS_IS_NULL(ptr)       ((ptr) == NULL)
#define RTOS_IS_NOT_NULL(ptr)   ((ptr) != NULL)

/*===========================================================================*/
/* Arithmetic Safety Macros (MISRA Rules 12.x)                                */
/*===========================================================================*/

/**
 * Overflow-safe addition for unsigned types
 */
#define RTOS_ADD_SAFE_U32(a, b, result) \
    do { \
        if ((a) > (UINT32_MAX - (b))) { \
            *(result) = UINT32_MAX; \
        } else { \
            *(result) = (a) + (b); \
        } \
    } while (0)

/**
 * Overflow-safe subtraction for unsigned types
 */
#define RTOS_SUB_SAFE_U32(a, b, result) \
    do { \
        if ((a) < (b)) { \
            *(result) = 0U; \
        } else { \
            *(result) = (a) - (b); \
        } \
    } while (0)

/**
 * Safe division (avoid divide by zero)
 */
#define RTOS_DIV_SAFE(num, denom, default_val) \
    (((denom) != 0U) ? ((num) / (denom)) : (default_val))

/**
 * Range checking (MISRA Rule 14.3)
 */
#define RTOS_IN_RANGE(val, min, max)    (((val) >= (min)) && ((val) <= (max)))
#define RTOS_CLAMP(val, min, max)       (((val) < (min)) ? (min) : (((val) > (max)) ? (max) : (val)))

/*===========================================================================*/
/* Bit Manipulation Macros (MISRA Rules 10.1, 12.2)                           */
/*===========================================================================*/

/**
 * Type-safe bit manipulation (MISRA Rule 12.2)
 * Using unsigned literals to prevent sign extension issues
 */
#define RTOS_BIT(n)             (1UL << (n))
#define RTOS_BIT_SET(val, bit)  ((val) |= RTOS_BIT(bit))
#define RTOS_BIT_CLR(val, bit)  ((val) &= ~RTOS_BIT(bit))
#define RTOS_BIT_TGL(val, bit)  ((val) ^= RTOS_BIT(bit))
#define RTOS_BIT_TST(val, bit)  (((val) & RTOS_BIT(bit)) != 0U)

/**
 * Bit mask generation (MISRA Rule 12.2)
 */
#define RTOS_BITMASK(width)     ((1UL << (width)) - 1UL)
#define RTOS_BITMASK_RANGE(hi, lo) \
    (((1UL << ((hi) - (lo) + 1U)) - 1UL) << (lo))

/**
 * Extract bit field (MISRA Rule 10.6)
 */
#define RTOS_GET_BITS(val, mask, shift) \
    (((val) & (mask)) >> (shift))

#define RTOS_SET_BITS(val, mask, shift, bits) \
    (((val) & ~(mask)) | (((bits) << (shift)) & (mask)))

/*===========================================================================*/
/* Memory Barrier Macros (MISRA Directive 4.3)                                */
/*===========================================================================*/

/**
 * Memory barriers for multi-core and hardware synchronization
 */
#if defined(RTOS_COMPILER_GCC) || defined(RTOS_COMPILER_CLANG)
    #define RTOS_MEMORY_BARRIER()       __asm__ __volatile__("" ::: "memory")
    #if defined(__arm__) || defined(__ARM_ARCH)
        #define RTOS_DMB()              __asm__ __volatile__("dmb" ::: "memory")
        #define RTOS_DSB()              __asm__ __volatile__("dsb" ::: "memory")
        #define RTOS_ISB()              __asm__ __volatile__("isb" ::: "memory")
    #elif defined(__AVR__)
        #define RTOS_DMB()              RTOS_MEMORY_BARRIER()
        #define RTOS_DSB()              RTOS_MEMORY_BARRIER()
        #define RTOS_ISB()              RTOS_MEMORY_BARRIER()
    #else
        #define RTOS_DMB()              RTOS_MEMORY_BARRIER()
        #define RTOS_DSB()              RTOS_MEMORY_BARRIER()
        #define RTOS_ISB()              RTOS_MEMORY_BARRIER()
    #endif
#else
    #define RTOS_MEMORY_BARRIER()       /* empty */
    #define RTOS_DMB()                  /* empty */
    #define RTOS_DSB()                  /* empty */
    #define RTOS_ISB()                  /* empty */
#endif

/*===========================================================================*/
/* Array Safety Macros (MISRA Rule 18.1)                                      */
/*===========================================================================*/

/**
 * Safe array size calculation (MISRA Rule 18.1)
 * Prevents sizeof applied to pointer
 */
#define RTOS_ARRAY_SIZE(arr) \
    (sizeof(arr) / sizeof((arr)[0]) + \
     sizeof(char[1 - 2 * !!(sizeof(arr) == sizeof(void *))]) * 0)

/**
 * Array bounds checking (MISRA Rule 18.1)
 */
#define RTOS_ARRAY_INDEX_VALID(arr, idx) \
    ((size_t)(idx) < RTOS_ARRAY_SIZE(arr))

/*===========================================================================*/
/* String Safety Macros (MISRA Rule 21.17)                                    */
/*===========================================================================*/

/**
 * Safe string copy with explicit bounds
 */
#define RTOS_STRNCPY_SAFE(dest, src, size) \
    do { \
        (void)strncpy((dest), (src), (size) - 1U); \
        (dest)[(size) - 1U] = '\0'; \
    } while (0)

/*===========================================================================*/
/* Defensive Programming Macros (IEC 61508, ISO 26262)                        */
/*===========================================================================*/

/**
 * Runtime assertion with configurable behavior (MISRA Directive 4.1)
 * Use RTOS_ASSERT for debug checks, RTOS_REQUIRE for production checks
 */
#ifndef RTOS_ASSERT
    #if RTOS_USE_ASSERT
        extern void rtos_assert_failed(const char *file, int line);
        #define RTOS_ASSERT(expr) \
            do { if (!(expr)) { rtos_assert_failed(__FILE__, __LINE__); } } while(0)
    #else
        #define RTOS_ASSERT(expr)   ((void)0)
    #endif
#endif

/**
 * Production runtime check (always enabled) (IEC 61508 SIL requirement)
 */
#define RTOS_REQUIRE(expr, action) \
    do { if (!(expr)) { action; } } while(0)

/**
 * Precondition check (function entry validation)
 */
#define RTOS_PRECONDITION(expr)     RTOS_ASSERT(expr)

/**
 * Postcondition check (function exit validation)
 */
#define RTOS_POSTCONDITION(expr)    RTOS_ASSERT(expr)

/**
 * Invariant check (state consistency validation)
 */
#define RTOS_INVARIANT(expr)        RTOS_ASSERT(expr)

/**
 * Unreachable code marker (MISRA Rule 2.1)
 */
#if defined(RTOS_COMPILER_GCC) || defined(RTOS_COMPILER_CLANG)
    #define RTOS_UNREACHABLE() \
        do { RTOS_ASSERT(0); __builtin_unreachable(); } while(0)
#else
    #define RTOS_UNREACHABLE() \
        do { RTOS_ASSERT(0); for(;;) {} } while(0)
#endif

/**
 * Default case handler for switch statements (MISRA Rule 16.4)
 */
#define RTOS_DEFAULT_UNREACHABLE()  default: RTOS_UNREACHABLE(); break

/*===========================================================================*/
/* Function Contract Annotations (ISO 26262 ASIL-D)                           */
/*===========================================================================*/

/**
 * Function pre/post condition documentation macros
 * These are informational for static analysis tools
 */
#define RTOS_PRE(cond)      /* Precondition: cond */
#define RTOS_POST(cond)     /* Postcondition: cond */
#define RTOS_INV(cond)      /* Invariant: cond */

/*===========================================================================*/
/* Suppress MISRA Warnings (with documentation)                               */
/*===========================================================================*/

/**
 * MISRA deviation documentation macro
 * Use when a deviation from MISRA is necessary and justified
 */
#define RTOS_MISRA_DEVIATION_START(rule, reason) \
    /* MISRA Deviation: Rule rule - Reason: reason */

#define RTOS_MISRA_DEVIATION_END(rule) \
    /* End MISRA Deviation: Rule rule */

/**
 * Suppress specific compiler/analyzer warnings
 */
#if defined(RTOS_COMPILER_GCC) || defined(RTOS_COMPILER_CLANG)
    #define RTOS_PRAGMA(x)              _Pragma(#x)
    #define RTOS_DIAG_PUSH()            RTOS_PRAGMA(GCC diagnostic push)
    #define RTOS_DIAG_POP()             RTOS_PRAGMA(GCC diagnostic pop)
    #define RTOS_DIAG_IGNORE(warn)      RTOS_PRAGMA(GCC diagnostic ignored warn)
#else
    #define RTOS_PRAGMA(x)              /* empty */
    #define RTOS_DIAG_PUSH()            /* empty */
    #define RTOS_DIAG_POP()             /* empty */
    #define RTOS_DIAG_IGNORE(warn)      /* empty */
#endif

/*===========================================================================*/
/* Integer Type Verification (MISRA Directive 4.6)                            */
/*===========================================================================*/

/* Verify fixed-width integer types are available and correctly sized */
RTOS_STATIC_ASSERT(sizeof(uint8_t) == 1, "uint8_t must be 1 byte");
RTOS_STATIC_ASSERT(sizeof(uint16_t) == 2, "uint16_t must be 2 bytes");
RTOS_STATIC_ASSERT(sizeof(uint32_t) == 4, "uint32_t must be 4 bytes");
RTOS_STATIC_ASSERT(sizeof(int8_t) == 1, "int8_t must be 1 byte");
RTOS_STATIC_ASSERT(sizeof(int16_t) == 2, "int16_t must be 2 bytes");
RTOS_STATIC_ASSERT(sizeof(int32_t) == 4, "int32_t must be 4 bytes");

/* Verify pointer size for platform detection */
#if defined(__arm__) || defined(__ARM_ARCH)
    RTOS_STATIC_ASSERT(sizeof(void *) == 4, "ARM pointer must be 4 bytes");
#endif

/*===========================================================================*/
/* Critical Section Validation (IEC 61508)                                    */
/*===========================================================================*/

/**
 * Critical section tracking for debugging (detects imbalanced entry/exit)
 */
#if RTOS_USE_ASSERT
    #define RTOS_CRITICAL_ENTER_CHECKED() \
        do { \
            rtos_port_enter_critical(); \
            RTOS_ASSERT(g_critical_nesting < 255U); \
        } while(0)

    #define RTOS_CRITICAL_EXIT_CHECKED() \
        do { \
            RTOS_ASSERT(g_critical_nesting > 0U); \
            rtos_port_exit_critical(); \
        } while(0)
#else
    #define RTOS_CRITICAL_ENTER_CHECKED()   rtos_port_enter_critical()
    #define RTOS_CRITICAL_EXIT_CHECKED()    rtos_port_exit_critical()
#endif

/*===========================================================================*/
/* Version String Generation                                                  */
/*===========================================================================*/

#define RTOS_STRINGIFY(x)           #x
#define RTOS_STRINGIFY_EXPAND(x)    RTOS_STRINGIFY(x)

#define RTOS_MAKE_VERSION(major, minor, patch) \
    (((major) * 10000U) + ((minor) * 100U) + (patch))

#ifdef __cplusplus
}
#endif

#endif /* RTOS_MISRA_H */
