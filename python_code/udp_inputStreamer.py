#!/usr/bin/env python3
# ============================================================
# UDP WAVE STREAMER (PER-NODE CONTROL)
# Different scenarios for each ESP32 node
# Author: Noridel Herron
# December 2025
# ============================================================

import socket
import csv
import time
import os
import sys
import select
import termios
import tty

# ==================== CONFIG ====================
CMD_PORT  = 6000
DATA_PORT = 6001

CSV_DIR = "../csv_output"

SAMPLES_PER_CYCLE = 60
SAMPLE_RATE       = 3600.0
SAMPLE_PERIOD     = 1.0 / SAMPLE_RATE

# ==================== ESP NODES ====================
NODES = {
    # change "xx.xxx" based on your ESP assigned address
    1: "192.168.XX.XXX", 
    2: "192.168.XX.XXX",
    3: "192.168.XX.XXX",
}

# ==================== SCENARIOS ====================
CSV_FILES = {
    "base": "base.csv",
    "t1":   "t1.csv",
    "t2":   "t2.csv",
    "t3":   "t3.csv",
    "oc":   "oc.csv",
}

# ==================== TERMINAL MODE ====================
old_settings = None

def enable_raw_mode():
    global old_settings
    old_settings = termios.tcgetattr(sys.stdin)
    tty.setcbreak(sys.stdin.fileno())

def disable_raw_mode():
    global old_settings
    if old_settings:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)

# ==================== LOAD CSV ====================
def load_csv(path):
    data = []
    if not os.path.exists(path):
        print(f"[ERROR] File not found: {path}")
        return data
    
    with open(path, newline="") as f:
        rows = list(csv.reader(f))
        start = 1 if rows and rows[0][0].lower().startswith("raw") else 0
        for r in rows[start:]:
            try:
                data.append((float(r[0]), float(r[1])))
            except (ValueError, IndexError):
                continue
    return data

# ==================== SEND COMMAND ====================
def send_cmd(sock, ip, msg):
    sock.sendto(msg.encode(), (ip, CMD_PORT))
    print(f"[CMD -> {ip}] {msg}")
    time.sleep(0.1)

# ==================== RESET ALL NODES ====================
def reset_all_nodes(cmd_sock):
    print("\n===== RESETTING ALL ESP32 NODES =====")
    
    for nid, ip in NODES.items():
        send_cmd(cmd_sock, ip, f"RESET_CYCLE|0|{nid}")
    
    time.sleep(1)
    
    for nid, ip in NODES.items():
        send_cmd(cmd_sock, ip, f"SET_MODE|MODE_UDP|{nid}")
        send_cmd(cmd_sock, ip, f"SET_SEND|ON|{nid}")
    
    time.sleep(0.5)
    print("[OK] All nodes reset\n")

