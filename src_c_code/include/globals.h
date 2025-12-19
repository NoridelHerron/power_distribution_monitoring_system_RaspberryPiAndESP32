// ============================================================
// globals.h
// Noridel Herron - December 2025
// Global variable declarations shared between Process 1 and Process 2
// ============================================================

#ifndef GLOBALS_H
#define GLOBALS_H

#include <pthread.h>
#include "structs.h"

// Process 1: Combined packet from all ESP32 nodes
extern sensor_packet_t combined_pkt;

// Mutex protecting combined_pkt updates
extern pthread_mutex_t pkt_mutex;

// Process 2: Shared system state with RMS data, fault status, and synchronization
// Used in current_thread.c, led_thread.c, log_thread.c, voltage_thread.c
// Used in process2_init.c, main_process2.c
extern system_data_t shared;

// Process 1: Current operating mode for LED indicators (MODE_ADC, MODE_SD, MODE_UDP)
// Used in command.c
extern int current_mode;

#endif