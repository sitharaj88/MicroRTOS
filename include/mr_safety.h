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

#ifndef MR_SAFETY_H
#define MR_SAFETY_H

#include <stdint.h>
#include <stdbool.h>
#include "mr_config.h"
#include "mr_misra.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Safety Configuration                                                       */
/*===========================================================================*/

/**
 * Enable safety mechanisms (set to 1 for safety-critical applications)
 */
#ifndef MR_SAFETY_ENABLE
    #define MR_SAFETY_ENABLE          1
#endif

/**
 * Safety integrity level (1-4 per IEC 61508)
 */
#ifndef MR_SAFETY_SIL
    #define MR_SAFETY_SIL             2
#endif

/**
 * Enable control flow monitoring
 */
#ifndef MR_SAFETY_CFM_ENABLE
    #define MR_SAFETY_CFM_ENABLE      (MR_SAFETY_SIL >= 2)
#endif

/**
 * Enable memory integrity checks
 */
#ifndef MR_SAFETY_MEM_ENABLE
    #define MR_SAFETY_MEM_ENABLE      (MR_SAFETY_SIL >= 2)
#endif

/**
 * Enable redundant data storage
 */
#ifndef MR_SAFETY_REDUNDANT_ENABLE
    #define MR_SAFETY_REDUNDANT_ENABLE (MR_SAFETY_SIL >= 3)
#endif

/*===========================================================================*/
/* Fault Classification (IEC 61508-2)                                         */
/*===========================================================================*/

/**
 * Fault severity levels
 */
typedef enum mr_fault_severity {
    MR_FAULT_MINOR        = 0,    /**< Minor fault, continue operation */
    MR_FAULT_MODERATE     = 1,    /**< Moderate fault, degraded operation */
    MR_FAULT_SEVERE       = 2,    /**< Severe fault, safe state required */
    MR_FAULT_CRITICAL     = 3,    /**< Critical fault, immediate shutdown */
} mr_fault_severity_t;

/**
 * Fault categories
 */
typedef enum mr_fault_category {
    MR_FAULT_CAT_NONE         = 0,    /**< No fault */
    MR_FAULT_CAT_STACK        = 1,    /**< Stack overflow/corruption */
    MR_FAULT_CAT_MEMORY       = 2,    /**< Memory corruption */
    MR_FAULT_CAT_TIMING       = 3,    /**< Timing violation */
    MR_FAULT_CAT_WATCHDOG     = 4,    /**< Watchdog timeout */
    MR_FAULT_CAT_ASSERTION    = 5,    /**< Assertion failure */
    MR_FAULT_CAT_CONTROL_FLOW = 6,    /**< Control flow error */
    MR_FAULT_CAT_PARAM        = 7,    /**< Parameter validation failure */
    MR_FAULT_CAT_STATE        = 8,    /**< Invalid state transition */
    MR_FAULT_CAT_RESOURCE     = 9,    /**< Resource exhaustion */
    MR_FAULT_CAT_HARDWARE     = 10,   /**< Hardware fault */
    MR_FAULT_CAT_USER         = 11,   /**< User-defined fault */
} mr_fault_category_t;

/**
 * Fault information structure
 */
typedef struct mr_fault_info {
    mr_fault_category_t category;     /**< Fault category */
    mr_fault_severity_t severity;     /**< Fault severity */
    uint32_t              fault_code;   /**< Specific fault code */
    uint32_t              fault_data;   /**< Additional fault data */
    const char           *file;         /**< Source file (if available) */
    uint32_t              line;         /**< Source line (if available) */
    uint32_t              timestamp;    /**< Fault timestamp (tick count) */
} mr_fault_info_t;

/*===========================================================================*/
/* Fault Handler Interface                                                    */
/*===========================================================================*/

/**
 * Fault handler callback type
 */
typedef void (*mr_fault_handler_t)(const mr_fault_info_t *info);

/**
 * Register a custom fault handler
 *
 * @param handler   Fault handler callback
 */
void mr_safety_set_fault_handler(mr_fault_handler_t handler);

/**
 * Report a fault to the safety subsystem
 *
 * @param category  Fault category
 * @param severity  Fault severity
 * @param code      Specific fault code
 * @param data      Additional data
 */
void mr_safety_report_fault(
    mr_fault_category_t category,
    mr_fault_severity_t severity,
    uint32_t code,
    uint32_t data);

/**
 * Report fault with source location
 */
#define MR_SAFETY_FAULT(cat, sev, code, data) \
    mr_safety_report_fault_loc((cat), (sev), (code), (data), __FILE__, __LINE__)

void mr_safety_report_fault_loc(
    mr_fault_category_t category,
    mr_fault_severity_t severity,
    uint32_t code,
    uint32_t data,
    const char *file,
    uint32_t line);

