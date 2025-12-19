// ============================================================
// led_thread.c
// Noridel Herron - December 2025
// Control fault indicator LEDs with priority-based blinking
// ============================================================

#include "all_h.h"

// LED fault indicator thread
void* led_thread(void *arg)
{
    (void)arg;

    printf("[THREAD] LED thread started\n");

    int blink_state = 0;
    uint64_t last_blink_time = GET_TIMESTAMP_MS();

    while (1)
    {
        int vstat[NUM_NODES], istat[NUM_NODES];
        uint64_t now = GET_TIMESTAMP_MS();

        pthread_mutex_lock(&shared.lock);

        vstat[0] = shared.vdata.status1;
        vstat[1] = shared.vdata.status2;
        vstat[2] = shared.vdata.status3;

        istat[0] = shared.idata.status1;
        istat[1] = shared.idata.status2;
        istat[2] = shared.idata.status3;

        pthread_mutex_unlock(&shared.lock);

        if (now - last_blink_time >= 200) {
            blink_state = !blink_state;
            last_blink_time = now;
        }

        for (int n = 0; n < NUM_NODES; n++)
        {
            int green  = 0;
            int volt   = 0;
            int curr   = 0;

            if (istat[n] == ISTATUS_OC ) {
                curr = blink_state;
            }
            else if ( vstat[n] == VSTATUS_SWELL) {
                volt = blink_state;
            }
            else if (vstat[n] == VSTATUS_SAG) {
                volt = blink_state;
            }
            else {
                green = 1;
            }

            set_led_if_changed(n, green, volt, curr);
        }

        usleep(50000);
    }

    return NULL;
}