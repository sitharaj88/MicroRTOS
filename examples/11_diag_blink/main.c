/**
 * Diagnostic blink.
 *
 * Goal: see exactly how many task-resume cycles we get before the system
 * halts.
 *
 * Each iteration writes a 4-bit counter onto pins 13,12,11,10 (PB5..PB2).
 * On Uno only pin 13 has the built-in LED, but you can count by watching
 * pin 13: it toggles each iteration. So:
 *   1 toggle  = 1 successful resume from task_delay  (LED off -> on)
 *   2 toggles = 2 successful resumes                  (on -> off -> on)
 *   3 toggles = 3                                      (... -> off)
 *   etc.
 *
 * Short 100 ms delays so we can see many cycles quickly.
 */

#include "micrortos.h"

#ifdef __AVR__
#include <avr/io.h>
#define LED_PIN     PB5
#define LED_DDR     DDRB
#define LED_PORT    PORTB
#endif

static mr_tcb_t t1;
static uint8_t    t1_stack[256];

static volatile uint16_t iter_count;

static void blinker(void *arg)
{
    (void)arg;
    while (1) {
        iter_count++;
        LED_PORT ^= (1 << LED_PIN);
        mr_task_delay(MR_MS_TO_TICKS(100));
    }
}

int main(void)
{
    LED_DDR  |=  (1 << LED_PIN);
    LED_PORT &= ~(1 << LED_PIN);
    iter_count = 0;

    mr_kernel_init();
    mr_task_create(&t1, "blink", blinker, NULL, 2, t1_stack, sizeof(t1_stack));
    mr_kernel_start();
    return 0;
}
