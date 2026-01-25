/**
 * MicroRTOS - ARM Cortex-M Port Implementation
 *
 * Platform-specific code for ARM Cortex-M microcontrollers
 * used in Arduino Due (SAM3X8E), Zero (SAMD21), MKR series, etc.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#if defined(__arm__) || defined(__ARM_ARCH) || defined(RTOS_PLATFORM_ARM)

#include "rtos_port.h"
#include "rtos_types.h"
#include <stdint.h>

/*===========================================================================*/
/* ARM Cortex-M Register Definitions                                          */
/*===========================================================================*/

/* System Control Block (SCB) */
#define SCB_BASE            0xE000ED00UL
#define SCB_ICSR            (*(volatile uint32_t *)(SCB_BASE + 0x04))
#define SCB_SHPR3           (*(volatile uint32_t *)(SCB_BASE + 0x20))

/* ICSR bits */
#define SCB_ICSR_PENDSVSET  (1UL << 28)
#define SCB_ICSR_PENDSVCLR  (1UL << 27)

/* SysTick */
#define SYSTICK_BASE        0xE000E010UL
#define SYSTICK_CTRL        (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_LOAD        (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_VAL         (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))

/* SysTick CTRL bits */
#define SYSTICK_CTRL_ENABLE     (1UL << 0)
#define SYSTICK_CTRL_TICKINT    (1UL << 1)
#define SYSTICK_CTRL_CLKSOURCE  (1UL << 2)

/* NVIC */
#define NVIC_INT_CTRL       (*(volatile uint32_t *)0xE000ED04UL)

/* Priority configuration */
#define PENDSV_PRIORITY     0xFF    /* Lowest priority */
#define SYSTICK_PRIORITY    0xFF    /* Lowest priority */

/*===========================================================================*/
/* External Declarations                                                      */
/*===========================================================================*/

extern rtos_tcb_t *g_current_tcb;
extern volatile uint8_t g_critical_nesting;
extern void rtos_tick_handler(void);
extern void rtos_scheduler_switch_context(void);

#if RTOS_CHECK_STACK_OVERFLOW
extern void rtos_stack_overflow_hook(rtos_tcb_t *tcb);
#endif

/*===========================================================================*/
/* Local Variables                                                            */
/*===========================================================================*/

/* Critical section nesting counter */
static volatile uint32_t port_critical_nesting = 0;

/* Saved BASEPRI for critical sections */
static volatile uint32_t saved_basepri = 0;

/*===========================================================================*/
/* Port Initialization                                                        */
/*===========================================================================*/

void rtos_port_init(void)
{
    uint32_t priority_group;

    /* Reset nesting counter */
    port_critical_nesting = 0;

    /*
     * Set PendSV and SysTick to lowest priority.
     * This ensures that context switches don't interrupt
     * other ISRs and allows ISRs to use RTOS functions.
     */
    priority_group = SCB_SHPR3;
    priority_group &= 0x0000FFFFUL;
    priority_group |= ((uint32_t)PENDSV_PRIORITY << 16);   /* PendSV */
    priority_group |= ((uint32_t)SYSTICK_PRIORITY << 24);  /* SysTick */
    SCB_SHPR3 = priority_group;

    /*
     * Configure SysTick for system tick.
     * Reload value = (clock / tick_rate) - 1
     */
    SYSTICK_VAL = 0;
    SYSTICK_LOAD = (F_CPU / RTOS_TICK_RATE_HZ) - 1;
    SYSTICK_CTRL = SYSTICK_CTRL_CLKSOURCE |
                   SYSTICK_CTRL_TICKINT |
                   SYSTICK_CTRL_ENABLE;
}

/*===========================================================================*/
/* Stack Initialization                                                       */
/*===========================================================================*/

