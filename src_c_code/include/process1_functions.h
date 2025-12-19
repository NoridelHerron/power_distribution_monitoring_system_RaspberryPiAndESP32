// ============================================================
// process1_functions.h
// Noridel Herron - December 2025
// Function declarations for Process 1 (Network Controller)
// ============================================================

#ifndef PROCESS1_FUNCTIONS_H
#define PROCESS1_FUNCTIONS_H

#include <stdint.h>
#include "structs.h"

// NETWORK FUNCTIONS 
// Receive UDP packets from ESP32 nodes on port 5005
void* udp_receiver_thread(void *arg);

// Receive FAULT event messages from ESP32 nodes on CMD_PORT
void* fault_receiver_thread(void *arg);

// Update combined packet with data from a single ESP32 node
void update_combined_packet(const esp_packet_t *pkt);

// COMMAND FUNCTIONS 
// Interactive command interface for controlling ESP32 nodes
void* command_thread(void *arg);

// Send UDP command to ESP32 nodes via broadcast
void send_udp_command(int sock, const char* msg);

// IPC FUNCTIONS 
// Initialize POSIX shared memory and semaphore for inter-process communication
int ipc_init(void);

// Send sensor packet from Process 1 to Process 2 via shared memory
void ipc_send_packet(const sensor_packet_t *pkt);

// UTILITY FUNCTIONS 
// Get current system time in milliseconds
unsigned long long get_current_time_ms(void);

// Update mode indicator LEDs (ADC, SD, UDP)
void set_mode_leds(const char *mode);

#endif