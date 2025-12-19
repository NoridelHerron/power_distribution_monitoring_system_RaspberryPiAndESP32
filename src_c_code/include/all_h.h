// ============================================================
// all_h.h
// Noridel Herron
// Master header file 
// ============================================================

#ifndef ALL_H
#define ALL_H

// STANDARD C LIBRARIES 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

// POSIX / SYSTEM 
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <termios.h>

// NETWORK 
#include <arpa/inet.h>

// HARDWARE 
#include <wiringPi.h>

// PROJECT HEADERS 
#include "constants.h"
#include "structs.h"
#include "globals.h"

// Process 1 (UDP + IPC only)
#include "process1_functions.h"

// Process 2 (data processing)
#include "process2_functions.h"

#endif // ALL_H
