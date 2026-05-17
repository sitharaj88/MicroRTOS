/**
 * MicroRTOS - Safety-Critical Runtime Verification Header
 *
 * This header provides runtime verification mechanisms for safety-critical
 * systems following IEC 61508, ISO 26262, and DO-178C standards.
 *
 * Features:
 * - Runtime fault detection and handling
 * - Memory integrity checking
 * - Control flow monitoring
 * - Watchdog integration
 * - Diagnostic coverage metrics
 *
 * Safety Integrity Levels (SIL) Support:
 * - IEC 61508: SIL 1-4
 * - ISO 26262: ASIL A-D
 * - DO-178C: DAL A-E
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef RTOS_SAFETY_H
#define RTOS_SAFETY_H

#include <stdint.h>
#include <stdbool.h>
#include "rtos_config.h"
#include "rtos_misra.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Safety Configuration                                                       */
/*===========================================================================*/

/**
 * Enable safety mechanisms (set to 1 for safety-critical applications)
 */
#ifndef RTOS_SAFETY_ENABLE
    #define RTOS_SAFETY_ENABLE          1
#endif

/**
 * Safety integrity level (1-4 per IEC 61508)
 */
#ifndef RTOS_SAFETY_SIL
    #define RTOS_SAFETY_SIL             2
#endif

/**
 * Enable control flow monitoring
 */
#ifndef RTOS_SAFETY_CFM_ENABLE
    #define RTOS_SAFETY_CFM_ENABLE      (RTOS_SAFETY_SIL >= 2)
#endif

/**
 * Enable memory integrity checks
 */
#ifndef RTOS_SAFETY_MEM_ENABLE
    #define RTOS_SAFETY_MEM_ENABLE      (RTOS_SAFETY_SIL >= 2)
#endif

/**
 * Enable redundant data storage
 */
#ifndef RTOS_SAFETY_REDUNDANT_ENABLE
    #define RTOS_SAFETY_REDUNDANT_ENABLE (RTOS_SAFETY_SIL >= 3)
#endif

/*===========================================================================*/
/* Fault Classification (IEC 61508-2)                                         */
/*===========================================================================*/

/**
 * Fault severity levels
 */
typedef enum rtos_fault_severity {
    RTOS_FAULT_MINOR        = 0,    /**< Minor fault, continue operation */
    RTOS_FAULT_MODERATE     = 1,    /**< Moderate fault, degraded operation */
    RTOS_FAULT_SEVERE       = 2,    /**< Severe fault, safe state required */
    RTOS_FAULT_CRITICAL     = 3,    /**< Critical fault, immediate shutdown */
} rtos_fault_severity_t;

/**
 * Fault categories
 */
typedef enum rtos_fault_category {
    RTOS_FAULT_CAT_NONE         = 0,    /**< No fault */
    RTOS_FAULT_CAT_STACK        = 1,    /**< Stack overflow/corruption */
    RTOS_FAULT_CAT_MEMORY       = 2,    /**< Memory corruption */
    RTOS_FAULT_CAT_TIMING       = 3,    /**< Timing violation */
    RTOS_FAULT_CAT_WATCHDOG     = 4,    /**< Watchdog timeout */
    RTOS_FAULT_CAT_ASSERTION    = 5,    /**< Assertion failure */
    RTOS_FAULT_CAT_CONTROL_FLOW = 6,    /**< Control flow error */
    RTOS_FAULT_CAT_PARAM        = 7,    /**< Parameter validation failure */
    RTOS_FAULT_CAT_STATE        = 8,    /**< Invalid state transition */
    RTOS_FAULT_CAT_RESOURCE     = 9,    /**< Resource exhaustion */
    RTOS_FAULT_CAT_HARDWARE     = 10,   /**< Hardware fault */
    RTOS_FAULT_CAT_USER         = 11,   /**< User-defined fault */
} rtos_fault_category_t;

/**
 * Fault information structure
 */
