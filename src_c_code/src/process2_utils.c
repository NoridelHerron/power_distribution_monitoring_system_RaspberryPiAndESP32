// ============================================================
// process2_utils.c
// Noridel Herron - December 2025
// LED control, status conversion, and timestamp utilities
// ============================================================

#include "all_h.h"

static int prev_led_state[NUM_NODES][3] = {{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}};

// Update LED only if state changed (reduces GPIO writes)
void set_led_if_changed(int node, int g, int y, int r)
{
    if (node < 0 || node >= NUM_NODES) return;
    
    if (prev_led_state[node][0] != g) {
        digitalWrite(led_pins[node][0], g);
        prev_led_state[node][0] = g;
    }
    if (prev_led_state[node][1] != y) {
        digitalWrite(led_pins[node][1], y);
        prev_led_state[node][1] = y;
    }
    if (prev_led_state[node][2] != r) {
        digitalWrite(led_pins[node][2], r);
        prev_led_state[node][2] = r;
    }
}

// Convert voltage status to string
const char* vstatus_to_str(int s)
{
    switch (s) {
        case VSTATUS_SAG:   return "SAG";
        case VSTATUS_SWELL: return "SWELL";
        default:            return "NORMAL";
    }
}

// Convert current status to string
const char* istatus_to_str(int s)
{
    switch (s) {
        case ISTATUS_OC: return "OVERCURRENT"; 
        default:         return "NORMAL";
    }
}

// Get current timestamp in milliseconds
uint64_t GET_TIMESTAMP_MS(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000ULL +
           (uint64_t)tv.tv_usec / 1000ULL;
}