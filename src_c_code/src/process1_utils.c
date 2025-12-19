// ============================================================
// process1_utils.c
// Noridel Herron - December 2025
// Terminal control and LED utilities for Process 1
// ============================================================

#include "all_h.h"
#include <termios.h>
#include <unistd.h>

uint64_t GET_TIMESTAMP_MS(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000ULL +
           (uint64_t)tv.tv_usec / 1000ULL;
}

// Enable raw terminal mode for single-key input
void enable_raw_mode(void)
{
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

// Restore canonical terminal mode
void disable_raw_mode(void)
{
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

// Update mode indicator LEDs
void set_mode_leds(const char *mode)
{
    digitalWrite(LED_ADC, strcmp(mode, "MODE_ADC") == 0);
    digitalWrite(LED_SD,  strcmp(mode, "MODE_SD")  == 0);
    digitalWrite(LED_UDP, strcmp(mode, "MODE_UDP") == 0);
}