typedef struct rtos_fault_info {
    rtos_fault_category_t category;     /**< Fault category */
    rtos_fault_severity_t severity;     /**< Fault severity */
    uint32_t              fault_code;   /**< Specific fault code */
    uint32_t              fault_data;   /**< Additional fault data */
    const char           *file;         /**< Source file (if available) */
    uint32_t              line;         /**< Source line (if available) */
    uint32_t              timestamp;    /**< Fault timestamp (tick count) */
} rtos_fault_info_t;

/*===========================================================================*/
/* Fault Handler Interface                                                    */
/*===========================================================================*/

/**
 * Fault handler callback type
 */
typedef void (*rtos_fault_handler_t)(const rtos_fault_info_t *info);

/**
 * Register a custom fault handler
 *
 * @param handler   Fault handler callback
 */
void rtos_safety_set_fault_handler(rtos_fault_handler_t handler);

/**
 * Report a fault to the safety subsystem
 *
 * @param category  Fault category
 * @param severity  Fault severity
 * @param code      Specific fault code
 * @param data      Additional data
 */
void rtos_safety_report_fault(
    rtos_fault_category_t category,
    rtos_fault_severity_t severity,
    uint32_t code,
    uint32_t data);

/**
 * Report fault with source location
 */
#define RTOS_SAFETY_FAULT(cat, sev, code, data) \
    rtos_safety_report_fault_loc((cat), (sev), (code), (data), __FILE__, __LINE__)

void rtos_safety_report_fault_loc(
    rtos_fault_category_t category,
    rtos_fault_severity_t severity,
    uint32_t code,
    uint32_t data,
    const char *file,
    uint32_t line);

/*===========================================================================*/
/* Control Flow Monitoring (CFM)                                              */
/*===========================================================================*/

#if RTOS_SAFETY_CFM_ENABLE

/**
 * Control flow signature for function verification
 * Each critical function has a unique signature
 */
typedef uint32_t rtos_cfm_signature_t;

/**
 * Initialize CFM context for a function
 */
#define RTOS_CFM_INIT(sig)          rtos_cfm_init(sig)

/**
 * Update CFM checkpoint within a function
 */
#define RTOS_CFM_CHECKPOINT(id)     rtos_cfm_checkpoint(id)

/**
 * Verify CFM at function exit
 */
#define RTOS_CFM_VERIFY(expected)   rtos_cfm_verify(expected)

/**
 * CFM function prototypes
 */
void rtos_cfm_init(rtos_cfm_signature_t initial_sig);
void rtos_cfm_checkpoint(uint32_t checkpoint_id);
bool rtos_cfm_verify(rtos_cfm_signature_t expected_sig);

/**
 * Generate CFM signature from checkpoint sequence
 */
#define RTOS_CFM_SIG(base, cp1, cp2, cp3) \
    (((base) ^ ((cp1) << 8)) ^ ((cp2) << 16) ^ ((cp3) << 24))

#else /* !RTOS_SAFETY_CFM_ENABLE */

#define RTOS_CFM_INIT(sig)          ((void)0)
#define RTOS_CFM_CHECKPOINT(id)     ((void)0)
#define RTOS_CFM_VERIFY(expected)   (true)

#endif /* RTOS_SAFETY_CFM_ENABLE */

/*===========================================================================*/
/* Memory Integrity Checking                                                  */
/*===========================================================================*/

#if RTOS_SAFETY_MEM_ENABLE

/**
 * Memory region descriptor for integrity checking
 */
typedef struct rtos_mem_region {
    void       *base;           /**< Region base address */
    uint32_t    size;           /**< Region size in bytes */
    uint32_t    checksum;       /**< Calculated checksum */
    bool        read_only;      /**< Read-only region flag */
} rtos_mem_region_t;

/**
 * Calculate CRC32 checksum of memory region
 */
uint32_t rtos_safety_crc32(const void *data, uint32_t size);

/**
 * Initialize memory region for integrity monitoring
 */
void rtos_safety_mem_init(rtos_mem_region_t *region, void *base, uint32_t size, bool read_only);

/**
 * Update checksum for a memory region
 */
void rtos_safety_mem_update(rtos_mem_region_t *region);

/**
 * Verify memory region integrity
 */
bool rtos_safety_mem_verify(const rtos_mem_region_t *region);

/**
 * Guard value for memory corruption detection
 */
