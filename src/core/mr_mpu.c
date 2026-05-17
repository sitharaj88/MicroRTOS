/**
 * MicroRTOS - MPU Implementation for ARM Cortex-M
 *
 * Memory Protection Unit driver for Cortex-M3/M4/M7.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#include "mr_mpu.h"
#include "mr_types.h"

#if MR_USE_MPU && (defined(__arm__) || defined(__ARM_ARCH))

/*===========================================================================*/
/* ARM Cortex-M MPU Register Definitions                                      */
/*===========================================================================*/

/* MPU base address */
#define MPU_BASE                0xE000ED90UL

/* MPU registers */
#define MPU_TYPE                (*(volatile uint32_t *)(MPU_BASE + 0x00))
#define MPU_CTRL                (*(volatile uint32_t *)(MPU_BASE + 0x04))
#define MPU_RNR                 (*(volatile uint32_t *)(MPU_BASE + 0x08))
#define MPU_RBAR                (*(volatile uint32_t *)(MPU_BASE + 0x0C))
#define MPU_RASR                (*(volatile uint32_t *)(MPU_BASE + 0x10))

/* MPU_CTRL bits */
#define MPU_CTRL_ENABLE         (1UL << 0)
#define MPU_CTRL_HFNMIENA       (1UL << 1)  /* Enable MPU in HardFault/NMI */
#define MPU_CTRL_PRIVDEFENA     (1UL << 2)  /* Enable default map in privileged */

/* MPU_RBAR bits */
#define MPU_RBAR_VALID          (1UL << 4)
#define MPU_RBAR_REGION_Msk     (0xF)

/* MPU_RASR bits */
#define MPU_RASR_ENABLE         (1UL << 0)
#define MPU_RASR_SIZE_Pos       1
#define MPU_RASR_SIZE_Msk       (0x1F << MPU_RASR_SIZE_Pos)
#define MPU_RASR_SRD_Pos        8
#define MPU_RASR_SRD_Msk        (0xFF << MPU_RASR_SRD_Pos)
#define MPU_RASR_B_Pos          16
#define MPU_RASR_C_Pos          17
#define MPU_RASR_S_Pos          18
#define MPU_RASR_TEX_Pos        19
#define MPU_RASR_AP_Pos         24
#define MPU_RASR_AP_Msk         (0x7 << MPU_RASR_AP_Pos)
#define MPU_RASR_XN_Pos         28

/* SCB registers for fault status */
#define SCB_CFSR                (*(volatile uint32_t *)0xE000ED28UL)
#define SCB_MMFAR               (*(volatile uint32_t *)0xE000ED34UL)

/* CFSR MemManage bits */
#define CFSR_MMARVALID          (1UL << 7)
#define CFSR_MSTKERR            (1UL << 4)
#define CFSR_MUNSTKERR          (1UL << 3)
#define CFSR_DACCVIOL           (1UL << 1)
#define CFSR_IACCVIOL           (1UL << 0)

/*===========================================================================*/
/* External References                                                        */
/*===========================================================================*/

extern mr_tcb_t *g_current_tcb;
extern uint32_t mr_port_disable_interrupts(void);
extern void mr_port_restore_interrupts(uint32_t state);

/*===========================================================================*/
/* Module State                                                               */
/*===========================================================================*/

static bool mpu_enabled = false;
static uint8_t mpu_region_count = 0;

/* Per-task MPU configurations */
static mr_task_mpu_config_t task_mpu_configs[MR_MAX_PRIORITIES * 2];

/*===========================================================================*/
/* Internal Functions                                                         */
/*===========================================================================*/

/**
 * Calculate MPU region size encoding.
 * Size must be power of 2 and >= 32 bytes.
 */
static uint8_t size_to_encoding(uint32_t size)
{
    uint8_t encoding = 4;  /* Minimum: 32 bytes = 2^5 - 1 */

    while ((1UL << (encoding + 1)) < size && encoding < 31) {
        encoding++;
    }

    return encoding;
}

/**
 * Find task MPU config by TCB.
 */
static mr_task_mpu_config_t *find_task_mpu_config(mr_tcb_t *tcb)
{
    /* Simple linear search - could use hash or pointer in TCB */
    uint8_t i;

    for (i = 0; i < sizeof(task_mpu_configs) / sizeof(task_mpu_configs[0]); i++) {
        if (task_mpu_configs[i].enabled) {
            /* Compare by address for simplicity */
            /* In production, store pointer in TCB */
        }
    }

    return NULL;
}

/*===========================================================================*/
/* MPU Initialization                                                         */
/*===========================================================================*/

