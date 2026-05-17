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

#ifndef MR_MISRA_H
#define MR_MISRA_H

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* MISRA C:2012 Version Identification                                        */
/*===========================================================================*/

#define MR_MISRA_VERSION_MAJOR    2012U
#define MR_MISRA_VERSION_MINOR    3U
#define MR_MISRA_COMPLIANCE_LEVEL 2U    /* 1=Advisory, 2=Required, 3=Mandatory */

/*===========================================================================*/
/* Compiler Detection and Feature Macros                                      */
/*===========================================================================*/

/* Compiler identification */
#if defined(__GNUC__) && !defined(__clang__)
    #define MR_COMPILER_GCC       1
    #define MR_GCC_VERSION        (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#elif defined(__clang__)
    #define MR_COMPILER_CLANG     1
    #define MR_CLANG_VERSION      (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(_MSC_VER)
    #define MR_COMPILER_MSVC      1
    #define MR_MSVC_VERSION       _MSC_VER
#elif defined(__ICCARM__)
    #define MR_COMPILER_IAR       1
#elif defined(__ARMCC_VERSION)
    #define MR_COMPILER_ARMCC     1
#else
    #define MR_COMPILER_UNKNOWN   1
#endif

/* C Standard version detection */
#if defined(__STDC_VERSION__)
    #if __STDC_VERSION__ >= 201112L
        #define MR_C11_AVAILABLE  1
    #elif __STDC_VERSION__ >= 199901L
        #define MR_C99_AVAILABLE  1
    #endif
#endif

/*===========================================================================*/
/* Static Analysis Annotations (MISRA Rule 1.2)                               */
/*===========================================================================*/

/**
 * Annotation for functions that never return (MISRA Rule 17.10)
 */
#if defined(MR_COMPILER_GCC) || defined(MR_COMPILER_CLANG)
    #define MR_NORETURN           __attribute__((noreturn))
#elif defined(MR_C11_AVAILABLE)
    #define MR_NORETURN           _Noreturn
#else
    #define MR_NORETURN           /* empty */
#endif

/**
 * Annotation for unused parameters (MISRA Rule 2.7)
 */
#if defined(MR_COMPILER_GCC) || defined(MR_COMPILER_CLANG)
    #define MR_UNUSED             __attribute__((unused))
#else
    #define MR_UNUSED             /* empty */
#endif

/**
 * Annotation for deprecated functions (MISRA Rule 1.1)
 */
#if defined(MR_COMPILER_GCC) || defined(MR_COMPILER_CLANG)
    #define MR_DEPRECATED(msg)    __attribute__((deprecated(msg)))
#elif defined(MR_COMPILER_MSVC)
    #define MR_DEPRECATED(msg)    __declspec(deprecated(msg))
#else
    #define MR_DEPRECATED(msg)    /* empty */
#endif

/**
 * Annotation for pure functions (no side effects, result depends only on args)
 */
#if defined(MR_COMPILER_GCC) || defined(MR_COMPILER_CLANG)
    #define MR_PURE               __attribute__((pure))
#else
    #define MR_PURE               /* empty */
#endif

/**
 * Annotation for const functions (stricter than pure, no memory reads)
 */
#if defined(MR_COMPILER_GCC) || defined(MR_COMPILER_CLANG)
    #define MR_CONST              __attribute__((const))
#else
    #define MR_CONST              /* empty */
#endif

/**
 * Force function inlining (MISRA Rule 8.10)
 */
#if defined(MR_COMPILER_GCC) || defined(MR_COMPILER_CLANG)
    #define MR_INLINE             static inline __attribute__((always_inline))
#elif defined(MR_COMPILER_MSVC)
    #define MR_INLINE             static __forceinline
#else
    #define MR_INLINE             static inline
#endif

/**
 * Prevent function inlining
 */
#if defined(MR_COMPILER_GCC) || defined(MR_COMPILER_CLANG)
    #define MR_NOINLINE           __attribute__((noinline))
