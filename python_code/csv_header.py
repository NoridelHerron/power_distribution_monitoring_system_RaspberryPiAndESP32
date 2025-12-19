#!/usr/bin/env python3
# ============================================================
# csv_to_header.py
# Noridel Herron - 12/12/2025
# Convert CSV files to ESP32 header files
# Flexible: Choose RMS (actual) or RAW ADC conversion
# ============================================================

import csv
import os
import sys

# ---------------------------------------------------
# CONFIG: CSV folder + header output folder
# ---------------------------------------------------

CSV_FOLDER = "../csv_output"      # where CSV files are located
HEADER_FOLDER = "../headers"      # where .h files will be written

os.makedirs(HEADER_FOLDER, exist_ok=True)

# ---------------------------------------------------
# MODE SELECTION
# ---------------------------------------------------
# 0 = RMS (actual values) - DEFAULT
# 1 = RAW (ADC values)
# ---------------------------------------------------

CONVERSION_MODE = 0  # Change this to 1 for RAW mode

# ---------------------------------------------------
# Convert RMS CSV (Voltage_RMS, Current_RMS) → .h file
# ---------------------------------------------------

def convert_rms_csv_to_header(csv_filename, header_filename, mode_name):
    """
    Read CSV with Voltage_RMS and Current_RMS columns.
    Generate C header file with two float arrays.
    """
    csv_path = f"{CSV_FOLDER}/{csv_filename}"
    header_path = f"{HEADER_FOLDER}/{header_filename}"

    rms_v = []
    rms_i = []

    # Read CSV
    print(f"Reading: {csv_filename}...", end=" ")
    try:
        with open(csv_path, "r") as f:
            reader = csv.DictReader(f)
            
            # Get the first row to check column names
            first_row = next(reader, None)
            if first_row is None:
                print(f"ERROR - Empty file!")
                return
            
            # Check what columns exist and map them
            columns = list(first_row.keys())
            
            # Try different possible column name variations
            v_col = None
            i_col = None
            
            for col in columns:
                col_lower = col.lower().replace(' ', '').replace('_', '').replace('(', '').replace(')', '')
                if 'voltagerms' in col_lower or 'vrms' in col_lower:
                    v_col = col
                elif 'currentrms' in col_lower or 'irms' in col_lower:
                    i_col = col
            
            if not v_col or not i_col:
                print(f"  ERROR - Could not find RMS voltage/current columns!")
                print(f"  Available columns: {columns}")
                return
            
            print(f"  Using: {v_col} and {i_col}")
            
            # Process first row
            rms_v.append(float(first_row[v_col]))
            rms_i.append(float(first_row[i_col]))
            
            # Process remaining rows
            for row in reader:
                rms_v.append(float(row[v_col]))
                rms_i.append(float(row[i_col]))
    except FileNotFoundError:
        print(f"ERROR - File not found!")
        return
    except KeyError as e:
        print(f"ERROR - Missing column: {e}")
        return

    sample_count = len(rms_v)
    print(f"{sample_count} samples")

    # Write header file
    with open(header_path, "w") as h:
        guard = header_filename.replace(".", "_").upper()

        h.write(f"#ifndef {guard}\n")
        h.write(f"#define {guard}\n")
        h.write("// AUTO-GENERATED RMS WAVEFORM FILE\n")
        h.write(f"// MODE: {mode_name.upper()}\n")
        h.write(f"// TYPE: RMS (Actual Values)\n")
        h.write(f"// TOTAL SAMPLES: {sample_count}\n\n")
        
        # Sample count constant
        h.write(f"const int {mode_name}_sample_count = {sample_count};\n\n")
        
        # RMS voltage array
        h.write(f"const float {mode_name}_raw_voltage[{sample_count}] = {{\n")
        for i, v in enumerate(rms_v):
            if i % 5 == 0:
                h.write("    ")
            h.write(f"{v:.3f}f")
            if i < sample_count - 1:
                h.write(", ")
            if (i + 1) % 5 == 0 or i == sample_count - 1:
                h.write("\n")
        h.write("};\n\n")
        
        # RMS current array
        h.write(f"const float {mode_name}_raw_current[{sample_count}] = {{\n")
        for i, c in enumerate(rms_i):
            if i % 5 == 0:
                h.write("    ")
            h.write(f"{c:.3f}f")
            if i < sample_count - 1:
                h.write(", ")
            if (i + 1) % 5 == 0 or i == sample_count - 1:
                h.write("\n")
        h.write("};\n\n")
        
        h.write(f"#endif // {guard}\n")

    # Print statistics
    v_min, v_max = min(rms_v), max(rms_v)
    i_min, i_max = min(rms_i), max(rms_i)
    v_avg = sum(rms_v) / len(rms_v)
    i_avg = sum(rms_i) / len(rms_i)
    
    print(f"  Generated: {header_filename}")
    print(f"    Voltage: min={v_min:.2f}V, max={v_max:.2f}V, avg={v_avg:.2f}V")
    print(f"    Current: min={i_min:.2f}A, max={i_max:.2f}A, avg={i_avg:.2f}A")