# ==================== MAIN ====================
def main():
    print("\n=== UDP WAVE STREAMER (PER-NODE CONTROL) ===\n")

    cmd_sock  = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    data_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # Load CSVs
    scenarios = {}
    for name, fname in CSV_FILES.items():
        path = os.path.join(CSV_DIR, fname)
        samples = load_csv(path)
        if samples:
            scenarios[name] = samples
            cycles = len(samples) // SAMPLES_PER_CYCLE
            print(f"[OK] {name}: {len(samples)} samples ({cycles} cycles)")

    if not scenarios:
        print("[ERROR] No scenarios loaded")
        return

    # Per-node state
    node_scenario = {1: "base", 2: "base", 3: "base"}
    node_idx      = {1: 0, 2: 0, 3: 0}
    node_cycle    = {1: 0, 2: 0, 3: 0}

    selected_node = 0  # 0 = ALL, 1-3 = specific node

    reset_all_nodes(cmd_sock)

    print("Controls:")
    print("  a     -> select ALL nodes")
    print("  1,2,3 -> select specific node")
    print("  b     -> base scenario (for selected)")
    print("  s     -> sag scenario t1 (for selected)")
    print("  w     -> swell scenario t2 (for selected)")
    print("  m     -> mixed scenario t3 (for selected)")
    print("  o     -> overcurrent scenario (for selected)")
    print("  r     -> reset all nodes")
    print("  p     -> print status")
    print("  q     -> quit\n")

    enable_raw_mode()

    start_time = time.time()
    last_status = time.time()

    try:
        print("[STREAMING] All nodes: BASE\n")
        
        while True:
            # ---------- Keyboard Input ----------
            if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
                key = sys.stdin.read(1)

                # Node selection
                if key == 'a':
                    selected_node = 0
                    print("\n[SELECT] ALL nodes")

                elif key == '1':
                    selected_node = 1
                    print(f"\n[SELECT] Node 1 ({node_scenario[1]})")

                elif key == '2':
                    selected_node = 2
                    print(f"\n[SELECT] Node 2 ({node_scenario[2]})")

                elif key == '3':
                    selected_node = 3
                    print(f"\n[SELECT] Node 3 ({node_scenario[3]})")

                # Scenario selection
                elif key == 'b':
                    if selected_node == 0:
                        for n in [1, 2, 3]:
                            node_scenario[n] = "base"
                            node_idx[n] = 0
                            node_cycle[n] = 0
                        print("\n[SCENARIO] ALL -> BASE")
                    else:
                        node_scenario[selected_node] = "base"
                        node_idx[selected_node] = 0
                        node_cycle[selected_node] = 0
                        print(f"\n[SCENARIO] Node {selected_node} -> BASE")

                elif key == 's' and "t1" in scenarios:
                    if selected_node == 0:
                        for n in [1, 2, 3]:
                            node_scenario[n] = "t1"
                            node_idx[n] = 0
                            node_cycle[n] = 0
                        print("\n[SCENARIO] ALL -> SAG (t1)")
                    else:
                        node_scenario[selected_node] = "t1"
                        node_idx[selected_node] = 0
                        node_cycle[selected_node] = 0
                        print(f"\n[SCENARIO] Node {selected_node} -> SAG (t1)")

                elif key == 'w' and "t2" in scenarios:
                    if selected_node == 0:
                        for n in [1, 2, 3]:
                            node_scenario[n] = "t2"
                            node_idx[n] = 0
                            node_cycle[n] = 0
                        print("\n[SCENARIO] ALL -> SWELL (t2)")
                    else:
                        node_scenario[selected_node] = "t2"
                        node_idx[selected_node] = 0
                        node_cycle[selected_node] = 0
                        print(f"\n[SCENARIO] Node {selected_node} -> SWELL (t2)")

                elif key == 'm' and "t3" in scenarios:
                    if selected_node == 0:
                        for n in [1, 2, 3]:
                            node_scenario[n] = "t3"
                            node_idx[n] = 0
                            node_cycle[n] = 0
                        print("\n[SCENARIO] ALL -> MIXED (t3)")
                    else:
                        node_scenario[selected_node] = "t3"
                        node_idx[selected_node] = 0
                        node_cycle[selected_node] = 0
                        print(f"\n[SCENARIO] Node {selected_node} -> MIXED (t3)")

                elif key == 'o' and "oc" in scenarios:
                    if selected_node == 0:
                        for n in [1, 2, 3]:
                            node_scenario[n] = "oc"
                            node_idx[n] = 0
                            node_cycle[n] = 0
                        print("\n[SCENARIO] ALL -> OVERCURRENT")
                    else:
                        node_scenario[selected_node] = "oc"
                        node_idx[selected_node] = 0
                        node_cycle[selected_node] = 0
                        print(f"\n[SCENARIO] Node {selected_node} -> OVERCURRENT")

                elif key == 'r':
                    print("\n[RESET] Resetting all nodes...")
                    reset_all_nodes(cmd_sock)
                    for n in [1, 2, 3]:
                        node_idx[n] = 0
                        node_cycle[n] = 0
                    start_time = time.time()

                elif key == 'p':
                    print("\n===== STATUS =====")
                    print(f"Selected: {'ALL' if selected_node == 0 else f'Node {selected_node}'}")
                    for n in [1, 2, 3]:
                        print(f"  Node {n}: {node_scenario[n]:6s} cycle {node_cycle[n]}")
                    print("==================")

                elif key == 'q':
                    print("\n[QUIT]")
                    break

            # ---------- Stream One Sample Per Node ----------
            for nid, ip in NODES.items():
                scenario = node_scenario[nid]
                samples = scenarios[scenario]
                idx = node_idx[nid]

                v_adc, i_adc = samples[idx]
                msg = f"WAVE|{v_adc:.1f}|{i_adc:.1f}"
                
                # Send to THIS node only
                data_sock.sendto(msg.encode(), (ip, DATA_PORT))

                node_idx[nid] += 1

                # Track cycles
                if node_idx[nid] % SAMPLES_PER_CYCLE == 0:
                    node_cycle[nid] += 1

                # Loop back
                if node_idx[nid] >= len(samples):
                    node_idx[nid] = 0
                    node_cycle[nid] = 0

            # Status update every 10 seconds
            if time.time() - last_status >= 10:
                print(f"[STATUS] N1:{node_scenario[1]}@{node_cycle[1]} | N2:{node_scenario[2]}@{node_cycle[2]} | N3:{node_scenario[3]}@{node_cycle[3]}")
                last_status = time.time()

            time.sleep(SAMPLE_PERIOD)

    except KeyboardInterrupt:
        print("\n\n[STOPPED] Ctrl+C")

    finally:
        disable_raw_mode()
        cmd_sock.close()
        data_sock.close()
        print("[CLEANUP] Done")

if __name__ == "__main__":
    main()