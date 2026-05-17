/**
 * MicroRTOS - AVR Port Implementation
 *
 * Platform-specific code for AVR microcontrollers (ATmega328P,
 * ATmega2560, etc.) used in Arduino Uno, Mega, Nano, etc.
 *
 * Copyright (c) 2026 MicroRTOS Project
 * SPDX-License-Identifier: MIT
 */

#if defined(__AVR__) || defined(RTOS_PLATFORM_AVR)

#include "rtos_port.h"
#include "rtos_types.h"
#include <avr/io.h>
#include <avr/interrupt.h>

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

/* Critical section nesting counter for port */
static volatile uint8_t port_critical_nesting = 0;

/* Flag indicating if we're in an ISR */
static volatile uint8_t in_isr = 0;

/*===========================================================================*/
/* Timer Configuration                                                        */
/*===========================================================================*/

/*
 * We use Timer0 for the system tick on AVR.
 * Timer0 is an 8-bit timer available on all ATmega devices.
 *
 * For a 16MHz clock and 1kHz tick rate (1ms period):
 * - Prescaler = 64
 * - Compare value = (16000000 / 64 / 1000) - 1 = 249
 */

#define TIMER_PRESCALER     64
#define TIMER_COMPARE_VALUE ((F_CPU / TIMER_PRESCALER / RTOS_TICK_RATE_HZ) - 1)

#if TIMER_COMPARE_VALUE > 255
    #error "Timer compare value too large - adjust prescaler or tick rate"
#endif

/*===========================================================================*/
/* Port Initialization                                                        */
/*===========================================================================*/

void rtos_port_init(void)
{
    /* Disable interrupts during setup */
    cli();

    /* Reset nesting counters */
    port_critical_nesting = 0;
    in_isr = 0;

    /*
     * Configure Timer0 for CTC mode (Clear Timer on Compare)
     * with interrupt on compare match.
     */

    /* Stop timer */
    TCCR0B = 0;

    /* Reset counter */
    TCNT0 = 0;

    /* Set compare value */
    OCR0A = TIMER_COMPARE_VALUE;

    /* CTC mode (WGM01:0 = 2) */
    TCCR0A = (1 << WGM01);

    /* Enable compare match A interrupt */
    TIMSK0 = (1 << OCIE0A);

    /* Start timer with prescaler = 64 (CS01 + CS00) */
    TCCR0B = (1 << CS01) | (1 << CS00);
}

/*===========================================================================*/
/* Context Switch Implementation                                              */
/*===========================================================================*/

/**
 * Save context macro for AVR.
 * Saves all 32 general purpose registers, SREG, and updates stack pointer.
 */
#define SAVE_CONTEXT()                                          \
    __asm__ volatile (                                              \
        "push   r0                              \n\t"           \
        "in     r0, __SREG__                    \n\t"           \
        "cli                                    \n\t"           \
        "push   r0                              \n\t"           \
        "push   r1                              \n\t"           \
        "clr    r1                              \n\t"           \
        "push   r2                              \n\t"           \
        "push   r3                              \n\t"           \
        "push   r4                              \n\t"           \
        "push   r5                              \n\t"           \
        "push   r6                              \n\t"           \
        "push   r7                              \n\t"           \
        "push   r8                              \n\t"           \
        "push   r9                              \n\t"           \
        "push   r10                             \n\t"           \
        "push   r11                             \n\t"           \
        "push   r12                             \n\t"           \
        "push   r13                             \n\t"           \
        "push   r14                             \n\t"           \
        "push   r15                             \n\t"           \
        "push   r16                             \n\t"           \
        "push   r17                             \n\t"           \
        "push   r18                             \n\t"           \
        "push   r19                             \n\t"           \
        "push   r20                             \n\t"           \
        "push   r21                             \n\t"           \
        "push   r22                             \n\t"           \
        "push   r23                             \n\t"           \
        "push   r24                             \n\t"           \
        "push   r25                             \n\t"           \
        "push   r26                             \n\t"           \
        "push   r27                             \n\t"           \
        "push   r28                             \n\t"           \
        "push   r29                             \n\t"           \
        "push   r30                             \n\t"           \
        "push   r31                             \n\t"           \
        "lds    r26, g_current_tcb              \n\t"           \
        "lds    r27, g_current_tcb + 1          \n\t"           \
        "in     r0, __SP_L__                    \n\t"           \
        "st     x+, r0                          \n\t"           \
        "in     r0, __SP_H__                    \n\t"           \
        "st     x+, r0                          \n\t"           \
    )

