/**
 * Bare-metal blink — NO RTOS.
 *
 * If this works on an Uno, hardware + toolchain + flash workflow
 * are fine and any failure in the RTOS examples is in the RTOS itself.
 */

#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
    /* Pin 13 = PB5, set as output */
    DDRB |= (1 << PB5);

    while (1) {
        PORTB ^= (1 << PB5);
        _delay_ms(500);
    }
}