/*===========================================================================*/
/* Control Flow Monitoring (CFM)                                              */
/*===========================================================================*/

#if MR_SAFETY_CFM_ENABLE

/**
 * Control flow signature for function verification
 * Each critical function has a unique signature
 */
typedef uint32_t mr_cfm_signature_t;

/**
 * Initialize CFM context for a function
 */
#define MR_CFM_INIT(sig)          mr_cfm_init(sig)

/**
 * Update CFM checkpoint within a function
 */
#define MR_CFM_CHECKPOINT(id)     mr_cfm_checkpoint(id)

/**
 * Verify CFM at function exit
 */
#define MR_CFM_VERIFY(expected)   mr_cfm_verify(expected)

/**
 * CFM function prototypes
 */
void mr_cfm_init(mr_cfm_signature_t initial_sig);
void mr_cfm_checkpoint(uint32_t checkpoint_id);
bool mr_cfm_verify(mr_cfm_signature_t expected_sig);

/**
 * Generate CFM signature from checkpoint sequence
 */
#define MR_CFM_SIG(base, cp1, cp2, cp3) \
    (((base) ^ ((cp1) << 8)) ^ ((cp2) << 16) ^ ((cp3) << 24))

#else /* !MR_SAFETY_CFM_ENABLE */

#define MR_CFM_INIT(sig)          ((void)0)
#define MR_CFM_CHECKPOINT(id)     ((void)0)
#define MR_CFM_VERIFY(expected)   (true)

#endif /* MR_SAFETY_CFM_ENABLE */

/*===========================================================================*/
/* Memory Integrity Checking                                                  */
/*===========================================================================*/

#if MR_SAFETY_MEM_ENABLE

/**
 * Memory region descriptor for integrity checking
 */
typedef struct mr_mem_region {
    void       *base;           /**< Region base address */
    uint32_t    size;           /**< Region size in bytes */
    uint32_t    checksum;       /**< Calculated checksum */
    bool        read_only;      /**< Read-only region flag */
} mr_mem_region_t;

/**
 * Calculate CRC32 checksum of memory region
 */
uint32_t mr_safety_crc32(const void *data, uint32_t size);

/**
 * Initialize memory region for integrity monitoring
 */
void mr_safety_mem_init(mr_mem_region_t *region, void *base, uint32_t size, bool read_only);

/**
 * Update checksum for a memory region
 */
void mr_safety_mem_update(mr_mem_region_t *region);

/**
 * Verify memory region integrity
 */
bool mr_safety_mem_verify(const mr_mem_region_t *region);

/**
 * Guard value for memory corruption detection
 */
#define MR_GUARD_VALUE        0xDEADBEEFUL
#define MR_GUARD_VALUE_INV    0x21524110UL  /* ~MR_GUARD_VALUE */

/**
 * Memory guard structure (place before/after critical data)
 */
typedef struct mr_mem_guard {
    uint32_t guard_start;       /**< Start guard (MR_GUARD_VALUE) */
    uint32_t guard_start_inv;   /**< Inverted start guard */
} mr_mem_guard_t;

/**
 * Initialize memory guard
 */
MR_INLINE void mr_mem_guard_init(mr_mem_guard_t *guard)
{
    guard->guard_start = MR_GUARD_VALUE;
    guard->guard_start_inv = MR_GUARD_VALUE_INV;
}

/**
 * Verify memory guard
 */
MR_INLINE bool mr_mem_guard_verify(const mr_mem_guard_t *guard)
{
    return (guard->guard_start == MR_GUARD_VALUE) &&
           (guard->guard_start_inv == MR_GUARD_VALUE_INV);
}

#endif /* MR_SAFETY_MEM_ENABLE */

/*===========================================================================*/
/* Redundant Data Storage (SIL 3+)                                            */
/*===========================================================================*/

#if MR_SAFETY_REDUNDANT_ENABLE

/**
 * Redundant 32-bit value storage (triple modular redundancy)
 */
typedef struct mr_redundant_u32 {
    uint32_t primary;           /**< Primary value */
    uint32_t secondary;         /**< Secondary copy */
    uint32_t tertiary;          /**< Tertiary copy (inverted) */
} mr_redundant_u32_t;

/**
 * Set redundant value
 */
MR_INLINE void mr_redundant_set(mr_redundant_u32_t *r, uint32_t value)
{
    r->primary = value;
    r->secondary = value;
    r->tertiary = ~value;
}

/**
 * Get redundant value with voting
 */
MR_INLINE uint32_t mr_redundant_get(const mr_redundant_u32_t *r, bool *valid)
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
MR_INLINE bool mr_redundant_verify(const mr_redundant_u32_t *r)
{
    return (r->primary == r->secondary) &&
           (r->primary == ~r->tertiary);
}

#endif /* MR_SAFETY_REDUNDANT_ENABLE */

