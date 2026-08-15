#!/usr/bin/env python3
"""
Python client example for ECU Multi-Emulator REST API.
Demonstrates FreeBUF/OpenCode integration via HTTP.
"""

import requests
import json
import time
import sys

API_BASE = "http://localhost:8080"

def pretty_print(data):
    print(json.dumps(data, indent=2))

def list_vehicles():
    r = requests.get(f"{API_BASE}/vehicles")
    r.raise_for_status()
    return r.json()

def select_manufacturer(manufacturer, model=None):
    payload = {"manufacturer": manufacturer}
    if model:
        payload["model"] = model
    r = requests.post(f"{API_BASE}/select", json=payload)
    r.raise_for_status()
    return r.json()

def get_current_state():
    r = requests.get(f"{API_BASE}/current")
    r.raise_for_status()
    return r.json()

def set_parameter(name, value):
    r = requests.post(f"{API_BASE}/set_parameter", json={"name": name, "value": value})
    r.raise_for_status()
    return r.json()

def inject_fault(code, description="", severity="minor"):
    r = requests.post(f"{API_BASE}/inject_fault",
                      json={"code": code, "description": description, "severity": severity})
    r.raise_for_status()
    return r.json()

def clear_faults():
    r = requests.post(f"{API_BASE}/clear_faults")
    r.raise_for_status()
    return r.json()

def get_diagnostics():
    r = requests.get(f"{API_BASE}/diagnostics")
    r.raise_for_status()
    return r.json()

def start_recording(path="/tmp/can_recording.log"):
    r = requests.post(f"{API_BASE}/start_recording", json={"path": path})
    r.raise_for_status()
    return r.json()

def get_metrics():
    r = requests.get(f"{API_BASE}/metrics")
    r.raise_for_status()
    return r.text

def get_logs(count=10):
    r = requests.get(f"{API_BASE}/logs", params={"count": count})
    r.raise_for_status()
    return r.json()

def start_fuzz(iterations=500, interval_ms=10):
    r = requests.post(f"{API_BASE}/fuzz_start",
                      json={"iterations": iterations, "interval_ms": interval_ms})
    r.raise_for_status()
    return r.json()


def main():
    print("=" * 60)
    print("ECU Multi-Emulator Python Client")
    print("=" * 60)

    # 1. List available vehicles
    print("\n1. Listing available vehicles...")
    vehicles = list_vehicles()
    pretty_print(vehicles)

    # 2. Select Toyota Camry
    print("\n2. Selecting Toyota Camry...")
    result = select_manufacturer("toyota", "Camry")
    pretty_print(result)

    # 3. Get current state
    print("\n3. Getting current state...")
    state = get_current_state()
    pretty_print(state)

    # 4. Set simulation parameters
    print("\n4. Setting engine RPM to 3000...")
    set_parameter("engine_rpm", 3000)
    set_parameter("vehicle_speed", 80)
    set_parameter("coolant_temp", 92)

    state = get_current_state()
    print(f"   RPM: {state['parameters']['engine_rpm']}")
    print(f"   Speed: {state['parameters']['vehicle_speed']}")
    print(f"   Coolant: {state['parameters']['coolant_temp']}°C")

    # 5. Inject a fault
    print("\n5. Injecting DTC P0300...")
    inject_fault("P0300", "Random Cylinder Misfire Detected", "major")
    inject_fault("P0420", "Catalyst Efficiency Below Threshold", "moderate")

    # 6. Get diagnostics
    print("\n6. Getting diagnostic report...")
    diag = get_diagnostics()
    pretty_print(diag)

    # 7. Clear faults
    print("\n7. Clearing faults...")
    clear_faults()
    state = get_current_state()
    print(f"   Active DTCs after clear: {len(state.get('active_dtcs', []))}")

    # 8. Get metrics
    print("\n8. Getting Prometheus metrics...")
    metrics = get_metrics()
    print(metrics[:500] + "...")

    # 9. Get recent logs
    print("\n9. Getting recent logs...")
    logs = get_logs(3)
    for log in logs:
        print(f"   {log}")

    # 10. Start recording
    print("\n10. Starting CAN recording...")
    result = start_recording("/tmp/test_recording.log")
    print(f"    Result: {result}")

    print("\n" + "=" * 60)
    print("All API calls successful!")
    print("=" * 60)

    return 0


if __name__ == "__main__":
    sys.exit(main())
