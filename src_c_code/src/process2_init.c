// ============================================================
// process2_init.c
// Noridel Herron - December 2025
// Initialize RMS storage, LEDs, and log files
// ============================================================

#include "all_h.h"

// Initialize RMS storage and reset log files
void init_buffers(void)
{
    FILE *fp = fopen("power_monitor.csv", "w");
    if (fp) {
        fprintf(fp,
            "timestamp,"
            "vrms1,vrms2,vrms3,"
            "vpeak1,vpeak2,vpeak3,"
            "irms1,irms2,irms3,"
            "ipeak1,ipeak2,ipeak3,"
            "power1,power2,power3\n"
        );
        fclose(fp);
        printf("[INIT] power_monitor.csv reset\n");
    } else {
        perror("[INIT] Failed to reset power_monitor.csv");
    }

    FILE *fe = fopen("fault_events.txt", "w");
    if (fe) {
        fprintf(fe,
            "========================================\n"
            "POWER MONITOR FAULT EVENT LOG\n"
            "========================================\n\n"
        );
        fclose(fe);
        printf("[INIT] fault_events.txt reset\n");
    }

    // ===== RESET RAW RMS VALUES =====
    shared.vrms1 = 0.0f;
    shared.vrms2 = 0.0f;
    shared.vrms3 = 0.0f;

    shared.irms1 = 0.0f;
    shared.irms2 = 0.0f;
    shared.irms3 = 0.0f;

    // ===== RESET DERIVED DATA =====
    memset(&shared.vdata, 0, sizeof(shared.vdata));
    memset(&shared.idata, 0, sizeof(shared.idata));
    memset(&shared.pdata, 0, sizeof(shared.pdata));

    // ===== RESET NODE STATUS =====
    for (int i = 0; i < NUM_NODES; i++) {
        shared.node_active[i] = 1;
        shared.cycle_id[i]    = 0;
    }

    printf("[INIT] RMS storage initialized\n");
}

// Initialize all fault indicator LED GPIO pins
void init_leds(void)
{
    for (int node = 0; node < NUM_NODES; node++) {
        for (int color = 0; color < 3; color++) {
            pinMode(led_pins[node][color], OUTPUT);
            digitalWrite(led_pins[node][color], LOW);
        }
    }

    printf("[INIT] LEDs initialized\n");
}