/**
 * Restore context macro for AVR.
 * Restores stack pointer, all registers, and SREG.
 */
#define RESTORE_CONTEXT()                                       \
    __asm__ volatile (                                              \
        "lds    r26, g_current_tcb              \n\t"           \
        "lds    r27, g_current_tcb + 1          \n\t"           \
        "ld     r28, x+                         \n\t"           \
        "out    __SP_L__, r28                   \n\t"           \
        "ld     r29, x+                         \n\t"           \
        "out    __SP_H__, r29                   \n\t"           \
        "pop    r31                             \n\t"           \
        "pop    r30                             \n\t"           \
        "pop    r29                             \n\t"           \
        "pop    r28                             \n\t"           \
        "pop    r27                             \n\t"           \
        "pop    r26                             \n\t"           \
        "pop    r25                             \n\t"           \
        "pop    r24                             \n\t"           \
        "pop    r23                             \n\t"           \
        "pop    r22                             \n\t"           \
        "pop    r21                             \n\t"           \
        "pop    r20                             \n\t"           \
        "pop    r19                             \n\t"           \
        "pop    r18                             \n\t"           \
        "pop    r17                             \n\t"           \
        "pop    r16                             \n\t"           \
        "pop    r15                             \n\t"           \
        "pop    r14                             \n\t"           \
        "pop    r13                             \n\t"           \
        "pop    r12                             \n\t"           \
        "pop    r11                             \n\t"           \
        "pop    r10                             \n\t"           \
        "pop    r9                              \n\t"           \
        "pop    r8                              \n\t"           \
        "pop    r7                              \n\t"           \
        "pop    r6                              \n\t"           \
        "pop    r5                              \n\t"           \
        "pop    r4                              \n\t"           \
        "pop    r3                              \n\t"           \
        "pop    r2                              \n\t"           \
        "pop    r1                              \n\t"           \
        "pop    r0                              \n\t"           \
        "out    __SREG__, r0                    \n\t"           \
        "pop    r0                              \n\t"           \
    )

/*===========================================================================*/
/* Stack Initialization                                                       */
/*===========================================================================*/

void rtos_port_init_task_stack(rtos_tcb_t *tcb, void (*entry)(void*), void *arg)
{
    uint8_t *stack_top;
    uint16_t entry_addr;

    /*
     * AVR stack grows downward. Stack pointer points to next free location.
     * Initial stack layout (from top to bottom):
     *
     * [High address]
     *   Return address (PC) - 2 bytes (3 on mega2560)
     *   r0 (saved at interrupt entry)
     *   SREG
     *   r1 (must be 0 for C runtime)
     *   r2-r23 (callee-saved and temporary)
     *   r24-r25 (arg parameter - low byte in r24)
     *   r26-r31 (X, Y, Z registers)
     * [Low address - stack_ptr]
     */

    /* Start at top of stack area */
    stack_top = (uint8_t *)tcb->stack_base + tcb->stack_size - 1;

    /* Push return address (entry point) - little endian, low byte first */
    entry_addr = (uint16_t)entry;

#if defined(__AVR_3_BYTE_PC__)
    /* ATmega2560 and similar with >128KB flash use 3-byte PC */
    *stack_top-- = 0;  /* High byte (usually 0) */
#endif
    *stack_top-- = (uint8_t)(entry_addr >> 8);   /* High byte */
    *stack_top-- = (uint8_t)(entry_addr & 0xFF); /* Low byte */

    /* Push initial register values */
    *stack_top-- = 0x00;    /* r0 */
    *stack_top-- = 0x80;    /* SREG - interrupts enabled (I bit set) */
    *stack_top-- = 0x00;    /* r1 - must be 0 */

    /* r2-r23: general purpose, initialize to 0 */
    for (int i = 2; i <= 23; i++) {
        *stack_top-- = 0x00;
    }

    /* r24-r25: argument (void *arg) - little endian */
    *stack_top-- = (uint8_t)((uint16_t)arg & 0xFF);        /* r24 - low byte */
    *stack_top-- = (uint8_t)(((uint16_t)arg >> 8) & 0xFF); /* r25 - high byte */

    /* r26-r31: X, Y, Z registers */
    *stack_top-- = 0x00;    /* r26 (XL) */
    *stack_top-- = 0x00;    /* r27 (XH) */
    *stack_top-- = 0x00;    /* r28 (YL) */
    *stack_top-- = 0x00;    /* r29 (YH) */
    *stack_top-- = 0x00;    /* r30 (ZL) */
    *stack_top-- = 0x00;    /* r31 (ZH) */

    /* Save stack pointer in TCB */
    tcb->stack_ptr = stack_top;
}