void rtos_port_init_task_stack(rtos_tcb_t *tcb, void (*entry)(void*), void *arg)
{
    uint32_t *stack_top;

    /*
     * ARM Cortex-M stack frame (hardware saved, from high to low):
     *   xPSR
     *   PC (return address / entry point)
     *   LR (link register)
     *   R12
     *   R3
     *   R2
     *   R1
     *   R0 (argument)
     *
     * Software saved context (r4-r11):
     *   R11
     *   R10
     *   R9
     *   R8
     *   R7
     *   R6
     *   R5
     *   R4
     *
     * Stack pointer points to last pushed item (R4).
     */

    /* Start at top of stack (must be 8-byte aligned) */
    stack_top = (uint32_t *)((uint32_t)tcb->stack_base + tcb->stack_size);
    stack_top = (uint32_t *)((uint32_t)stack_top & ~7UL);  /* 8-byte align */

    /* Exception return frame (hardware saved) */
    *(--stack_top) = 0x01000000UL;              /* xPSR - Thumb bit set */
    *(--stack_top) = (uint32_t)entry;           /* PC - entry point */
    *(--stack_top) = 0xFFFFFFFDUL;              /* LR - return to thread mode with PSP */
    *(--stack_top) = 0x12121212UL;              /* R12 */
    *(--stack_top) = 0x03030303UL;              /* R3 */
    *(--stack_top) = 0x02020202UL;              /* R2 */
    *(--stack_top) = 0x01010101UL;              /* R1 */
    *(--stack_top) = (uint32_t)arg;             /* R0 - argument */

    /* Software saved context (r4-r11) */
    *(--stack_top) = 0x11111111UL;              /* R11 */
    *(--stack_top) = 0x10101010UL;              /* R10 */
    *(--stack_top) = 0x09090909UL;              /* R9 */
    *(--stack_top) = 0x08080808UL;              /* R8 */
    *(--stack_top) = 0x07070707UL;              /* R7 */
    *(--stack_top) = 0x06060606UL;              /* R6 */
    *(--stack_top) = 0x05050505UL;              /* R5 */
    *(--stack_top) = 0x04040404UL;              /* R4 */

    /* Save stack pointer in TCB */
    tcb->stack_ptr = stack_top;
}

/*===========================================================================*/
/* Scheduler Start                                                            */
/*===========================================================================*/

/**
 * Start the first task.
 * This function is written in assembly for precise control.
 */
void rtos_port_start_scheduler(void)
{
    __asm volatile (
        /* Load address of g_current_tcb */
        "ldr    r3, =g_current_tcb          \n"
        "ldr    r1, [r3]                    \n"
        /* Get stack pointer from TCB */
        "ldr    r0, [r1]                    \n"

        /* Restore r4-r11 from the stack */
        "ldmia  r0!, {r4-r11}               \n"

        /* Set PSP to the new stack pointer */
        "msr    psp, r0                     \n"

        /* Ensure we're using PSP not MSP */
        "mov    r0, #2                      \n"
        "msr    control, r0                 \n"
        "isb                                \n"

        /* Pop the exception return frame and start task */
        "pop    {r0-r3, r12, lr}            \n"
        "pop    {pc}                        \n"
    );

    /* Should never reach here */
    for (;;) { }
}

/*===========================================================================*/
/* Context Switch (PendSV Handler)                                            */
/*===========================================================================*/

/**
 * PendSV handler - performs the actual context switch.
 * This is triggered by setting the PendSV pending bit.
 */
void __attribute__((naked)) PendSV_Handler(void)
{
    __asm volatile (
        /* Disable interrupts */
        "cpsid  i                           \n"

        /* Get current PSP */
        "mrs    r0, psp                     \n"

        /* Get current TCB address */
        "ldr    r3, =g_current_tcb          \n"
        "ldr    r2, [r3]                    \n"

        /* Save r4-r11 onto the current task's stack */
        "stmdb  r0!, {r4-r11}               \n"

        /* Save the new stack pointer in the TCB */
        "str    r0, [r2]                    \n"

#if RTOS_CHECK_STACK_OVERFLOW
        /* Check for stack overflow */
        "push   {r3, lr}                    \n"
        "bl     rtos_port_check_stack_overflow \n"
        "pop    {r3, lr}                    \n"
#endif

        /* Call the scheduler to select the next task */
        "push   {r3, lr}                    \n"
        "bl     rtos_scheduler_switch_context \n"
        "pop    {r3, lr}                    \n"

        /* Get new current TCB */
        "ldr    r1, [r3]                    \n"

        /* Get stack pointer from new TCB */
        "ldr    r0, [r1]                    \n"

        /* Restore r4-r11 from new task's stack */
        "ldmia  r0!, {r4-r11}               \n"

        /* Set PSP to new stack pointer */
        "msr    psp, r0                     \n"

        /* Re-enable interrupts */
        "cpsie  i                           \n"

        /* Return from exception (to new task) */
        "bx     lr                          \n"
    );
}

