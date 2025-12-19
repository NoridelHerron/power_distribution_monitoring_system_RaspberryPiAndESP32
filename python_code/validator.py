#!/usr/bin/env python3
# ============================================================
# RMS VALIDATION TOOL (ESP32 + Process 2)
# Author: Noridel Herron
# December 2025
#
# Validates RMS calculations from ESP32 nodes against
# Python-calculated reference values from baseline CSV.
# Also validates Process 2 logic (Vpeak, Ipeak, status, power).
# ============================================================

import csv
import math
import os
from statistics import mean
import matplotlib.pyplot as plt

# ==================== FILE PATHS ====================
BASELINE_CSV = "../csv_output/base.csv"
PROCESS2_CSV = "../src_c_code/src/power_monitor.csv"

# ==================== ADC + SCALING (same as ESP32) ====================
ADC_MAX = 4095.0
ADC_MID = ADC_MAX / 2.0
V_SCALE = 170.0 / (ADC_MID * 0.6)
I_SCALE = 0.0244

WINDOW  = 60  # ESP32 RMS window size

# ==================== FAULT THRESHOLDS ====================
V_SAG_LEVEL    = 50.0
V_SWELL_LEVEL  = 130.0
I_OC_LEVEL     = 11.0

VSTATUS_NORMAL = 0
VSTATUS_SAG    = 1
VSTATUS_SWELL  = 2
ISTATUS_NORMAL = 0
ISTATUS_OC     = 1

# ==================== HELPER FUNCTIONS ====================

def expected_vstatus(vrms):
    # Calculate expected voltage status
    if vrms < 0.1:
        return VSTATUS_NORMAL
    if vrms < V_SAG_LEVEL:
        return VSTATUS_SAG
    if vrms > V_SWELL_LEVEL:
        return VSTATUS_SWELL
    return VSTATUS_NORMAL

def expected_istatus(irms):
    # Calculate expected current status
    if irms < 0.1:
        return ISTATUS_NORMAL
    if irms > I_OC_LEVEL:
        return ISTATUS_OC
    return ISTATUS_NORMAL

def load_baseline_csv(filepath):
    # Load baseline ADC samples and convert to physical values
    if not os.path.exists(filepath):
        print(f"[ERROR] Baseline CSV not found: {filepath}")
        return None, None
    
    v_samples = []
    i_samples = []
    
    with open(filepath, newline="") as f:
        reader = csv.reader(f)
        header = next(reader)  # skip header
        
        for row in reader:
            if len(row) < 2:
                continue
            try:
                v_adc = float(row[0])
                i_adc = float(row[1])
                
                # Convert ADC to physical values (same as ESP32)
                v_phys = (v_adc - ADC_MID) * V_SCALE
                i_phys = (i_adc - ADC_MID) * I_SCALE
                
                v_samples.append(v_phys)
                i_samples.append(i_phys)
            except ValueError:
                continue
    
    return v_samples, i_samples

def calculate_reference_rms(v_samples, i_samples, window=WINDOW):
    # Calculate reference RMS values for all complete cycles
    num_cycles = len(v_samples) // window
    
    vrms_values = []
    irms_values = []
    
    for cycle in range(num_cycles):
        start = cycle * window
        end = start + window
        
        v_window = v_samples[start:end]
        i_window = i_samples[start:end]
        
        vrms = math.sqrt(sum(x*x for x in v_window) / window)
        irms = math.sqrt(sum(x*x for x in i_window) / window)
        
        vrms_values.append(vrms)
        irms_values.append(irms)
    
    return vrms_values, irms_values