#define RTOS_GUARD_VALUE        0xDEADBEEFUL
#define RTOS_GUARD_VALUE_INV    0x21524110UL  /* ~RTOS_GUARD_VALUE */

/**
 * Memory guard structure (place before/after critical data)
 */
typedef struct rtos_mem_guard {
    uint32_t guard_start;       /**< Start guard (RTOS_GUARD_VALUE) */
    uint32_t guard_start_inv;   /**< Inverted start guard */
} rtos_mem_guard_t;

/**
 * Initialize memory guard
 */
RTOS_INLINE void rtos_mem_guard_init(rtos_mem_guard_t *guard)
{
    guard->guard_start = RTOS_GUARD_VALUE;
    guard->guard_start_inv = RTOS_GUARD_VALUE_INV;
}

/**
 * Verify memory guard
 */
RTOS_INLINE bool rtos_mem_guard_verify(const rtos_mem_guard_t *guard)
{
    return (guard->guard_start == RTOS_GUARD_VALUE) &&
           (guard->guard_start_inv == RTOS_GUARD_VALUE_INV);
}

#endif /* RTOS_SAFETY_MEM_ENABLE */

/*===========================================================================*/
/* Redundant Data Storage (SIL 3+)                                            */
/*===========================================================================*/

#if RTOS_SAFETY_REDUNDANT_ENABLE

/**
 * Redundant 32-bit value storage (triple modular redundancy)
 */
typedef struct rtos_redundant_u32 {
    uint32_t primary;           /**< Primary value */
    uint32_t secondary;         /**< Secondary copy */
    uint32_t tertiary;          /**< Tertiary copy (inverted) */
} rtos_redundant_u32_t;

/**
 * Set redundant value
 */
RTOS_INLINE void rtos_redundant_set(rtos_redundant_u32_t *r, uint32_t value)
{
    r->primary = value;
    r->secondary = value;
    r->tertiary = ~value;
}

/**
 * Get redundant value with voting
 */
RTOS_INLINE uint32_t rtos_redundant_get(const rtos_redundant_u32_t *r, bool *valid)
{
    uint32_t inverted_tertiary = ~r->tertiary;
    uint32_t vote_count = 0U;
    uint32_t result = r->primary;

    /* Majority voting */
    if (r->primary == r->secondary) {
        vote_count++;
        result = r->primary;
    }
    if (r->primary == inverted_tertiary) {
        vote_count++;
        result = r->primary;
    }
    if (r->secondary == inverted_tertiary) {
        vote_count++;
        result = r->secondary;
    }

    if (valid != NULL) {
        *valid = (vote_count >= 2U);
    }

    /* Return majority value (if all differ, return primary) */
    if (r->primary == r->secondary) {
        return r->primary;
    }
    if (r->primary == inverted_tertiary) {
        return r->primary;
    }
    if (r->secondary == inverted_tertiary) {
        return r->secondary;
    }

    /* No majority - return primary and flag as invalid */
    if (valid != NULL) {
        *valid = false;
    }
    return r->primary;
}

/**
 * Verify redundant value consistency
 */
RTOS_INLINE bool rtos_redundant_verify(const rtos_redundant_u32_t *r)
{
    return (r->primary == r->secondary) &&
           (r->primary == ~r->tertiary);
}

#endif /* RTOS_SAFETY_REDUNDANT_ENABLE */

/*===========================================================================*/
/* Safe State Management                                                      */
/*===========================================================================*/

/**
 * Safe state callback type
 */
typedef void (*rtos_safe_state_handler_t)(rtos_fault_severity_t severity);

/**
 * Register safe state handler
 */
void rtos_safety_set_safe_state_handler(rtos_safe_state_handler_t handler);

/**
 * Enter safe state
 */
RTOS_NORETURN void rtos_safety_enter_safe_state(rtos_fault_severity_t severity);

/**
 * Check if system is in safe state
 */
bool rtos_safety_is_safe_state(void);

/*===========================================================================*/
/* Timing Verification                                                        */
/*===========================================================================*/

/**
 * Deadline monitor structure
 */