/*===========================================================================*/
/* SysTick Handler                                                            */
/*===========================================================================*/

/**
 * SysTick handler - system tick interrupt.
 */
void SysTick_Handler(void)
{
    /* Call the RTOS tick handler */
    rtos_tick_handler();
}

/*===========================================================================*/
/* Yield (Trigger PendSV)                                                     */
/*===========================================================================*/

void rtos_port_yield(void)
{
    /* Set PendSV pending bit to trigger context switch */
    SCB_ICSR = SCB_ICSR_PENDSVSET;

    /* Data and instruction synchronization barriers */
    __asm volatile ("dsb" ::: "memory");
    __asm volatile ("isb");
}

/*===========================================================================*/
/* Critical Section Implementation                                            */
/*===========================================================================*/

void rtos_port_enter_critical(void)
{
    if (port_critical_nesting == 0) {
        /* Save current interrupt state and disable */
        __asm volatile (
            "mrs    %0, primask             \n"
            "cpsid  i                       \n"
            : "=r" (saved_basepri)
            :
            : "memory"
        );
    }
    port_critical_nesting++;
}

void rtos_port_exit_critical(void)
{
    if (port_critical_nesting > 0) {
        port_critical_nesting--;
        if (port_critical_nesting == 0) {
            /* Restore previous interrupt state */
            __asm volatile (
                "msr    primask, %0         \n"
                :
                : "r" (saved_basepri)
                : "memory"
            );
        }
    }
}

uint32_t rtos_port_disable_interrupts(void)
{
    uint32_t primask;

    __asm volatile (
        "mrs    %0, primask                 \n"
        "cpsid  i                           \n"
        : "=r" (primask)
        :
        : "memory"
    );

    return primask;
}

void rtos_port_restore_interrupts(uint32_t state)
{
    __asm volatile (
        "msr    primask, %0                 \n"
        :
        : "r" (state)
        : "memory"
    );
}

bool rtos_port_is_in_isr(void)
{
    uint32_t ipsr;

    __asm volatile ("mrs %0, ipsr" : "=r" (ipsr));

    /* IPSR is non-zero when in an exception handler */
    return (ipsr != 0);
}

/*===========================================================================*/
/* BASEPRI Helpers (for priority-based masking)                               */
/*===========================================================================*/

void rtos_port_set_basepri(uint32_t basepri)
{
    __asm volatile (
        "msr    basepri, %0                 \n"
        :
        : "r" (basepri)
        : "memory"
    );
}

uint32_t rtos_port_get_basepri(void)
{
    uint32_t basepri;

    __asm volatile (
        "mrs    %0, basepri                 \n"
        : "=r" (basepri)
    );

    return basepri;
}

/*===========================================================================*/
/* Stack Overflow Check                                                       */
/*===========================================================================*/

#if RTOS_CHECK_STACK_OVERFLOW

void rtos_port_check_stack_overflow(void)
{
    rtos_tcb_t *tcb = g_current_tcb;
    uint32_t *stack_limit;
    uint32_t *current_sp;

    if (tcb == NULL) {
        return;
    }

    /* Check if stack pointer is below stack base */
    stack_limit = (uint32_t *)tcb->stack_base;
    current_sp = (uint32_t *)tcb->stack_ptr;

    if (current_sp < stack_limit) {
        rtos_stack_overflow_hook(tcb);
    }

#if RTOS_CHECK_STACK_OVERFLOW >= 1
    /* Check canary pattern at bottom of stack */
    if (*((uint8_t *)tcb->stack_base) != 0xA5) {
        rtos_stack_overflow_hook(tcb);
    }
#endif
}

#endif /* RTOS_CHECK_STACK_OVERFLOW */

#endif /* __arm__ || __ARM_ARCH || RTOS_PLATFORM_ARM */