#elif defined(MR_COMPILER_MSVC)
    #define MR_NOINLINE           __declspec(noinline)
#else
    #define MR_NOINLINE           /* empty */
#endif

/**
 * Weak symbol linkage for optional hooks
 */
#if defined(MR_COMPILER_GCC) || defined(MR_COMPILER_CLANG)
    #define MR_WEAK               __attribute__((weak))
#else
    #define MR_WEAK               /* empty */
#endif

/**
 * Naked function (no prologue/epilogue) for assembly routines
 */
#if defined(MR_COMPILER_GCC) || defined(MR_COMPILER_CLANG)
    #define MR_NAKED              __attribute__((naked))
#else
    #define MR_NAKED              /* empty */
#endif

/**
 * Section placement for linker control
 */
#if defined(MR_COMPILER_GCC) || defined(MR_COMPILER_CLANG)
    #define MR_SECTION(name)      __attribute__((section(name)))
#elif defined(MR_COMPILER_IAR)
    #define MR_SECTION(name)      @ name
#else
    #define MR_SECTION(name)      /* empty */
#endif

/**
 * Alignment specification (MISRA Rule 11.3)
 */
#if defined(MR_COMPILER_GCC) || defined(MR_COMPILER_CLANG)
    #define MR_ALIGNED(n)         __attribute__((aligned(n)))
#elif defined(MR_COMPILER_MSVC)
    #define MR_ALIGNED(n)         __declspec(align(n))
#else
    #define MR_ALIGNED(n)         /* empty */
#endif

/**
 * Pack structure without padding (MISRA Rule 6.1)
 */
#if defined(MR_COMPILER_GCC) || defined(MR_COMPILER_CLANG)
    #define MR_PACKED             __attribute__((packed))
#elif defined(MR_COMPILER_MSVC)
    #define MR_PACKED             /* Use #pragma pack instead */
#else
    #define MR_PACKED             /* empty */
#endif

/**
 * Mark function return value as requiring check (MISRA Rule 17.7)
 */
#if defined(MR_COMPILER_GCC) && (MR_GCC_VERSION >= 30400)
    #define MR_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#elif defined(MR_COMPILER_CLANG)
    #define MR_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
    #define MR_WARN_UNUSED_RESULT /* empty */
#endif

/**
 * Non-null pointer parameter annotation
 */
#if defined(MR_COMPILER_GCC) || defined(MR_COMPILER_CLANG)
    #define MR_NONNULL(...)       __attribute__((nonnull(__VA_ARGS__)))
    #define MR_RETURNS_NONNULL    __attribute__((returns_nonnull))
#else
    #define MR_NONNULL(...)       /* empty */
    #define MR_RETURNS_NONNULL    /* empty */
#endif

/*===========================================================================*/
/* Static Assertions (MISRA Rule 1.1, C11 Compliance)                         */
/*===========================================================================*/

/**
 * Compile-time assertion macro (MISRA Directive 4.6)
 */
#if defined(MR_C11_AVAILABLE)
    #define MR_STATIC_ASSERT(cond, msg)   _Static_assert(cond, msg)
#elif defined(MR_COMPILER_GCC) || defined(MR_COMPILER_CLANG)
    #define MR_STATIC_ASSERT(cond, msg) \
        typedef char mr_static_assert_##__LINE__[(cond) ? 1 : -1] MR_UNUSED
#else
    #define MR_STATIC_ASSERT(cond, msg) \
        typedef char mr_static_assert_##__LINE__[(cond) ? 1 : -1]
#endif

/*===========================================================================*/
/* Type-Safe Casting Macros (MISRA Rules 10.x, 11.x)                          */
/*===========================================================================*/

/**
 * Cast with explicit size verification (MISRA Rule 10.3)
 * These macros make narrowing conversions explicit and traceable
 */