/*===========================================================================*/
/* Scheduler Start                                                            */
/*===========================================================================*/

void rtos_port_start_scheduler(void)
{
    /* Restore context of first task and start it */
    RESTORE_CONTEXT();

    /* Enable interrupts and return to task */
    __asm__ volatile ("reti");

    /* Should never reach here */
    for (;;) { }
}

/*===========================================================================*/
/* Context Switch (Yield)                                                     */
/*===========================================================================*/

void rtos_port_yield(void)
{
    /* Trigger a software context switch */
    /* Save context, switch task, restore context */

    SAVE_CONTEXT();

#if RTOS_CHECK_STACK_OVERFLOW
    rtos_port_check_stack_overflow();
#endif

    rtos_scheduler_switch_context();

    RESTORE_CONTEXT();

    /* Return continues in new task */
}

/*===========================================================================*/
/* Timer Interrupt Handler                                                    */
/*===========================================================================*/

/**
 * Timer0 Compare Match A interrupt handler.
 * This is the system tick interrupt.
 */
ISR(TIMER0_COMPA_vect, ISR_NAKED)
{
    /* Mark that we're in ISR */
    in_isr = 1;

    /* Save context */
    SAVE_CONTEXT();

#if RTOS_CHECK_STACK_OVERFLOW
    rtos_port_check_stack_overflow();
#endif

    /* Call the RTOS tick handler */
    rtos_tick_handler();

    /* Select next task if needed */
    rtos_scheduler_switch_context();

    /* No longer in ISR */
    in_isr = 0;

    /* Restore context (possibly different task) */
    RESTORE_CONTEXT();

    /* Return from interrupt */
    __asm__ volatile ("reti");
}

/*===========================================================================*/
/* Critical Section Implementation                                            */
/*===========================================================================*/

void rtos_port_enter_critical(void)
{
    cli();  /* Disable interrupts */
    port_critical_nesting++;
}

void rtos_port_exit_critical(void)
{
    if (port_critical_nesting > 0) {
        port_critical_nesting--;
        if (port_critical_nesting == 0) {
            sei();  /* Enable interrupts */
        }
    }
}

uint32_t rtos_port_disable_interrupts(void)
{
    uint8_t sreg = SREG;
    cli();
    return sreg;
}

void rtos_port_restore_interrupts(uint32_t state)
{
    SREG = (uint8_t)state;
}

bool rtos_port_is_in_isr(void)
{
    return (in_isr != 0);
}

/*===========================================================================*/
/* Stack Overflow Check                                                       */
/*===========================================================================*/

#if RTOS_CHECK_STACK_OVERFLOW

void rtos_port_check_stack_overflow(void)
{
    rtos_tcb_t *tcb = g_current_tcb;
    uint8_t *stack_limit;
    uint8_t *current_sp;

    if (tcb == NULL) {
        return;
    }

    /* Check if stack pointer is below stack base */
    stack_limit = (uint8_t *)tcb->stack_base;
    current_sp = (uint8_t *)tcb->stack_ptr;

    if (current_sp < stack_limit) {
        rtos_stack_overflow_hook(tcb);
    }

#if RTOS_CHECK_STACK_OVERFLOW >= 1
    /* Check canary at bottom of stack */
    if (*stack_limit != 0xA5) {
        rtos_stack_overflow_hook(tcb);
    }
#endif
}

#endif /* RTOS_CHECK_STACK_OVERFLOW */

/*===========================================================================*/
/* Tickless Idle Support                                                      */
/*===========================================================================*/

#if RTOS_USE_TICKLESS_IDLE

#include "rtos_tickless.h"
#include <avr/sleep.h>
#include <avr/power.h>

/* Saved timer state for tickless mode */
static volatile uint8_t tickless_saved_ocr = 0;
static volatile uint32_t tickless_expected_ticks = 0;
static volatile uint32_t tickless_elapsed_ticks = 0;
static volatile bool tickless_active = false;

/* Use Timer1 for extended tickless periods (16-bit timer) */
static volatile uint16_t tickless_timer1_overflows = 0;
static volatile uint16_t tickless_timer1_target = 0;

/* Timer1 overflow ISR for counting long sleep periods */
ISR(TIMER1_OVF_vect)
{
    tickless_timer1_overflows++;
}

