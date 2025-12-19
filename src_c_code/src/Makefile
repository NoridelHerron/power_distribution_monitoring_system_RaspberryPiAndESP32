# ============================================================
# Makefile - Noridel Herron - 12/12/2025
# Build Process 1 and Process 2 (UDP-only RMS power monitoring)
# ============================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -I../include
LDFLAGS = -lpthread -lwiringPi -lrt -lm

SRC_DIR = .
OBJ_DIR = build

# ==================== PROCESS 1 OBJECTS ====================
# Network controller - receives RMS packets from ESP32 nodes
P1_OBJS = $(OBJ_DIR)/main_process1.o \
          $(OBJ_DIR)/command.o \
          $(OBJ_DIR)/network.o \
          $(OBJ_DIR)/ipc.o \
          $(OBJ_DIR)/process1_utils.o

# ==================== PROCESS 2 OBJECTS ====================
# Data processing - calculates peaks, monitors faults, controls LEDs
P2_OBJS = $(OBJ_DIR)/main_process2.o \
          $(OBJ_DIR)/process2_init.o \
          $(OBJ_DIR)/process2_utils.o \
          $(OBJ_DIR)/voltage_thread.o \
          $(OBJ_DIR)/current_thread.o \
          $(OBJ_DIR)/led_thread.o \
          $(OBJ_DIR)/log_thread.o \
          $(OBJ_DIR)/ipc.o

TARGETS = process1 process2

# ==================== BUILD TARGETS ====================

all: $(TARGETS)

# Create build folder if needed
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)
	@echo "Created build directory"

# Compile all .c files
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Link Process 1 executable (UDP only)
process1: $(P1_OBJS)
	@echo "Linking process1..."
	$(CC) $(P1_OBJS) -o $@ $(LDFLAGS)
	@echo "Process 1 built successfully"

# Link Process 2 executable
process2: $(P2_OBJS)
	@echo "Linking process2..."
	$(CC) $(P2_OBJS) -o $@ $(LDFLAGS)
	@echo "Process 2 built successfully"

# ==================== UTILITY TARGETS ====================

clean:
	@echo "Cleaning build files..."
	rm -rf $(OBJ_DIR) process1 process2
	@echo "Clean complete"

cleandata:
	@echo "Cleaning output data files..."
	rm -f log.txt power_monitor.csv fault_events.txt
	@echo "Data files cleaned"

rebuild: clean all

help:
	@echo "RMS-Based Power Monitoring System - Makefile Help"
	@echo ""
	@echo "Architecture:"
	@echo "  Process 1: UDP network controller + command interface"
	@echo "  Process 2: RMS processing, fault detection, LED control"
	@echo "  ESP32:     Sensor nodes (RMS computed locally)"
	@echo ""
	@echo "Targets:"
	@echo "  make all       - Build both Process 1 and Process 2 (default)"
	@echo "  make process1  - Build Process 1 only"
	@echo "  make process2  - Build Process 2 only"
	@echo "  make clean     - Remove all build files and executables"
	@echo "  make cleandata - Remove output CSV and log files"
	@echo "  make rebuild   - Clean and rebuild everything"
	@echo "  make help      - Show this help message"
	@echo ""
	@echo "Requirements:"
	@echo "  - GCC compiler"
	@echo "  - wiringPi library"
	@echo "  - pthread, rt, m libraries"

.PHONY: all clean cleandata rebuild help
