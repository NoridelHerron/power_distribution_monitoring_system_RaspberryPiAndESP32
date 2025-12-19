// ============================================================
// main_process2.c
// Noridel Herron - December 2025
// Process 2 entry point - RMS data processing and fault detection
// ============================================================

#include "all_h.h"

system_data_t shared = {
    .lock       = PTHREAD_MUTEX_INITIALIZER,
    .data_ready = PTHREAD_COND_INITIALIZER
};

// Process 2 main - data processing and monitoring
int main(void)
{
    printf("\n============================================\n");
    printf(" PROCESS 2 - RMS DATA PROCESSING\n");
    printf("============================================\n\n");

    printf("[Process2] Architecture:\n");
    printf("  - Voltage thread: Calculate Vpeak from Vrms\n");
    printf("  - Current thread: Calculate Ipeak from Irms\n");
    printf("  - Log thread:     Atomic power calculation + CSV logging\n");
    printf("  - LED thread:     Fault monitoring\n\n");

    if (wiringPiSetupGpio() == -1) {
        fprintf(stderr, "[ERROR] wiringPi init failed\n");
        return 1;
    }

    init_buffers();
    init_leds();

    if (ipc_init() != 0) {
        fprintf(stderr, "[ERROR] IPC init failed\n");
        return 1;
    }

    pthread_t vtid, itid, ltid, ledtid;
    pthread_create(&vtid,   NULL, voltage_thread, NULL);
    pthread_create(&itid,   NULL, current_thread, NULL);
    pthread_create(&ltid,   NULL, log_thread,     NULL);
    pthread_create(&ledtid, NULL, led_thread,     NULL);

    printf("[Process2] Worker threads running.\n\n");

    sensor_packet_t pkt;

    while (1)
    {
        if (ipc_receive_packet(&pkt) != 0) {
            fprintf(stderr, "[ERROR] IPC receive failed\n");
            continue;
        }

        pthread_mutex_lock(&shared.lock);

        shared.vrms1 = pkt.vrms1;
        shared.vrms2 = pkt.vrms2;
        shared.vrms3 = pkt.vrms3;

        shared.irms1 = pkt.irms1;
        shared.irms2 = pkt.irms2;
        shared.irms3 = pkt.irms3;

        for (int i = 0; i < NUM_NODES; i++) {
            shared.cycle_id[i]    = pkt.cycle_id[i];
            shared.node_active[i] = pkt.node_active[i];
        }

        pthread_mutex_unlock(&shared.lock);
    }

    return 0;
}