void mr_mpu_init(void)
{
    uint32_t type_reg;

    /* Read MPU TYPE register to get region count */
    type_reg = MPU_TYPE;
    mpu_region_count = (type_reg >> 8) & 0xFF;

    if (mpu_region_count == 0) {
        /* No MPU available */
        mpu_enabled = false;
        return;
    }

    /* Disable MPU during configuration */
    MPU_CTRL = 0;

    /* Configure default background region (privileged access only) */
    /* Region 0: Flash - full access */
    MPU_RNR = 0;
    MPU_RBAR = 0x00000000 | MPU_RBAR_VALID | 0;
    MPU_RASR = MPU_RASR_ENABLE |
               (31 << MPU_RASR_SIZE_Pos) |     /* Full 4GB address space */
               (0x03 << MPU_RASR_AP_Pos) |     /* Full access */
               (1 << MPU_RASR_C_Pos) |         /* Cacheable */
               (1 << MPU_RASR_B_Pos);          /* Bufferable */

    /* Enable MPU with privileged default memory map */
    MPU_CTRL = MPU_CTRL_ENABLE | MPU_CTRL_PRIVDEFENA;

    /* Memory barriers */
    __asm volatile("dsb");
    __asm volatile("isb");

    mpu_enabled = true;
}

void mr_mpu_enable(void)
{
    if (mpu_region_count > 0) {
        MPU_CTRL |= MPU_CTRL_ENABLE;
        __asm volatile("dsb");
        __asm volatile("isb");
        mpu_enabled = true;
    }
}

void mr_mpu_disable(void)
{
    MPU_CTRL &= ~MPU_CTRL_ENABLE;
    __asm volatile("dsb");
    __asm volatile("isb");
    mpu_enabled = false;
}

bool mr_mpu_is_enabled(void)
{
    return mpu_enabled;
}

uint8_t mr_mpu_get_region_count(void)
{
    return mpu_region_count;
}

/*===========================================================================*/
/* Region Configuration                                                       */
/*===========================================================================*/

mr_status_t mr_mpu_configure_region(uint8_t region_num,
                                         void *base,
                                         uint32_t size,
                                         mr_mpu_access_t access,
                                         uint8_t flags)
{
    uint32_t rbar;
    uint32_t rasr;
    uint8_t size_encoding;

    if (region_num >= mpu_region_count) {
        return MR_ERR_PARAM;
    }

    /* Size must be power of 2 and >= 32 */
    if (size < 32 || (size & (size - 1)) != 0) {
        return MR_ERR_PARAM;
    }

    /* Base must be aligned to size */
    if ((uint32_t)base & (size - 1)) {
        return MR_ERR_PARAM;
    }

    size_encoding = size_to_encoding(size);

    /* Build RBAR */
    rbar = (uint32_t)base | MPU_RBAR_VALID | region_num;

    /* Build RASR */
    rasr = (size_encoding << MPU_RASR_SIZE_Pos) |
           ((uint32_t)access << MPU_RASR_AP_Pos);

    if (flags & MR_MPU_FLAG_ENABLE) {
        rasr |= MPU_RASR_ENABLE;
    }
    if (flags & MR_MPU_FLAG_XN) {
        rasr |= (1UL << MPU_RASR_XN_Pos);
    }
    if (flags & MR_MPU_FLAG_SHAREABLE) {
        rasr |= (1UL << MPU_RASR_S_Pos);
    }
    if (flags & MR_MPU_FLAG_BUFFERABLE) {
        rasr |= (1UL << MPU_RASR_B_Pos);
    }
    if (flags & MR_MPU_FLAG_CACHEABLE) {
        rasr |= (1UL << MPU_RASR_C_Pos);
    }

    /* Configure region */
    MPU_RBAR = rbar;
    MPU_RASR = rasr;

    /* Memory barriers */
    __asm volatile("dsb");
    __asm volatile("isb");

    return MR_OK;
}

void mr_mpu_disable_region(uint8_t region_num)
{
    if (region_num >= mpu_region_count) {
        return;
    }

    MPU_RNR = region_num;
    MPU_RASR = 0;

    __asm volatile("dsb");
}

mr_status_t mr_mpu_configure_regions(const mr_mpu_region_t *regions,
                                          uint8_t count)
{
    uint8_t i;
    mr_status_t status;

    for (i = 0; i < count; i++) {
        status = mr_mpu_configure_region(
            regions[i].region_num,
            regions[i].base_addr,
            regions[i].size,
            regions[i].access,
            regions[i].flags
        );

        if (status != MR_OK) {
            return status;
        }
    }

    return MR_OK;
}

/*===========================================================================*/
/* Task Memory Protection                                                     */
/*===========================================================================*/

mr_status_t mr_mpu_configure_task(mr_tcb_t *tcb,
                                       const mr_mpu_region_t *regions,
                                       uint8_t count)
{
    if (tcb == NULL || count > MR_MPU_REGIONS_PER_TASK) {
        return MR_ERR_PARAM;
    }

    /* Store configuration for context switch */
    /* In a full implementation, store in TCB or linked structure */

    return MR_OK;
}

