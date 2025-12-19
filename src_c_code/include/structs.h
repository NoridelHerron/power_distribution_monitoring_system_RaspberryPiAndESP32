// ============================================================
// structs.h
// Noridel Herron - December 2025
// Data structure definitions for multi-node power monitoring system
// ============================================================

#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "constants.h"

// ESP32 PACKET
// UDP packet format received from ESP32 nodes
typedef struct {
    uint32_t node_id;           // ESP32 node identifier (1-3)
    uint32_t cycle_id;          // RMS calculation cycle counter
    float vrms;                 // RMS voltage (V)
    float irms;                 // RMS current (A)
} __attribute__((packed)) esp_packet_t;

// INTER-PROCESS PACKET 
// Packet format for IPC from Process 1 to Process 2
typedef struct {
    uint32_t cycle_id[NUM_NODES];  // Per-node cycle counters
    float vrms1, vrms2, vrms3;     // RMS voltages for nodes 1-3
    float irms1, irms2, irms3;     // RMS currents for nodes 1-3
    int node_active[NUM_NODES];    // Node activity flags (0=inactive, 1=active)
} sensor_packet_t;

// VOLTAGE DATA
// Processed voltage data with fault classification
typedef struct {
    float vrms1, vrms2, vrms3;     // RMS voltages (V)
    float vpeak1, vpeak2, vpeak3;  // Peak voltages (V)
    int status1, status2, status3; // Fault status (NORMAL/SAG/SWELL)
    uint64_t timestamp;            // Processing timestamp (ms)
} voltage_data_t;

// CURRENT DATA 
// Processed current data with fault classification
typedef struct {
    float irms1, irms2, irms3;     // RMS currents (A)
    float ipeak1, ipeak2, ipeak3;  // Peak currents (A)
    int status1, status2, status3; // Fault status (NORMAL/OVERCURRENT)
    uint64_t timestamp;            // Processing timestamp (ms)
} current_data_t;

// POWER DATA 
// Calculated power from coherent RMS snapshot
typedef struct {
    float p1, p2, p3;               // Power per node (W)
    bool is_valid;                  // Data validity flag
    uint64_t timestamp;             // Calculation timestamp (ms)
} power_data_t;

// SHARED SYSTEM STATE 
// Thread-safe shared state for Process 2
typedef struct {
    pthread_mutex_t lock;           // Mutex for thread-safe access
    pthread_cond_t data_ready;      // Condition variable for thread synchronization

    uint32_t cycle_id[NUM_NODES];   // Per-node cycle counters
    float vrms1, vrms2, vrms3;      // Raw RMS voltages from Process 1
    float irms1, irms2, irms3;      // Raw RMS currents from Process 1
    int node_active[NUM_NODES];     // Node activity status

    voltage_data_t vdata;           // Processed voltage data
    current_data_t idata;           // Processed current data
    power_data_t pdata;             // Calculated power data
} system_data_t;

#endif