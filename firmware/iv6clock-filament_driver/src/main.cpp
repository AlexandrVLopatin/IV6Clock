#include <avr/io.h>
#include <util/delay.h>

#define PIN_PWM1 PB0
#define PIN_PWM2 PB1

#define bit_set(p, b) ((p) |= (1 << b))

int main()
{
    bit_set(DDRB, PIN_PWM1);
    bit_set(DDRB, PIN_PWM2);

    // Waiting for MX1508 H-Bridge initialization (does not works without it),
    _delay_ms(1000);

    for (;;)
    {
        PORTB = 1; // PB0 is off, PB1 is n
        _delay_us(300);
        PORTB = 2; // PB0 is on, PB1 is off
        _delay_us(300);
    }
}