/*===========================================================================*/
/* Safe State Management                                                      */
/*===========================================================================*/

/**
 * Safe state callback type
 */
typedef void (*mr_safe_state_handler_t)(mr_fault_severity_t severity);

/**
 * Register safe state handler
 */
void mr_safety_set_safe_state_handler(mr_safe_state_handler_t handler);

/**
 * Enter safe state
 */
MR_NORETURN void mr_safety_enter_safe_state(mr_fault_severity_t severity);

/**
 * Check if system is in safe state
 */
bool mr_safety_is_safe_state(void);

/*===========================================================================*/
/* Timing Verification                                                        */
/*===========================================================================*/

/**
 * Deadline monitor structure
 */
typedef struct mr_deadline_monitor {
    uint32_t start_tick;        /**< Start timestamp */
    uint32_t deadline_ticks;    /**< Maximum allowed duration */
    bool     active;            /**< Monitor is active */
} mr_deadline_monitor_t;

/**
 * Start deadline monitoring
 */
void mr_safety_deadline_start(mr_deadline_monitor_t *monitor, uint32_t max_ticks);

/**
 * Check deadline (call periodically)
 */
bool mr_safety_deadline_check(const mr_deadline_monitor_t *monitor);

/**
 * Stop deadline monitoring
 */
void mr_safety_deadline_stop(mr_deadline_monitor_t *monitor);

/**
 * Deadline check macro with automatic fault reporting
 */
#define MR_DEADLINE_CHECK(monitor) \
    do { \
        if (!mr_safety_deadline_check(monitor)) { \
            MR_SAFETY_FAULT(MR_FAULT_CAT_TIMING, MR_FAULT_SEVERE, 0, 0); \
        } \
    } while(0)

/*===========================================================================*/
/* Parameter Validation Macros                                                */
/*===========================================================================*/

/**
 * Validate pointer parameter (not NULL)
 */
#define MR_VALIDATE_PTR(ptr) \
    do { \
        if ((ptr) == NULL) { \
            MR_SAFETY_FAULT(MR_FAULT_CAT_PARAM, MR_FAULT_MODERATE, 0, 0); \
            return MR_ERR_PARAM; \
        } \
    } while(0)

/**
 * Validate pointer parameter with custom return
 */
#define MR_VALIDATE_PTR_RET(ptr, ret) \
    do { \
        if ((ptr) == NULL) { \
            MR_SAFETY_FAULT(MR_FAULT_CAT_PARAM, MR_FAULT_MODERATE, 0, 0); \
            return (ret); \
        } \
    } while(0)

/**
 * Validate range parameter
 */
#define MR_VALIDATE_RANGE(val, min, max) \
    do { \
        if (((val) < (min)) || ((val) > (max))) { \
            MR_SAFETY_FAULT(MR_FAULT_CAT_PARAM, MR_FAULT_MODERATE, \
                             (uint32_t)(val), (uint32_t)(max)); \
            return MR_ERR_PARAM; \
        } \
    } while(0)

/**
 * Validate alignment
 */
#define MR_VALIDATE_ALIGNED(ptr, align) \
    do { \
        if ((MR_PTR_TO_UINT(ptr) & ((align) - 1U)) != 0U) { \
            MR_SAFETY_FAULT(MR_FAULT_CAT_PARAM, MR_FAULT_MODERATE, \
                             (uint32_t)MR_PTR_TO_UINT(ptr), (align)); \
            return MR_ERR_PARAM; \
        } \
    } while(0)

/*===========================================================================*/
/* Diagnostic Coverage Metrics (IEC 61508-2)                                  */
/*===========================================================================*/

/**
 * Diagnostic coverage statistics
 */
typedef struct mr_diag_stats {
    uint32_t total_checks;          /**< Total checks performed */
    uint32_t passed_checks;         /**< Checks that passed */
    uint32_t failed_checks;         /**< Checks that failed */
    uint32_t corrected_faults;      /**< Faults that were corrected */
    uint32_t uncorrected_faults;    /**< Faults that could not be corrected */
} mr_diag_stats_t;

/**
 * Get diagnostic statistics
 */
const mr_diag_stats_t *mr_safety_get_diag_stats(void);

/**
 * Reset diagnostic statistics
 */
void mr_safety_reset_diag_stats(void);

/**
 * Calculate diagnostic coverage (percentage * 100)
 */
uint32_t mr_safety_get_diagnostic_coverage(void);

/*===========================================================================*/
/* Safety Initialization                                                      */
/*===========================================================================*/

/**
 * Initialize safety subsystem
 * Must be called before any other safety functions
 */
void mr_safety_init(void);

/**
 * Perform periodic safety checks (call from tick handler or dedicated task)
 */
void mr_safety_periodic_check(void);

#ifdef __cplusplus
}
#endif

#endif /* MR_SAFETY_H */
