#!/usr/bin/env python3
# ============================================================
# REALISTIC WAVEFORM GENERATOR - PRODUCTION VERSION
# Mostly NORMAL with occasional short faults
# Author: Noridel Herron
# ============================================================

import csv
import math
import random
import os
# OUTPUT CONFIG
OUT_DIR = "../csv_output"
os.makedirs(OUT_DIR, exist_ok=True)
OUT_FILE = "realistic_raw.csv"

# SIGNAL CONFIG
FREQ = 60.0
SAMPLES_PER_CYCLE = 60
TOTAL_CYCLES = 1200              # ~20 seconds
TOTAL_SAMPLES = TOTAL_CYCLES * SAMPLES_PER_CYCLE

# ESP32 ADC MODEL (12-bit ADC)
ADC_MAX = 4095.0
ADC_MID = ADC_MAX / 2.0  # 2047.5

# CORRECT SCALING (matches ESP32 firmware exactly)
# These values convert physical V/A to ADC counts
# Formula: ADC_count = ADC_MID + (physical_value / SCALE)

V_SCALE = 170.0 / (ADC_MID * 0.6)  # ~0.228 V/count
I_SCALE = 0.0244                    # A/count (ACS712 calibration)

NOISE_COUNTS = 3.0  # Realistic ADC noise

# THRESHOLDS (from constants.h)
V_SAG_LEVEL = 50.0      # Below this = SAG
V_SWELL_LEVEL = 130.0   # Above this = SWELL
I_OC_LEVEL = 11.0       # Above this = OVERCURRENT

# ADC CONVERSION
def physical_to_adc(v_inst, i_inst):
    """Convert instantaneous physical values to ADC counts"""
    # Convert to ADC with noise
    v_adc = ADC_MID + (v_inst / V_SCALE) + random.uniform(-NOISE_COUNTS, NOISE_COUNTS)
    i_adc = ADC_MID + (i_inst / I_SCALE) + random.uniform(-NOISE_COUNTS, NOISE_COUNTS)
    
    # Clamp to valid ADC range
    v_adc = max(0, min(ADC_MAX, v_adc))
    i_adc = max(0, min(ADC_MAX, i_adc))
    
    return round(v_adc, 1), round(i_adc, 1)

# FAULT SCHEDULER (RARE EVENTS)
def build_cycle_states(num_cycles):
    states = ["NORMAL"] * num_cycles
    
    # ---- FIRST GUARANTEED TRIP ----
    for c in range(150, 156):   # sustained OC
        states[c] = "OC"
    
    # ---- SECOND GUARANTEED TRIP ----
    for c in range(400, 406):
        states[c] = "OC"

    for c in range(600, 606):   # sustained OC
        states[c] = "OC"
    
    for c in range(800, 806):
        states[c] = "OC"
    
    for c in range(1000, 1006):
        states[c] = "OC"
    """
    # ---- OCCASIONAL SAG / SWELL ----
    for c in range(500, 503):
        states[c] = "SAG"

    for c in range(700, 703):
        states[c] = "SWELL"
    """
    return states

# ============================================================
# MAIN GENERATOR
# ============================================================
def generate_waveform():
    print("[GEN] Generating realistic power waveform...")
    print(f"[GEN] V_SCALE={V_SCALE:.6f} V/count")
    print(f"[GEN] I_SCALE={I_SCALE:.6f} A/count\n")
    
    cycle_states = build_cycle_states(TOTAL_CYCLES)
    rows = []
    
    # Count fault types for statistics
    fault_counts = {"NORMAL": 0, "SAG": 0, "SWELL": 0, "OC": 0}
    
    for cycle, state in enumerate(cycle_states):
        fault_counts[state] += 1
        
        # ===== NORMAL OPERATION (90-95% of time) =====
        # Safe voltage: 105-120V RMS (well below 130V swell threshold)
        # Safe current: 5-9A RMS (well below 11A OC threshold)
        vrms = random.uniform(105.0, 120.0)
        irms = random.uniform(5.0, 9.0)
        
        # ===== FAULT OVERRIDES (5-10% of time) =====
        if state == "SAG":
            # Deep sag: 30-45V (well below 50V threshold)
            vrms = random.uniform(30.0, 45.0)
            
        elif state == "SWELL":
            # Overvoltage: 135-145V (above 130V threshold)
            vrms = random.uniform(135.0, 145.0)
            
        elif state == "OC":
            # Overcurrent: 13-17A (above 11A threshold)
            irms = random.uniform(15.5, 16.5)
        elif state == "RECOVER":
            irms = random.uniform(6.0, 9.0)
            
        # Convert RMS to peak
        vpeak = vrms * math.sqrt(2.0)
        ipeak = irms * math.sqrt(2.0)
        
        # ===== GENERATE 60 SAMPLES PER CYCLE =====
        for s in range(SAMPLES_PER_CYCLE):
            t = (cycle * SAMPLES_PER_CYCLE + s) / (FREQ * SAMPLES_PER_CYCLE)
            phase = 2.0 * math.pi * FREQ * t
            
            # Instantaneous sinusoidal values
            v_inst = vpeak * math.sin(phase)
            i_inst = ipeak * math.sin(phase)
            
            rows.append(physical_to_adc(v_inst, i_inst))
    
    # Print statistics
    print("[STATS] Cycle distribution:")
    for state, count in fault_counts.items():
        pct = 100.0 * count / TOTAL_CYCLES
        print(f"  {state:8s}: {count:4d} cycles ({pct:5.2f}%)")
    
    return rows

# WRITE CSV
def write_csv(rows):
    path = os.path.join(OUT_DIR, OUT_FILE)
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["Raw_V", "Raw_I"])
        w.writerows(rows)
    
    print(f"\n[OK] Saved: {path}")
    print(f"[OK] Samples: {len(rows)}")
    print(f"[OK] Duration: {len(rows) / (FREQ * SAMPLES_PER_CYCLE):.1f} seconds")

# STATISTICS
def print_adc_stats(rows):
    v_vals = [r[0] for r in rows]
    i_vals = [r[1] for r in rows]
    
    print("\n" + "="*50)
    print("ADC RANGE CHECK")
    print("="*50)
    print(f"V_ADC: min={min(v_vals):.0f}, max={max(v_vals):.0f}, avg={sum(v_vals)/len(v_vals):.0f}")
    print(f"I_ADC: min={min(i_vals):.0f}, max={max(i_vals):.0f}, avg={sum(i_vals)/len(i_vals):.0f}")
    print(f"ADC range: 0-{ADC_MAX:.0f} (midpoint={ADC_MID:.0f})")
    print("="*50)

# ENTRY POINT
if __name__ == "__main__":
    data = generate_waveform()
    write_csv(data)
    print_adc_stats(data)
    
    print("\nDONE - Production waveform generated")
    print("80% Normal operation (GREEN LED)")
    print("5% Voltage sags (YELLOW LED)")
    print("7% Swell events (RED LED)")
    print("8% Overcurrent events (RED LED)")
    print("Real sensor scaling applied")
