// ============================================================
// process2_functions.h
// Noridel Herron - December 2025
// Function declarations for Process 2 (Data Processing)
// ============================================================

#ifndef PROCESS2_FUNCTIONS_H
#define PROCESS2_FUNCTIONS_H

#include "structs.h"

// INITIALIZATION 
// Initialize RMS storage and reset CSV log files
void init_buffers(void);

// Initialize GPIO pins for fault indicator LEDs (9 LEDs total)
void init_leds(void);

// IPC FUNCTIONS 
// Initialize POSIX shared memory and semaphore for inter-process communication
int ipc_init(void);

// Receive sensor packet from Process 1 via shared memory (blocking)
int ipc_receive_packet(sensor_packet_t *pkt);

// Clean up shared memory and semaphore resources
void ipc_cleanup(void);

// TERMINAL UTILITIES 
// Enable raw terminal mode for single-key command input
void enable_raw_mode(void);

// Restore canonical terminal mode
void disable_raw_mode(void);

// LED UTILITIES 
// Update mode indicator LEDs (ADC, SD, UDP)
void set_mode_leds(const char *mode);

// Update fault indicator LED if state changed (prevents unnecessary GPIO writes)
void set_led_if_changed(int node, int g, int y, int r);

// TIMESTAMP HELPER 
// Get current system time in milliseconds since epoch
uint64_t GET_TIMESTAMP_MS(void);

// STATUS CONVERTERS 
// Convert voltage status code to human-readable string
const char* vstatus_to_str(int s);

// Convert current status code to human-readable string
const char* istatus_to_str(int s);

// PROCESS 2 THREAD FUNCTIONS 
// Calculate Vpeak and classify voltage faults (SAG/SWELL/NORMAL)
void* voltage_thread(void *arg);

// Calculate Ipeak and classify current faults (OVERCURRENT/NORMAL)
void* current_thread(void *arg);

// Control fault indicator LEDs with priority-based blinking
void* led_thread(void *arg);

// Log data to CSV and record fault events to text file
void* log_thread(void *arg);

#endif