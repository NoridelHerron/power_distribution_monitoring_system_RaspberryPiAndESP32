// ============================================================
// voltage_thread.c
// Noridel Herron - December 2025
// Calculate Vpeak and classify voltage faults
// ============================================================

#include "all_h.h"

// Voltage monitoring and fault detection thread
void* voltage_thread(void *arg)
{
    (void)arg;
    int status;

    while (1)
    {
        pthread_mutex_lock(&shared.lock);

        for (int n = 0; n < NUM_NODES; n++)
        {
            float vrms =
                (n == 0) ? shared.vrms1 :
                (n == 1) ? shared.vrms2 :
                           shared.vrms3;

            float vpeak = vrms * 1.414213562f;

            if (vrms < 0.1f)
                status = VSTATUS_NORMAL;
            else if (vrms < V_SAG_LEVEL)
                status = VSTATUS_SAG;
            else if (vrms > V_SWELL_LEVEL)
                status = VSTATUS_SWELL;
            else
                status = VSTATUS_NORMAL;

            if (n == 0) {
                shared.vdata.vrms1   = vrms;
                shared.vdata.vpeak1  = vpeak;
                shared.vdata.status1 = status;
            } else if (n == 1) {
                shared.vdata.vrms2   = vrms;
                shared.vdata.vpeak2  = vpeak;
                shared.vdata.status2 = status;
            } else {
                shared.vdata.vrms3   = vrms;
                shared.vdata.vpeak3  = vpeak;
                shared.vdata.status3 = status;
            }
        }

        pthread_mutex_unlock(&shared.lock);
        usleep(50000);
    }
}