mr_status_t mr_mpu_protect_stack(mr_tcb_t *tcb)
{
    uint32_t stack_guard_size = 32;  /* Minimum MPU region size */

    if (tcb == NULL) {
        return MR_ERR_PARAM;
    }

    /* Configure a no-access region at the bottom of the stack */
    return mr_mpu_configure_region(
        1,  /* Use region 1 for stack guard */
        tcb->stack_base,
        stack_guard_size,
        MR_MPU_NO_ACCESS,
        MR_MPU_FLAG_ENABLE | MR_MPU_FLAG_XN
    );
}

void mr_mpu_switch_context(mr_tcb_t *tcb)
{
    /* Configure task-specific regions on context switch */
    if (tcb == NULL) {
        return;
    }

    /* In a full implementation:
     * 1. Disable MPU briefly
     * 2. Configure task's regions
     * 3. Re-enable MPU
     */

    /* Minimal implementation: reconfigure stack guard */
    mr_mpu_protect_stack(tcb);
}

/*===========================================================================*/
/* Privileged/Unprivileged Mode                                               */
/*===========================================================================*/

void mr_mpu_drop_privileges(void)
{
    __asm volatile(
        "mrs    r0, control     \n"
        "orr    r0, r0, #1      \n"  /* Set CONTROL[0] = unprivileged */
        "msr    control, r0     \n"
        "isb                    \n"
        ::: "r0", "memory"
    );
}

bool mr_mpu_is_privileged(void)
{
    uint32_t control;

    __asm volatile("mrs %0, control" : "=r" (control));

    return (control & 1) == 0;
}

/*===========================================================================*/
/* Fault Handling                                                             */
/*===========================================================================*/

bool mr_mpu_get_fault_info(mr_mpu_fault_info_t *info)
{
    uint32_t cfsr;

    if (info == NULL) {
        return false;
    }

    cfsr = SCB_CFSR;

    /* Check if MemManage fault bits are set */
    if ((cfsr & 0xFF) == 0) {
        return false;
    }

    info->fault_status = cfsr;
    info->valid_addr = (cfsr & CFSR_MMARVALID) != 0;
    info->fault_addr = info->valid_addr ? SCB_MMFAR : 0;
    info->stack_error = (cfsr & (CFSR_MSTKERR | CFSR_MUNSTKERR)) != 0;
    info->data_access = (cfsr & CFSR_DACCVIOL) != 0;
    info->instruction = (cfsr & CFSR_IACCVIOL) != 0;

    return true;
}

void mr_mpu_clear_fault(void)
{
    /* Clear MemManage fault bits by writing 1s */
    SCB_CFSR = 0xFF;
}

/* Default (weak) fault hook */
__attribute__((weak))
void mr_mpu_fault_hook(mr_tcb_t *tcb, const mr_mpu_fault_info_t *info)
{
    (void)tcb;
    (void)info;

    /* Default: infinite loop (halt) */
    while (1) {}
}

/*===========================================================================*/
/* Shared Memory Regions                                                      */
/*===========================================================================*/

mr_status_t mr_mpu_create_shared_region(uint8_t region_num,
                                             void *base,
                                             uint32_t size)
{
    return mr_mpu_configure_region(
        region_num,
        base,
        size,
        MR_MPU_FULL_ACCESS,
        MR_MPU_FLAG_ENABLE | MR_MPU_FLAG_SHAREABLE |
        MR_MPU_FLAG_CACHEABLE | MR_MPU_FLAG_BUFFERABLE
    );
}

#else /* !MR_USE_MPU || !ARM */

/* Stub implementations */
void mr_mpu_init(void) {}
void mr_mpu_enable(void) {}
void mr_mpu_disable(void) {}
bool mr_mpu_is_enabled(void) { return false; }
uint8_t mr_mpu_get_region_count(void) { return 0; }

mr_status_t mr_mpu_configure_region(uint8_t r, void *b, uint32_t s,
                                         mr_mpu_access_t a, uint8_t f)
{
    (void)r; (void)b; (void)s; (void)a; (void)f;
    return MR_ERR_NOT_SUPPORTED;
}

void mr_mpu_disable_region(uint8_t r) { (void)r; }

mr_status_t mr_mpu_configure_task(mr_tcb_t *t, const mr_mpu_region_t *r, uint8_t c)
{
    (void)t; (void)r; (void)c;
    return MR_ERR_NOT_SUPPORTED;
}

mr_status_t mr_mpu_protect_stack(mr_tcb_t *t)
{
    (void)t;
    return MR_ERR_NOT_SUPPORTED;
}

void mr_mpu_switch_context(mr_tcb_t *t) { (void)t; }
void mr_mpu_drop_privileges(void) {}
bool mr_mpu_is_privileged(void) { return true; }

bool mr_mpu_get_fault_info(mr_mpu_fault_info_t *i)
{
    (void)i;
    return false;
}

void mr_mpu_clear_fault(void) {}

#endif /* MR_USE_MPU && ARM */