#define MR_CAST_U8(x)     ((uint8_t)((x) & 0xFFU))
#define MR_CAST_U16(x)    ((uint16_t)((x) & 0xFFFFU))
#define MR_CAST_U32(x)    ((uint32_t)(x))
#define MR_CAST_S8(x)     ((int8_t)(x))
#define MR_CAST_S16(x)    ((int16_t)(x))
#define MR_CAST_S32(x)    ((int32_t)(x))

/**
 * Safe pointer casting macros (MISRA Rules 11.1-11.8)
 */
#define MR_PTR_TO_UINT(ptr)   ((uintptr_t)(const void *)(ptr))
#define MR_UINT_TO_PTR(val)   ((void *)(uintptr_t)(val))

/**
 * Volatile cast for memory-mapped registers (MISRA Rule 11.3)
 */
#define MR_VOLATILE_U32(addr) (*(volatile uint32_t *)(uintptr_t)(addr))
#define MR_VOLATILE_U16(addr) (*(volatile uint16_t *)(uintptr_t)(addr))
#define MR_VOLATILE_U8(addr)  (*(volatile uint8_t *)(uintptr_t)(addr))

/*===========================================================================*/
/* Boolean Type Safety (MISRA Rules 10.1, 14.4)                               */
/*===========================================================================*/

/**
 * Explicit boolean conversion (MISRA Rule 14.4)
 */
#define MR_BOOL(expr)         ((expr) != 0)
#define MR_IS_NULL(ptr)       ((ptr) == NULL)
#define MR_IS_NOT_NULL(ptr)   ((ptr) != NULL)

/*===========================================================================*/
/* Arithmetic Safety Macros (MISRA Rules 12.x)                                */
/*===========================================================================*/

/**
 * Overflow-safe addition for unsigned types
 */
#define MR_ADD_SAFE_U32(a, b, result) \
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
#define MR_SUB_SAFE_U32(a, b, result) \
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
#define MR_DIV_SAFE(num, denom, default_val) \
    (((denom) != 0U) ? ((num) / (denom)) : (default_val))

/**
 * Range checking (MISRA Rule 14.3)
 */
#define MR_IN_RANGE(val, min, max)    (((val) >= (min)) && ((val) <= (max)))
#define MR_CLAMP(val, min, max)       (((val) < (min)) ? (min) : (((val) > (max)) ? (max) : (val)))

/*===========================================================================*/
/* Bit Manipulation Macros (MISRA Rules 10.1, 12.2)                           */
/*===========================================================================*/

/**
 * Type-safe bit manipulation (MISRA Rule 12.2)
 * Using unsigned literals to prevent sign extension issues
 */
#define MR_BIT(n)             (1UL << (n))
#define MR_BIT_SET(val, bit)  ((val) |= MR_BIT(bit))
#define MR_BIT_CLR(val, bit)  ((val) &= ~MR_BIT(bit))
#define MR_BIT_TGL(val, bit)  ((val) ^= MR_BIT(bit))
#define MR_BIT_TST(val, bit)  (((val) & MR_BIT(bit)) != 0U)

/**
 * Bit mask generation (MISRA Rule 12.2)
 */
#define MR_BITMASK(width)     ((1UL << (width)) - 1UL)
#define MR_BITMASK_RANGE(hi, lo) \
    (((1UL << ((hi) - (lo) + 1U)) - 1UL) << (lo))

/**
 * Extract bit field (MISRA Rule 10.6)
 */
#define MR_GET_BITS(val, mask, shift) \
    (((val) & (mask)) >> (shift))

#define MR_SET_BITS(val, mask, shift, bits) \
    (((val) & ~(mask)) | (((bits) << (shift)) & (mask)))

/*===========================================================================*/
/* Memory Barrier Macros (MISRA Directive 4.3)                                */
/*===========================================================================*/

/**
 * Memory barriers for multi-core and hardware synchronization
 */