# ---------------------------------------------------
# Convert RAW CSV (Raw_V, Raw_I) → .h file
# ---------------------------------------------------

def convert_raw_csv_to_header(csv_filename, header_filename, mode_name):
    """
    Read CSV with Raw_V and Raw_I columns.
    Generate C header file with two integer arrays.
    """
    csv_path = f"{CSV_FOLDER}/{csv_filename}"
    header_path = f"{HEADER_FOLDER}/{header_filename}"

    raw_v = []
    raw_i = []

    # Read CSV
    print(f"Reading: {csv_filename}...", end=" ")
    try:
        with open(csv_path, "r") as f:
            reader = csv.DictReader(f)
            
            # Get the first row to check column names
            first_row = next(reader, None)
            if first_row is None:
                print(f"ERROR - Empty file!")
                return
            
            # Check what columns exist and map them
            columns = list(first_row.keys())
            
            # Try different possible column name variations
            v_col = None
            i_col = None
            
            for col in columns:
                col_lower = col.lower().replace(' ', '').replace('_', '')
                if 'rawv' in col_lower or 'raw' in col_lower and 'v' in col_lower:
                    v_col = col
                elif 'rawi' in col_lower or 'raw' in col_lower and 'i' in col_lower:
                    i_col = col
            
            if not v_col or not i_col:
                print(f"  ERROR - Could not find RAW voltage/current columns!")
                print(f"  Available columns: {columns}")
                return
            
            print(f"  Using: {v_col} and {i_col}")
            
            # Process first row
            raw_v.append(int(float(first_row[v_col])))
            raw_i.append(int(float(first_row[i_col])))
            
            # Process remaining rows
            for row in reader:
                raw_v.append(int(float(row[v_col])))
                raw_i.append(int(float(row[i_col])))
    except FileNotFoundError:
        print(f"ERROR - File not found!")
        return
    except KeyError as e:
        print(f"ERROR - Missing column: {e}")
        return

    sample_count = len(raw_v)
    print(f"{sample_count} samples")

    # Write header file
    with open(header_path, "w") as h:
        guard = header_filename.replace(".", "_").upper()

        h.write(f"#ifndef {guard}\n")
        h.write(f"#define {guard}\n")
        h.write("// AUTO-GENERATED RAW ADC WAVEFORM FILE\n")
        h.write(f"// MODE: {mode_name.upper()}\n")
        h.write(f"// TYPE: RAW (ADC Values)\n")
        h.write(f"// TOTAL SAMPLES: {sample_count}\n\n")
        
        # Sample count constant
        h.write(f"const int {mode_name}_sample_count = {sample_count};\n\n")
        
        # Raw voltage array (as integers)
        h.write(f"const int {mode_name}_raw_voltage[{sample_count}] = {{\n")
        for i, v in enumerate(raw_v):
            if i % 10 == 0:
                h.write("    ")
            h.write(f"{v:4d}")
            if i < sample_count - 1:
                h.write(", ")
            if (i + 1) % 10 == 0 or i == sample_count - 1:
                h.write("\n")
        h.write("};\n\n")
        
        # Raw current array (as integers)
        h.write(f"const int {mode_name}_raw_current[{sample_count}] = {{\n")
        for i, c in enumerate(raw_i):
            if i % 10 == 0:
                h.write("    ")
            h.write(f"{c:4d}")
            if i < sample_count - 1:
                h.write(", ")
            if (i + 1) % 10 == 0 or i == sample_count - 1:
                h.write("\n")
        h.write("};\n\n")
        
        h.write(f"#endif // {guard}\n")

    # Print statistics
    v_min, v_max = min(raw_v), max(raw_v)
    i_min, i_max = min(raw_i), max(raw_i)
    print(f"  Generated: {header_filename}")
    print(f"    Voltage range: {v_min:4d} - {v_max:4d} (swing: {v_max-v_min:4d})")
    print(f"    Current range: {i_min:4d} - {i_max:4d} (swing: {i_max-i_min:4d})")


