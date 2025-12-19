// ============================================================
// main_process1.c
// Noridel Herron - December 2025
// Process 1 entry point - Network controller
// ============================================================

#include "all_h.h"

int current_mode = MODE_ADC;

int main(void)
{
    wiringPiSetupGpio();

    pinMode(LED_ADC, OUTPUT);
    pinMode(LED_SD, OUTPUT);
    pinMode(LED_UDP, OUTPUT);

    ipc_init();
    enable_raw_mode();

    pthread_t net_t, fault_t, cmd_t;

    pthread_create(&net_t,   NULL, udp_receiver_thread,   NULL);
    pthread_create(&fault_t, NULL, fault_receiver_thread, NULL);
    pthread_create(&cmd_t,   NULL, command_thread,        NULL);

    pthread_join(cmd_t, NULL);
    return 0;
}