#if defined(MR_COMPILER_GCC) || defined(MR_COMPILER_CLANG)
    #define MR_MEMORY_BARRIER()       __asm__ __volatile__("" ::: "memory")
    #if defined(__arm__) || defined(__ARM_ARCH)
        #define MR_DMB()              __asm__ __volatile__("dmb" ::: "memory")
        #define MR_DSB()              __asm__ __volatile__("dsb" ::: "memory")
        #define MR_ISB()              __asm__ __volatile__("isb" ::: "memory")
    #elif defined(__AVR__)
        #define MR_DMB()              MR_MEMORY_BARRIER()
        #define MR_DSB()              MR_MEMORY_BARRIER()
        #define MR_ISB()              MR_MEMORY_BARRIER()
    #else
        #define MR_DMB()              MR_MEMORY_BARRIER()
        #define MR_DSB()              MR_MEMORY_BARRIER()
        #define MR_ISB()              MR_MEMORY_BARRIER()
    #endif
#else
    #define MR_MEMORY_BARRIER()       /* empty */
    #define MR_DMB()                  /* empty */
    #define MR_DSB()                  /* empty */
    #define MR_ISB()                  /* empty */
#endif

/*===========================================================================*/
/* Array Safety Macros (MISRA Rule 18.1)                                      */
/*===========================================================================*/

/**
 * Safe array size calculation (MISRA Rule 18.1)
 * Prevents sizeof applied to pointer
 */
#define MR_ARRAY_SIZE(arr) \
    (sizeof(arr) / sizeof((arr)[0]) + \
     sizeof(char[1 - 2 * !!(sizeof(arr) == sizeof(void *))]) * 0)

/**
 * Array bounds checking (MISRA Rule 18.1)
 */
#define MR_ARRAY_INDEX_VALID(arr, idx) \
    ((size_t)(idx) < MR_ARRAY_SIZE(arr))

/*===========================================================================*/
/* String Safety Macros (MISRA Rule 21.17)                                    */
/*===========================================================================*/

/**
 * Safe string copy with explicit bounds
 */
#define MR_STRNCPY_SAFE(dest, src, size) \
    do { \
        (void)strncpy((dest), (src), (size) - 1U); \
        (dest)[(size) - 1U] = '\0'; \
    } while (0)

/*===========================================================================*/
/* Defensive Programming Macros (IEC 61508, ISO 26262)                        */
/*===========================================================================*/

/**
 * Runtime assertion with configurable behavior (MISRA Directive 4.1)
 * Use MR_ASSERT for debug checks, MR_REQUIRE for production checks
 */
#ifndef MR_ASSERT
    #if MR_USE_ASSERT
        extern void mr_assert_failed(const char *file, int line);
        #define MR_ASSERT(expr) \
            do { if (!(expr)) { mr_assert_failed(__FILE__, __LINE__); } } while(0)
    #else
        #define MR_ASSERT(expr)   ((void)0)
    #endif
#endif

/**
 * Production runtime check (always enabled) (IEC 61508 SIL requirement)
 */
#define MR_REQUIRE(expr, action) \
    do { if (!(expr)) { action; } } while(0)

/**
 * Precondition check (function entry validation)
 */
#define MR_PRECONDITION(expr)     MR_ASSERT(expr)

/**
 * Postcondition check (function exit validation)
 */
#define MR_POSTCONDITION(expr)    MR_ASSERT(expr)

/**
 * Invariant check (state consistency validation)
 */
#define MR_INVARIANT(expr)        MR_ASSERT(expr)

/**
 * Unreachable code marker (MISRA Rule 2.1)
 */
#if defined(MR_COMPILER_GCC) || defined(MR_COMPILER_CLANG)
    #define MR_UNREACHABLE() \
        do { MR_ASSERT(0); __builtin_unreachable(); } while(0)
#else
    #define MR_UNREACHABLE() \
        do { MR_ASSERT(0); for(;;) {} } while(0)
#endif

/**
 * Default case handler for switch statements (MISRA Rule 16.4)
 */
#define MR_DEFAULT_UNREACHABLE()  default: MR_UNREACHABLE(); break

