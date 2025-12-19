// ============================================================
// network.c
// UDP packet reception and ESP FAULT event handling
// ============================================================

#include "all_h.h"

sensor_packet_t combined_pkt = {0};
pthread_mutex_t pkt_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ==================== RMS DATA RECEIVER ==================== */
void* udp_receiver_thread(void *arg)
{
    (void)arg;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(DATA_PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(sock, (struct sockaddr*)&addr, sizeof(addr));

    esp_packet_t pkt;

    while (1) {
        ssize_t n = recv(sock, &pkt, sizeof(pkt), 0);
        if (n != sizeof(pkt)) continue;

        pthread_mutex_lock(&pkt_mutex);

        int idx = pkt.node_id - 1;
        if (idx >= 0 && idx < NUM_NODES) {

            combined_pkt.node_active[idx] = 1;
            combined_pkt.cycle_id[idx]    = pkt.cycle_id;

            if (idx == 0) {
                combined_pkt.vrms1 = pkt.vrms;
                combined_pkt.irms1 = pkt.irms;
            } else if (idx == 1) {
                combined_pkt.vrms2 = pkt.vrms;
                combined_pkt.irms2 = pkt.irms;
            } else if (idx == 2) {
                combined_pkt.vrms3 = pkt.vrms;
                combined_pkt.irms3 = pkt.irms;
            }

            ipc_send_packet(&combined_pkt);
        }

        pthread_mutex_unlock(&pkt_mutex);
    }
}

/* ==================== FAULT EVENT RECEIVER ==================== */
void* fault_receiver_thread(void *arg)
{
    (void)arg;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(CMD_PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(sock, (struct sockaddr*)&addr, sizeof(addr));

    char buf[256];

    while (1) {
        ssize_t n = recv(sock, buf, sizeof(buf) - 1, 0);
        if (n <= 0) continue;
        buf[n] = '\0';

        if (strncmp(buf, "FAULT|", 6) == 0) {
            int node;
            char type[16];

            if (sscanf(buf, "FAULT|%d|%15[^|]", &node, type) == 2) {
                printf("[FAULT] Node %d reported %s\n", node, type);
            }
        }
    }
}
