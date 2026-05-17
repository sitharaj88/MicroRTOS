/**
 * Minimal RTOS test - one task, busy-wait blink.
 *
 * Avoids rtos_task_delay and timer ticks entirely. Just creates one task
 * that toggles the LED in a tight loop.
 *
 * If this blinks:  task creation + context-switch-to-first-task works;
 *                  the bug is in rtos_task_delay or tick handling.
 * If solid ON:     task got CPU once and crashed.
 * If solid OFF:    task never got CPU.
 */

#include "rtos.h"

#ifdef __AVR__
#include <avr/io.h>
#define LED_PIN     PB5      /* pin 13 */
#define LED_DDR     DDRB
#define LED_PORT    PORTB
#endif

static rtos_tcb_t t1;
static uint8_t    t1_stack[128];

static void blinker(void *arg)
{
    (void)arg;
    while (1) {
        LED_PORT ^= (1 << LED_PIN);
        /* Busy wait ~250 ms at 16 MHz with -Os.
         * Volatile so the compiler doesn't optimize the loop away. */
        for (volatile uint32_t i = 0; i < 200000UL; i++) { }
    }
}

int main(void)
{
    LED_DDR  |=  (1 << LED_PIN);
    LED_PORT &= ~(1 << LED_PIN);

    rtos_kernel_init();
    rtos_task_create(&t1, "blink", blinker, NULL, 2, t1_stack, sizeof(t1_stack));
    rtos_kernel_start();
    return 0;
}