/*===========================================================================*/
/* Function Contract Annotations (ISO 26262 ASIL-D)                           */
/*===========================================================================*/

/**
 * Function pre/post condition documentation macros
 * These are informational for static analysis tools
 */
#define MR_PRE(cond)      /* Precondition: cond */
#define MR_POST(cond)     /* Postcondition: cond */
#define MR_INV(cond)      /* Invariant: cond */

/*===========================================================================*/
/* Suppress MISRA Warnings (with documentation)                               */
/*===========================================================================*/

/**
 * MISRA deviation documentation macro
 * Use when a deviation from MISRA is necessary and justified
 */
#define MR_MISRA_DEVIATION_START(rule, reason) \
    /* MISRA Deviation: Rule rule - Reason: reason */

#define MR_MISRA_DEVIATION_END(rule) \
    /* End MISRA Deviation: Rule rule */

/**
 * Suppress specific compiler/analyzer warnings
 */
#if defined(MR_COMPILER_GCC) || defined(MR_COMPILER_CLANG)
    #define MR_PRAGMA(x)              _Pragma(#x)
    #define MR_DIAG_PUSH()            MR_PRAGMA(GCC diagnostic push)
    #define MR_DIAG_POP()             MR_PRAGMA(GCC diagnostic pop)
    #define MR_DIAG_IGNORE(warn)      MR_PRAGMA(GCC diagnostic ignored warn)
#else
    #define MR_PRAGMA(x)              /* empty */
    #define MR_DIAG_PUSH()            /* empty */
    #define MR_DIAG_POP()             /* empty */
    #define MR_DIAG_IGNORE(warn)      /* empty */
#endif

/*===========================================================================*/
/* Integer Type Verification (MISRA Directive 4.6)                            */
/*===========================================================================*/

/* Verify fixed-width integer types are available and correctly sized */
MR_STATIC_ASSERT(sizeof(uint8_t) == 1, "uint8_t must be 1 byte");
MR_STATIC_ASSERT(sizeof(uint16_t) == 2, "uint16_t must be 2 bytes");
MR_STATIC_ASSERT(sizeof(uint32_t) == 4, "uint32_t must be 4 bytes");
MR_STATIC_ASSERT(sizeof(int8_t) == 1, "int8_t must be 1 byte");
MR_STATIC_ASSERT(sizeof(int16_t) == 2, "int16_t must be 2 bytes");
MR_STATIC_ASSERT(sizeof(int32_t) == 4, "int32_t must be 4 bytes");

/* Verify pointer size for platform detection */
#if defined(__arm__) || defined(__ARM_ARCH)
    MR_STATIC_ASSERT(sizeof(void *) == 4, "ARM pointer must be 4 bytes");
#endif

/*===========================================================================*/
/* Critical Section Validation (IEC 61508)                                    */
/*===========================================================================*/

/**
 * Critical section tracking for debugging (detects imbalanced entry/exit)
 */
#if MR_USE_ASSERT
    #define MR_CRITICAL_ENTER_CHECKED() \
        do { \
            mr_port_enter_critical(); \
            MR_ASSERT(g_critical_nesting < 255U); \
        } while(0)

    #define MR_CRITICAL_EXIT_CHECKED() \
        do { \
            MR_ASSERT(g_critical_nesting > 0U); \
            mr_port_exit_critical(); \
        } while(0)
#else
    #define MR_CRITICAL_ENTER_CHECKED()   mr_port_enter_critical()
    #define MR_CRITICAL_EXIT_CHECKED()    mr_port_exit_critical()
#endif

/*===========================================================================*/
/* Version String Generation                                                  */
/*===========================================================================*/

#define MR_STRINGIFY(x)           #x
#define MR_STRINGIFY_EXPAND(x)    MR_STRINGIFY(x)

#define MR_MAKE_VERSION(major, minor, patch) \
    (((major) * 10000U) + ((minor) * 100U) + (patch))

#ifdef __cplusplus
}
#endif

#endif /* MR_MISRA_H */