void rtos_port_tickless_setup(uint32_t sleep_ticks)
{
    uint32_t timer_counts;
    uint16_t compare_value;

    /* Save current Timer0 state */
    tickless_saved_ocr = OCR0A;
    tickless_expected_ticks = sleep_ticks;
    tickless_active = true;
    tickless_elapsed_ticks = 0;
    tickless_timer1_overflows = 0;

    /* Stop Timer0 (normal tick timer) */
    TCCR0B = 0;

    /*
     * Use Timer1 (16-bit) for extended sleep.
     * This allows much longer sleep periods than the 8-bit Timer0.
     *
     * Calculate timer counts needed:
     * counts = ticks * (F_CPU / prescaler / RTOS_TICK_RATE_HZ)
     */
    timer_counts = sleep_ticks * (F_CPU / 64 / RTOS_TICK_RATE_HZ);

    if (timer_counts > 65535) {
        /* Need multiple overflows - use CTC mode with overflow counting */
        compare_value = 65535;
        tickless_timer1_target = (uint16_t)(timer_counts / 65536) + 1;

        /* Enable overflow interrupt */
        TIMSK1 = (1 << TOIE1);
    } else {
        compare_value = (uint16_t)timer_counts;
        tickless_timer1_target = 1;
        TIMSK1 = (1 << OCIE1A);  /* Compare match interrupt */
    }

    /* Configure Timer1 */
    TCCR1A = 0;                     /* Normal mode */
    TCCR1B = 0;                     /* Stopped for now */
    TCNT1 = 0;                      /* Reset counter */
    OCR1A = compare_value;          /* Set compare value */

    /* Start Timer1 with prescaler 64 */
    TCCR1B = (1 << CS11) | (1 << CS10);
}

void rtos_port_sleep_enter(rtos_sleep_mode_t mode)
{
    /* Set appropriate sleep mode */
    switch (mode) {
        case RTOS_SLEEP_POWER_DOWN:
            set_sleep_mode(SLEEP_MODE_PWR_DOWN);
            break;

        case RTOS_SLEEP_STANDBY:
            set_sleep_mode(SLEEP_MODE_STANDBY);
            break;

        case RTOS_SLEEP_IDLE:
        default:
            set_sleep_mode(SLEEP_MODE_IDLE);
            break;
    }

    /* Enable sleep, enter sleep mode */
    sleep_enable();
    sei();  /* Ensure interrupts are enabled for wake */
    sleep_cpu();
    sleep_disable();

    /* Execution continues here after wakeup */
}

uint32_t rtos_port_tickless_get_elapsed(void)
{
    uint32_t elapsed_counts;
    uint16_t current_count;
    uint16_t overflows;

    if (!tickless_active) {
        return 0;
    }

    /* Read Timer1 state atomically */
    cli();
    current_count = TCNT1;
    overflows = tickless_timer1_overflows;
    sei();

    /* Calculate total elapsed counts */
    elapsed_counts = ((uint32_t)overflows * 65536UL) + current_count;

    /* Convert counts to ticks */
    tickless_elapsed_ticks = elapsed_counts / (F_CPU / 64 / RTOS_TICK_RATE_HZ);

    /* Don't exceed expected ticks */
    if (tickless_elapsed_ticks > tickless_expected_ticks) {
        tickless_elapsed_ticks = tickless_expected_ticks;
    }

    return tickless_elapsed_ticks;
}

void rtos_port_tickless_restore(void)
{
    if (!tickless_active) {
        return;
    }

    /* Stop Timer1 */
    TCCR1B = 0;
    TIMSK1 = 0;

    /* Restore Timer0 for normal tick operation */
    TCNT0 = 0;
    OCR0A = tickless_saved_ocr;
    TCCR0A = (1 << WGM01);              /* CTC mode */
    TIMSK0 = (1 << OCIE0A);             /* Enable compare interrupt */
    TCCR0B = (1 << CS01) | (1 << CS00); /* Start with prescaler 64 */

    tickless_active = false;
}

bool rtos_port_tickless_timer_wakeup(void)
{
    /* Check if Timer1 compare or overflow caused wakeup */
    return (TIFR1 & ((1 << OCF1A) | (1 << TOV1))) != 0;
}

/* Timer1 Compare Match A ISR - wakeup from tickless sleep */
ISR(TIMER1_COMPA_vect)
{
    /* Just wake up - actual handling done in tickless_exit */
}

#endif /* RTOS_USE_TICKLESS_IDLE */

#endif /* __AVR__ || RTOS_PLATFORM_AVR */
