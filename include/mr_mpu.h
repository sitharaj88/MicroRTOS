/**
 * MicroRTOS - Memory Protection Unit Support
 *
 * Hardware memory protection for ARM Cortex-M processors.
 * Provides per-task memory isolation and stack protection.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#ifndef MR_MPU_H
#define MR_MPU_H

#include "mr_config.h"
#include "mr_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Configuration                                                              */
/*===========================================================================*/

#ifndef MR_USE_MPU
#define MR_USE_MPU                    0
#endif

#ifndef MR_MPU_REGIONS_PER_TASK
#define MR_MPU_REGIONS_PER_TASK       3   /* Regions configurable per task */
#endif

#ifndef MR_MPU_TOTAL_REGIONS
#define MR_MPU_TOTAL_REGIONS          8   /* Typical ARM Cortex-M */
#endif

/*===========================================================================*/
/* MPU Region Attributes                                                      */
/*===========================================================================*/

/**
 * Memory region access permissions.
 */
typedef enum {
    MR_MPU_NO_ACCESS = 0x00,          /* No access */
    MR_MPU_PRIV_RW = 0x01,            /* Privileged RW only */
    MR_MPU_PRIV_RW_USER_RO = 0x02,    /* Privileged RW, unprivileged RO */
    MR_MPU_FULL_ACCESS = 0x03,        /* Full access */
    MR_MPU_PRIV_RO = 0x05,            /* Privileged RO only */
    MR_MPU_RO = 0x06,                 /* Privileged and unprivileged RO */
} mr_mpu_access_t;

/**
 * Memory region type attributes.
 */
typedef enum {
    MR_MPU_STRONGLY_ORDERED = 0,      /* Strongly-ordered */
    MR_MPU_DEVICE = 1,                /* Device memory */
    MR_MPU_NORMAL_WT = 2,             /* Normal, write-through */
    MR_MPU_NORMAL_WB = 3,             /* Normal, write-back */
    MR_MPU_NORMAL_NC = 4,             /* Normal, non-cacheable */
} mr_mpu_type_t;

/**
 * Memory region flags.
 */
#define MR_MPU_FLAG_ENABLE        0x01    /* Region enabled */
#define MR_MPU_FLAG_XN            0x02    /* Execute never */
#define MR_MPU_FLAG_SHAREABLE     0x04    /* Shareable (for multi-core) */
#define MR_MPU_FLAG_BUFFERABLE    0x08    /* Bufferable */
#define MR_MPU_FLAG_CACHEABLE     0x10    /* Cacheable */

/*===========================================================================*/
/* MPU Region Definition                                                      */
/*===========================================================================*/

/**
 * MPU region configuration.
 */
typedef struct mr_mpu_region {
    void                *base_addr;     /* Region base address (aligned) */
    uint32_t            size;           /* Region size in bytes */
    mr_mpu_access_t   access;         /* Access permissions */
    mr_mpu_type_t     type;           /* Memory type */
    uint8_t             flags;          /* Region flags */
    uint8_t             region_num;     /* Hardware region number */
} mr_mpu_region_t;

/**
 * Per-task MPU configuration.
 */
typedef struct mr_task_mpu_config {
    mr_mpu_region_t   regions[MR_MPU_REGIONS_PER_TASK];
    uint8_t             region_count;
    bool                enabled;
} mr_task_mpu_config_t;

/*===========================================================================*/
/* MPU Initialization                                                         */
/*===========================================================================*/

/**
 * Initialize the MPU.
 * Sets up default memory map and enables the MPU.
 */
void mr_mpu_init(void);

/**
 * Enable the MPU.
 */
void mr_mpu_enable(void);

/**
 * Disable the MPU.
 */
void mr_mpu_disable(void);

/**
 * Check if MPU is available and enabled.
 *
 * @return  true if MPU is enabled
 */
bool mr_mpu_is_enabled(void);

/**
 * Get the number of supported MPU regions.
 *
 * @return  Number of hardware regions
 */
uint8_t mr_mpu_get_region_count(void);

/*===========================================================================*/
/* Region Configuration                                                       */
/*===========================================================================*/

/**
 * Configure an MPU region.
 *
 * @param region_num    Region number (0 to N-1)
 * @param base          Base address (must be size-aligned)
 * @param size          Region size (must be power of 2, >= 32)
 * @param access        Access permissions
 * @param flags         Region flags
 * @return              MR_OK on success
 */