def load_process2_output(filepath):
    # Load Process 2 CSV output
    if not os.path.exists(filepath):
        print(f"[ERROR] Process 2 CSV not found: {filepath}")
        return None
    
    records = {1: [], 2: [], 3: []}
    
    with open(filepath, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            for n in [1, 2, 3]:
                try:
                    vrms = float(row[f"vrms{n}"])
                    irms = float(row[f"irms{n}"])
                    
                    # Skip initialization records with very low values
                    if vrms < 0.01 and irms < 0.01:
                        continue
                    
                    records[n].append({
                        'vrms': vrms,
                        'irms': irms,
                        'vpeak': float(row[f"vpeak{n}"]),
                        'ipeak': float(row[f"ipeak{n}"]),
                        'vstat': int(row[f"vstat{n}"]),
                        'istat': int(row[f"istat{n}"]),
                        'power': float(row[f"power{n}"])
                    })
                except (ValueError, KeyError):
                    continue
    
    return records

# ==================== MAIN VALIDATION ====================

def main():
    print("===== ESP32 + Process 2 Output Verification=====\n")
    v_samples, i_samples = load_baseline_csv(BASELINE_CSV) # Load Baseline 
    
    # Calculate Reference RMS like ESP
    vrms_ref_all, irms_ref_all = calculate_reference_rms(v_samples, i_samples)
    num_cycles = len(vrms_ref_all)
    print(f"Loaded {len(v_samples)} and calculated {num_cycles} reference cycles")

    # Overall reference (average of all cycles)
    vrms_ref     = mean(vrms_ref_all)
    irms_ref     = mean(irms_ref_all)
    
    vrms_ref_min = min(vrms_ref_all)
    vrms_ref_max = max(vrms_ref_all)
    irms_ref_min = min(irms_ref_all)
    irms_ref_max = max(irms_ref_all)
    
    print(f"\nReference Statistics:")
    print(f"Vrms: avg = {vrms_ref:.2f} V, range = {vrms_ref_min:3.2f} - {vrms_ref_max:3.2f} V")
    print(f"Irms: avg =   {irms_ref:.2f} A, range =  {irms_ref_min:3.2f} - {irms_ref_max:3.2f} A\n")
    
    p2_data = load_process2_output(PROCESS2_CSV) # Load Process 2 Output
    
    total_records = sum(len(p2_data[n]) for n in [1, 2, 3])
    print(f"Loaded {total_records} records from Process 2")
    
    for n in [1, 2, 3]:
        print(f"Node {n}: {len(p2_data[n])} records")
    
    # ==================== ESP32 RMS Validation ====================
    print("ESP32 RMS VALIDATION")
    
    node_results = {}
    
    for n in [1, 2, 3]:
        if not p2_data[n]:
            print(f"\nNode {n}: NO DATA (inactive)")
            continue
        
        vrms_vals = [r['vrms'] for r in p2_data[n]]
        irms_vals = [r['irms'] for r in p2_data[n]]
        
        vrms_avg = mean(vrms_vals)
        irms_avg = mean(irms_vals)
        
        vrms_min = min(vrms_vals)
        vrms_max = max(vrms_vals)
        irms_min = min(irms_vals)
        irms_max = max(irms_vals)
        
        # Calculate error against reference
        v_err = abs(vrms_avg - vrms_ref) / vrms_ref * 100
        i_err = abs(irms_avg - irms_ref) / irms_ref * 100
        
        # Pass/Fail (5% tolerance)
        v_pass = v_err < 5.0
        i_pass = i_err < 5.0
        
        node_results[n] = {
            'vrms_avg': vrms_avg,
            'irms_avg': irms_avg,
            'v_err'   : v_err,
            'i_err'   : i_err,
            'v_pass'  : v_pass,
            'i_pass'  : i_pass,
            'count'   : len(p2_data[n])
        }
        
        print(f"\nNode {n} ({len(p2_data[n])} samples):")
        print(f"  Vrms   : avg = {vrms_avg:.2f} V, range = {vrms_min:.2f} - {vrms_max:.2f} V")
        print(f"  Irms   : avg =   {irms_avg:.2f} A, range = {irms_min:.2f} - {irms_max:.2f} A")
        print(f"  V error: {v_err:.2f} % {'PASS' if v_pass else 'FAIL'}")
        print(f"  I error: {i_err:.2f} % {'PASS' if i_pass else 'FAIL'}")
    
    # ==================== Process 2 Logic Validation ====================
    print("PROCESS 2 LOGIC VALIDATION")
    
    for n in [1, 2, 3]:
        if not p2_data[n]:
            continue
        
        vpeak_ok = 0
        ipeak_ok = 0
        vstat_ok = 0
        istat_ok = 0
        power_ok = 0
        total = len(p2_data[n])
        
        for rec in p2_data[n]:
            vrms = rec['vrms']
            irms = rec['irms']
            
            # Vpeak = Vrms * sqrt(2)
            expected_vpeak = vrms * 1.414213562
            if abs(rec['vpeak'] - expected_vpeak) / max(expected_vpeak, 0.001) < 0.001:
                vpeak_ok += 1
            
            # Ipeak = Irms * sqrt(2)
            expected_ipeak = irms * 1.414213562
            if abs(rec['ipeak'] - expected_ipeak) / max(expected_ipeak, 0.001) < 0.001:
                ipeak_ok += 1
            
            # Voltage status
            if rec['vstat'] == expected_vstatus(vrms):
                vstat_ok += 1
            
            # Current status
            if rec['istat'] == expected_istatus(irms):
                istat_ok += 1
            
            # Power = Vrms * Irms
            expected_power = vrms * irms
            if abs(rec['power'] - expected_power) / max(expected_power, 0.001) < 0.001:
                power_ok += 1
        
        print(f"\nNode {n}:")
        print(f"  Vpeak calculation: {vpeak_ok}/{total} ({100*vpeak_ok/total:.1f}%)")
        print(f"  Ipeak calculation: {ipeak_ok}/{total} ({100*ipeak_ok/total:.1f}%)")
        print(f"  Voltage status   : {vstat_ok}/{total} ({100*vstat_ok/total:.1f}%)")
        print(f"  Current status   : {istat_ok}/{total} ({100*istat_ok/total:.1f}%)")
        print(f"  Power calculation: {power_ok}/{total} ({100*power_ok/total:.1f}%)")
    
    # ==================== Generate Plot ====================
    if not node_results:
        print("\n[WARNING] No active nodes to plot")
        return
    
    fig, axes = plt.subplots(2, 2, figsize=(12, 10))
    fig.suptitle("RMS Validation Results", fontsize=14, fontweight='bold')
    
    # Plot 1: Vrms Comparison
    ax1          = axes[0, 0]
    active_nodes = [n for n in [1, 2, 3] if n in node_results]
    node_labels  = [f"Node {n}" for n in active_nodes]
    vrms_avgs    = [node_results[n]['vrms_avg'] for n in active_nodes]
    
    bars1 = ax1.bar(node_labels, vrms_avgs, color=['green' if node_results[n]['v_pass'] else 'red' for n in active_nodes])
    ax1.axhline(vrms_ref, color='blue', linestyle='--', label=f'Reference ({vrms_ref:.2f}V)')
    ax1.set_ylabel("Vrms (V)")
    ax1.set_title("Voltage RMS: ESP32 vs Reference")
    ax1.legend()
    
    # Add value labels on bars
    for bar, val in zip(bars1, vrms_avgs):
        ax1.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 1,
                f'{val:.2f}V', ha='center', va='bottom', fontsize=9)
    
    # Plot 2: Irms Comparison
    ax2 = axes[0, 1]
    irms_avgs = [node_results[n]['irms_avg'] for n in active_nodes]
    
    bars2 = ax2.bar(node_labels, irms_avgs, color=['green' if node_results[n]['i_pass'] else 'red' for n in active_nodes])
    ax2.axhline(irms_ref, color='blue', linestyle='--', label=f'Reference ({irms_ref:.2f}A)')
    ax2.set_ylabel("Irms (A)")
    ax2.set_title("Current RMS: ESP32 vs Reference")
    ax2.legend()
    
    for bar, val in zip(bars2, irms_avgs):
        ax2.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.1,
                f'{val:.2f}A', ha='center', va='bottom', fontsize=9)
    
    # Plot 3: Error Percentages
    ax3 = axes[1, 0]
    v_errors = [node_results[n]['v_err'] for n in active_nodes]
    i_errors = [node_results[n]['i_err'] for n in active_nodes]
    
    x = range(len(active_nodes))
    width = 0.35
    ax3.bar([i - width/2 for i in x], v_errors, width, label='Vrms Error', color='steelblue')
    ax3.bar([i + width/2 for i in x], i_errors, width, label='Irms Error', color='coral')
    ax3.axhline(5.0, color='red', linestyle='--', label='5% Threshold')
    ax3.set_ylabel("Error (%)")
    ax3.set_title("RMS Error Percentages")
    ax3.set_xticks(x)
    ax3.set_xticklabels(node_labels)
    ax3.legend()
    
    # Plot 4: Reference RMS Distribution
    ax4 = axes[1, 1]
    ax4.hist(vrms_ref_all, bins=20, alpha=0.7, label='Vrms cycles', color='steelblue')
    ax4.axvline(vrms_ref, color='red', linestyle='--', label=f'Mean ({vrms_ref:.2f}V)')
    ax4.set_xlabel("Vrms (V)")
    ax4.set_ylabel("Frequency")
    ax4.set_title("Reference Vrms Distribution (All Cycles)")
    ax4.legend()
    
    plt.tight_layout()
    plt.savefig("validator.png", dpi=150)
    plt.show()
    
    # ==================== Final Summary ====================
    print("\n ===== FINAL SUMMARY =====")
    
    all_pass = True
    
    print(f"\nReference Values:")
    print(f"  Vrms = {vrms_ref:.2f}V")
    print(f"  Irms =   {irms_ref:.2f}A")
    
    print(f"\nNode Results:")
    for n in active_nodes:
        v_status = "PASS" if node_results[n]['v_pass'] else "FAIL"
        i_status = "PASS" if node_results[n]['i_pass'] else "FAIL"
        
        print(f"  Node {n}: Vrms = {node_results[n]['vrms_avg']:.2f} V ({node_results[n]['v_err']:.2f} %) {v_status}")
        print(f"          Irms =   {node_results[n]['irms_avg']:.2f} A ({node_results[n]['i_err']:.2f} %) {i_status}")
        
        if not (node_results[n]['v_pass'] and node_results[n]['i_pass']):
            all_pass = False
    
    if all_pass:
        print("\nVALIDATION PASSED - All ESP32 nodes within 5% tolerance")
    else:
        print("\nVALIDATION FAILED - Some nodes exceed 5% error threshold")

if __name__ == "__main__":
    main()