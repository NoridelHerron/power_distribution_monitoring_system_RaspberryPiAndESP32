// ============================================================
// gpio_utils.c
// Noridel Herron - December 2025
// Initialize GPIO pins for fault indicator LEDs
// ============================================================

#include "all_h.h"

// Initialize all fault indicator LED pins
void init_leds(void)
{
    pinMode(27, OUTPUT);
    pinMode(17, OUTPUT);
    pinMode(22, OUTPUT);

    pinMode(21, OUTPUT);
    pinMode(20, OUTPUT);
    pinMode(16, OUTPUT);

    pinMode(13, OUTPUT);
    pinMode(19, OUTPUT);
    pinMode(26, OUTPUT);
}