mr_status_t mr_mpu_configure_region(uint8_t region_num,
                                         void *base,
                                         uint32_t size,
                                         mr_mpu_access_t access,
                                         uint8_t flags);

/**
 * Disable an MPU region.
 *
 * @param region_num    Region to disable
 */
void mr_mpu_disable_region(uint8_t region_num);

/**
 * Configure multiple regions at once.
 *
 * @param regions       Array of region configurations
 * @param count         Number of regions
 * @return              MR_OK on success
 */
mr_status_t mr_mpu_configure_regions(const mr_mpu_region_t *regions,
                                          uint8_t count);

/*===========================================================================*/
/* Task Memory Protection                                                     */
/*===========================================================================*/

/**
 * Configure MPU regions for a task.
 *
 * @param tcb           Task to configure
 * @param regions       Array of region configurations
 * @param count         Number of regions (max MR_MPU_REGIONS_PER_TASK)
 * @return              MR_OK on success
 */
mr_status_t mr_mpu_configure_task(mr_tcb_t *tcb,
                                       const mr_mpu_region_t *regions,
                                       uint8_t count);

/**
 * Automatically configure stack protection for a task.
 * Sets up a region to detect stack overflow.
 *
 * @param tcb           Task to configure
 * @return              MR_OK on success
 */
mr_status_t mr_mpu_protect_stack(mr_tcb_t *tcb);

/**
 * Switch MPU configuration to a new task.
 * Called during context switch.
 *
 * @param tcb           Task to switch to
 */
void mr_mpu_switch_context(mr_tcb_t *tcb);

/*===========================================================================*/
/* Privileged/Unprivileged Mode                                               */
/*===========================================================================*/

/**
 * Drop to unprivileged mode.
 * After this call, privileged instructions will cause a fault.
 */
void mr_mpu_drop_privileges(void);

/**
 * Check if currently running in privileged mode.
 *
 * @return  true if privileged
 */
bool mr_mpu_is_privileged(void);

/**
 * Execute a function in privileged mode via supervisor call.
 *
 * @param func          Function to execute
 * @param arg           Argument to pass
 * @return              Return value from function
 */
uint32_t mr_mpu_svc_call(uint32_t (*func)(void *), void *arg);

/*===========================================================================*/
/* Fault Handling                                                             */
/*===========================================================================*/

/**
 * MPU fault information.
 */
typedef struct mr_mpu_fault_info {
    uint32_t    fault_addr;         /* Faulting address */
    uint32_t    fault_status;       /* Fault status register */
    bool        valid_addr;         /* Fault address is valid */
    bool        stack_error;        /* Stack operation caused fault */
    bool        data_access;        /* Data access violation */
    bool        instruction;        /* Instruction access violation */
} mr_mpu_fault_info_t;

/**
 * Get information about the last MPU fault.
 *
 * @param info          Pointer to fault info structure
 * @return              true if fault info is valid
 */
bool mr_mpu_get_fault_info(mr_mpu_fault_info_t *info);

/**
 * Clear fault status.
 */
void mr_mpu_clear_fault(void);

/**
 * User hook for MPU faults.
 * Called from the fault handler.
 *
 * @param tcb           Task that caused the fault
 * @param info          Fault information
 */
extern void mr_mpu_fault_hook(mr_tcb_t *tcb,
                                 const mr_mpu_fault_info_t *info);

/*===========================================================================*/
/* Shared Memory Regions                                                      */
/*===========================================================================*/

/**
 * Create a shared memory region accessible by multiple tasks.
 *
 * @param region_num    Region number to use
 * @param base          Base address
 * @param size          Size of region
 * @return              MR_OK on success
 */
mr_status_t mr_mpu_create_shared_region(uint8_t region_num,
                                             void *base,
                                             uint32_t size);

/*===========================================================================*/
/* Convenience Macros                                                         */
/*===========================================================================*/

/**
 * Calculate minimum MPU region size for a given data size.
 * MPU regions must be power of 2.
 */
#define MR_MPU_ALIGN_SIZE(size) \
    (1UL << (32 - __builtin_clz((size) - 1)))

/**
 * Check if address is aligned to size.
 */
#define MR_MPU_IS_ALIGNED(addr, size) \
    (((uint32_t)(addr) & ((size) - 1)) == 0)

#ifdef __cplusplus
}
#endif

#endif /* MR_MPU_H */
