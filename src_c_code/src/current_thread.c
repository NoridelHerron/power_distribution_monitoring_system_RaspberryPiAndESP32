// ============================================================
// current_thread.c
// Noridel Herron - December 2025
// Calculate Ipeak and classify current faults
// ============================================================

#include "all_h.h"

// Current monitoring and fault detection thread
void* current_thread(void *arg)
{
    (void)arg;
    int status;

    while (1)
    {
        pthread_mutex_lock(&shared.lock);

        for (int n = 0; n < NUM_NODES; n++)
        {
            float irms =
                (n == 0) ? shared.irms1 :
                (n == 1) ? shared.irms2 :
                           shared.irms3;

            float ipeak = irms * 1.414213562f;

            if (irms > I_OC_LEVEL)
                status = ISTATUS_OC;
            else
                status = ISTATUS_NORMAL;

            if (n == 0) {
                shared.idata.irms1   = irms;
                shared.idata.ipeak1  = ipeak;
                shared.idata.status1 = status;
            } else if (n == 1) {
                shared.idata.irms2   = irms;
                shared.idata.ipeak2  = ipeak;
                shared.idata.status2 = status;
            } else {
                shared.idata.irms3   = irms;
                shared.idata.ipeak3  = ipeak;
                shared.idata.status3 = status;
            }
        }

        pthread_mutex_unlock(&shared.lock);
        usleep(50000);
    }
}