typedef struct rtos_deadline_monitor {
    uint32_t start_tick;        /**< Start timestamp */
    uint32_t deadline_ticks;    /**< Maximum allowed duration */
    bool     active;            /**< Monitor is active */
} rtos_deadline_monitor_t;

/**
 * Start deadline monitoring
 */
void rtos_safety_deadline_start(rtos_deadline_monitor_t *monitor, uint32_t max_ticks);

/**
 * Check deadline (call periodically)
 */
bool rtos_safety_deadline_check(const rtos_deadline_monitor_t *monitor);

/**
 * Stop deadline monitoring
 */
void rtos_safety_deadline_stop(rtos_deadline_monitor_t *monitor);

/**
 * Deadline check macro with automatic fault reporting
 */
#define RTOS_DEADLINE_CHECK(monitor) \
    do { \
        if (!rtos_safety_deadline_check(monitor)) { \
            RTOS_SAFETY_FAULT(RTOS_FAULT_CAT_TIMING, RTOS_FAULT_SEVERE, 0, 0); \
        } \
    } while(0)

/*===========================================================================*/
/* Parameter Validation Macros                                                */
/*===========================================================================*/

/**
 * Validate pointer parameter (not NULL)
 */
#define RTOS_VALIDATE_PTR(ptr) \
    do { \
        if ((ptr) == NULL) { \
            RTOS_SAFETY_FAULT(RTOS_FAULT_CAT_PARAM, RTOS_FAULT_MODERATE, 0, 0); \
            return RTOS_ERR_PARAM; \
        } \
    } while(0)

/**
 * Validate pointer parameter with custom return
 */
#define RTOS_VALIDATE_PTR_RET(ptr, ret) \
    do { \
        if ((ptr) == NULL) { \
            RTOS_SAFETY_FAULT(RTOS_FAULT_CAT_PARAM, RTOS_FAULT_MODERATE, 0, 0); \
            return (ret); \
        } \
    } while(0)

/**
 * Validate range parameter
 */
#define RTOS_VALIDATE_RANGE(val, min, max) \
    do { \
        if (((val) < (min)) || ((val) > (max))) { \
            RTOS_SAFETY_FAULT(RTOS_FAULT_CAT_PARAM, RTOS_FAULT_MODERATE, \
                             (uint32_t)(val), (uint32_t)(max)); \
            return RTOS_ERR_PARAM; \
        } \
    } while(0)

/**
 * Validate alignment
 */
#define RTOS_VALIDATE_ALIGNED(ptr, align) \
    do { \
        if ((RTOS_PTR_TO_UINT(ptr) & ((align) - 1U)) != 0U) { \
            RTOS_SAFETY_FAULT(RTOS_FAULT_CAT_PARAM, RTOS_FAULT_MODERATE, \
                             (uint32_t)RTOS_PTR_TO_UINT(ptr), (align)); \
            return RTOS_ERR_PARAM; \
        } \
    } while(0)

/*===========================================================================*/
/* Diagnostic Coverage Metrics (IEC 61508-2)                                  */
/*===========================================================================*/

/**
 * Diagnostic coverage statistics
 */
typedef struct rtos_diag_stats {
    uint32_t total_checks;          /**< Total checks performed */
    uint32_t passed_checks;         /**< Checks that passed */
    uint32_t failed_checks;         /**< Checks that failed */
    uint32_t corrected_faults;      /**< Faults that were corrected */
    uint32_t uncorrected_faults;    /**< Faults that could not be corrected */
} rtos_diag_stats_t;

/**
 * Get diagnostic statistics
 */
const rtos_diag_stats_t *rtos_safety_get_diag_stats(void);

/**
 * Reset diagnostic statistics
 */
void rtos_safety_reset_diag_stats(void);

/**
 * Calculate diagnostic coverage (percentage * 100)
 */
uint32_t rtos_safety_get_diagnostic_coverage(void);

/*===========================================================================*/
/* Safety Initialization                                                      */
/*===========================================================================*/

/**
 * Initialize safety subsystem
 * Must be called before any other safety functions
 */
void rtos_safety_init(void);

/**
 * Perform periodic safety checks (call from tick handler or dedicated task)
 */
void rtos_safety_periodic_check(void);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_SAFETY_H */
