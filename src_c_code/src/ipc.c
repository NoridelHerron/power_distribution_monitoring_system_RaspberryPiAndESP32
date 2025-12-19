// ============================================================
// ipc.c
// Noridel Herron - December 2025
// POSIX shared memory and semaphore IPC implementation
// ============================================================

#include "all_h.h"

#define SHM_NAME "/packet_shm"
#define SEM_NAME "/packet_sem"

static sensor_packet_t *shared_packet = NULL;
static sem_t *data_ready = NULL;

// Initialize shared memory and semaphore
int ipc_init(void)
{
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        perror("shm_open");
        return -1;
    }

    if (ftruncate(fd, sizeof(sensor_packet_t)) < 0) {
        perror("ftruncate");
        close(fd);
        return -1;
    }

    shared_packet = mmap(NULL, sizeof(sensor_packet_t),
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED, fd, 0);
    if (shared_packet == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }
    
    close(fd);

    data_ready = sem_open(SEM_NAME, O_CREAT, 0666, 0);
    if (data_ready == SEM_FAILED) {
        perror("sem_open");
        munmap(shared_packet, sizeof(sensor_packet_t));
        return -1;
    }

    printf("[IPC] Initialized\n");
    return 0;
}

// Send packet from Process 1 to Process 2
void ipc_send_packet(const sensor_packet_t *pkt)
{
    if (!shared_packet || !data_ready) {
        fprintf(stderr, "[IPC] Not initialized\n");
        return;
    }

    memcpy(shared_packet, pkt, sizeof(sensor_packet_t));
    sem_post(data_ready);
}

// Receive packet in Process 2 (blocking)
int ipc_receive_packet(sensor_packet_t *pkt)
{
    if (!shared_packet || !data_ready) {
        fprintf(stderr, "[IPC] Not initialized\n");
        return -1;
    }

    sem_wait(data_ready);
    memcpy(pkt, shared_packet, sizeof(sensor_packet_t));
    
    return 0;
}

// Clean up IPC resources
void ipc_cleanup(void)
{
    if (shared_packet) {
        munmap(shared_packet, sizeof(sensor_packet_t));
        shared_packet = NULL;
    }

    if (data_ready) {
        sem_close(data_ready);
        data_ready = NULL;
    }

    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);

    printf("[IPC] Cleaned up\n");
}