# ---------------------------------------------------
# Main conversion logic
# ---------------------------------------------------

if __name__ == "__main__":
    # Check for command line argument
    if len(sys.argv) > 1:
        try:
            CONVERSION_MODE = int(sys.argv[1])
            if CONVERSION_MODE not in [0, 1]:
                print("ERROR: Mode must be 0 (RMS) or 1 (RAW)")
                sys.exit(1)
        except ValueError:
            print("ERROR: Mode must be 0 (RMS) or 1 (RAW)")
            sys.exit(1)
    
    print("\n" + "="*70)
    print("  CSV TO HEADER CONVERTER")
    print("="*70)
    print(f"\nInput folder:  {CSV_FOLDER}")
    print(f"Output folder: {HEADER_FOLDER}")
    print(f"\nConversion mode: {CONVERSION_MODE} ({'RMS (Actual)' if CONVERSION_MODE == 0 else 'RAW (ADC)'})")
    print("="*70)
    
    if CONVERSION_MODE == 0:
        # RMS MODE (ACTUAL VALUES)
        print("\n🔹 CONVERTING RMS FILES (Actual Values)")
        print("="*70)
        convert_rms_csv_to_header("real_rms.csv", "real_raw.h", "real")
        print()
        convert_rms_csv_to_header("random_rms.csv", "random_raw.h", "random")
        print()
        convert_rms_csv_to_header("wave_rms.csv", "wave_raw.h", "wave")
        
        print("\n" + "="*70)
        print("✓ RMS Conversion Complete!")
        print("="*70)
        print("\nGenerated files (RMS - float arrays):")
        print("  - real_raw.h")
        print("  - random_raw.h")
        print("  - wave_raw.h")
        print("\nArray names in headers:")
        print("  - real_raw_voltage[]    (float)")
        print("  - real_raw_current[]    (float)")
        print("  - random_raw_voltage[]  (float)")
        print("  - random_raw_current[]  (float)")
        print("  - wave_raw_voltage[]    (float)")
        print("  - wave_raw_current[]    (float)")
        
    else:
        # RAW MODE (ADC VALUES)
        print("\n🔹 CONVERTING RAW ADC FILES")
        print("="*70)
        convert_raw_csv_to_header("real_raw.csv", "real_raw.h", "real")
        print()
        convert_raw_csv_to_header("random_raw.csv", "random_raw.h", "random")
        print()
        convert_raw_csv_to_header("wave_raw.csv", "wave_raw.h", "wave")
        
        print("\n" + "="*70)
        print("✓ RAW ADC Conversion Complete!")
        print("="*70)
        print("\nGenerated files (RAW - int arrays):")
        print("  - real_raw.h")
        print("  - random_raw.h")
        print("  - wave_raw.h")
        print("\nArray names in headers:")
        print("  - real_raw_voltage[]    (int)")
        print("  - real_raw_current[]    (int)")
        print("  - random_raw_voltage[]  (int)")
        print("  - random_raw_current[]  (int)")
        print("  - wave_raw_voltage[]    (int)")
        print("  - wave_raw_current[]    (int)")
    
    print("\nUsage:")
    print("  Default (RMS):  python3 csv_to_header.py")
    print("  Force RMS:      python3 csv_to_header.py 0")
    print("  Force RAW:      python3 csv_to_header.py 1")
    print("\nTo switch modes, edit CONVERSION_MODE at top of script")
    print("or pass mode as command line argument")
    print()