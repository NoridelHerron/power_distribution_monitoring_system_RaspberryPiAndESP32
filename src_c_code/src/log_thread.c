// ============================================================
// log_thread.c
// Noridel Herron - December 2025
// CSV logging and fault event recording
// ============================================================

#include "all_h.h"

// Data logging and fault event detection thread
void* log_thread(void *arg)
{
    (void)arg;

    FILE *csv = fopen("power_monitor.csv", "w");
    if (!csv) {
        perror("[log_thread] CSV open failed");
        return NULL;
    }
    // Data Logging
    fprintf(csv,
        "timestamp,"
        "cycle1,cycle2,cycle3,"
        "vrms1,vrms2,vrms3,"
        "vpeak1,vpeak2,vpeak3,"
        "irms1,irms2,irms3,"
        "ipeak1,ipeak2,ipeak3,"
        "vstat1,vstat2,vstat3,"
        "istat1,istat2,istat3,"
        "power1,power2,power3\n");
    fflush(csv);
    
    // Voltage and Current events
    FILE *event_log = fopen("fault_events.txt", "w");
    if (!event_log) {
        perror("[log_thread] event log open failed");
        fclose(csv);
        return NULL;
    }

    fprintf(event_log,
        "============================================================================\n"
        "                      POWER MONITOR FAULT EVENT LOG\n"
        "============================================================================\n\n");
    fflush(event_log);

    time_t last_csv_time = 0;
    int prev_vstat[NUM_NODES] = {VSTATUS_NORMAL, VSTATUS_NORMAL, VSTATUS_NORMAL};
    int prev_istat[NUM_NODES] = {ISTATUS_NORMAL, ISTATUS_NORMAL, ISTATUS_NORMAL};

    printf("[THREAD] Log thread started\n");

    while (1)
    {
        float vrms[NUM_NODES], vpeak[NUM_NODES];
        float irms[NUM_NODES], ipeak[NUM_NODES];
        float power[NUM_NODES];
        uint32_t cycle_id[NUM_NODES];
        int vstat[NUM_NODES], istat[NUM_NODES];

        pthread_mutex_lock(&shared.lock);

        for (int n = 0; n < NUM_NODES; n++) {
            cycle_id[n] = shared.cycle_id[n];

            vrms[n]  = (n==0)?shared.vdata.vrms1  :
                       (n==1)?shared.vdata.vrms2  :shared.vdata.vrms3;

            vpeak[n] = (n==0)?shared.vdata.vpeak1 :
                       (n==1)?shared.vdata.vpeak2 :shared.vdata.vpeak3;

            irms[n]  = (n==0)?shared.idata.irms1  :
                       (n==1)?shared.idata.irms2  :shared.idata.irms3;

            ipeak[n] = (n==0)?shared.idata.ipeak1 :
                       (n==1)?shared.idata.ipeak2 :shared.idata.ipeak3;

            vstat[n] = (n==0)?shared.vdata.status1 :
                       (n==1)?shared.vdata.status2 :shared.vdata.status3;

            istat[n] = (n==0)?shared.idata.status1 :
                       (n==1)?shared.idata.status2 :shared.idata.status3;

            power[n] = vrms[n] * irms[n];
        }

        pthread_mutex_unlock(&shared.lock);

        time_t now = time(NULL);
        char tbuf[64];
        strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

        for (int n = 0; n < NUM_NODES; n++) {
            if (vstat[n] != prev_vstat[n]) {
                if (vstat[n] == VSTATUS_SAG) {
                    fprintf(event_log, "[%s] NODE %d: VOLTAGE SAG DETECTED       -  %.2f V (cycle %u)\n",
                            tbuf, n+1, vrms[n], cycle_id[n]);
                    fflush(event_log);
                }
                else if (vstat[n] == VSTATUS_SWELL) {
                    fprintf(event_log, "[%s] NODE %d: VOLTAGE SWELL DETECTED     - %.2f V (cycle %u)\n",
                            tbuf, n+1, vrms[n], cycle_id[n]);
                    fflush(event_log);
                }
                else if (prev_vstat[n] != VSTATUS_NORMAL) {
                    fprintf(event_log, "[%s] NODE %d: Voltage returned to NORMAL - %.2f V (cycle %u)\n",
                            tbuf, n+1, vrms[n], cycle_id[n]);
                    fflush(event_log);
                }
                prev_vstat[n] = vstat[n];
            }
            
            if (istat[n] != prev_istat[n]) {
                if (istat[n] == ISTATUS_OC) {
                    fprintf(event_log, "[%s] NODE %d: OVERCURRENT DETECTED       -  %.2f A (cycle %u)\n",
                            tbuf, n+1, irms[n], cycle_id[n]);
                    fflush(event_log);
                }
                else if (prev_istat[n] == ISTATUS_OC) {
                    fprintf(event_log, "[%s] NODE %d: Current returned to NORMAL -  %.2f A (cycle %u)\n",
                            tbuf, n+1, irms[n], cycle_id[n]);
                    fflush(event_log);
                }
                prev_istat[n] = istat[n];
            }
        }

        if (now - last_csv_time >= 10) {
            last_csv_time = now;

            fprintf(csv,
                "%s,"
                "%u,%u,%u,"
                "%.3f,%.3f,%.3f,"
                "%.3f,%.3f,%.3f,"
                "%.3f,%.3f,%.3f,"
                "%.3f,%.3f,%.3f,"
                "%d,%d,%d,"
                "%d,%d,%d,"
                "%.3f,%.3f,%.3f\n",
                tbuf,
                cycle_id[0], cycle_id[1], cycle_id[2],
                vrms[0], vrms[1], vrms[2],
                vpeak[0], vpeak[1], vpeak[2],
                irms[0], irms[1], irms[2],
                ipeak[0], ipeak[1], ipeak[2],
                vstat[0], vstat[1], vstat[2],
                istat[0], istat[1], istat[2],
                power[0], power[1], power[2]
            );
            fflush(csv);
        }

        usleep(50000);
    }

    return NULL;
}