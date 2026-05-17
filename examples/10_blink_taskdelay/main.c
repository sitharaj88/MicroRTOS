/**
 * Single-task blink using mr_task_delay.
 *
 * If this blinks:  mr_task_delay works for the single-task case;
 *                  bug in example 1 is in multi-task switching.
 * If solid ON:     task_delay never returns (tick wake-up broken),
 *                  bug is in the tick/delay path.
 */

#include "micrortos.h"

#ifdef __AVR__
#include <avr/io.h>
#define LED_PIN     PB5
#define LED_DDR     DDRB
#define LED_PORT    PORTB
#endif

static mr_tcb_t t1;
static uint8_t    t1_stack[128];

static void blinker(void *arg)
{
    (void)arg;
    while (1) {
        LED_PORT ^= (1 << LED_PIN);
        mr_task_delay(MR_MS_TO_TICKS(500));
    }
}

int main(void)
{
    LED_DDR  |=  (1 << LED_PIN);
    LED_PORT &= ~(1 << LED_PIN);

    mr_kernel_init();
    mr_task_create(&t1, "blink", blinker, NULL, 2, t1_stack, sizeof(t1_stack));
    mr_kernel_start();
    return 0;
}
