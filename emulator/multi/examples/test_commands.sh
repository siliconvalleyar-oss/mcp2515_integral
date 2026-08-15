#!/bin/bash
# Test commands for ECU Multi-Emulator
# Requires: can-utils (cansend, candump), curl, jq

set -e

EMULATOR_HOST="localhost:8080"
CAN_IFACE="can0"

echo "╔══════════════════════════════════════════╗"
echo "║  ECU Multi-Emulator Test Suite          ║"
echo "║  Manufacturer: Chevrolet (default)       ║"
echo "╚══════════════════════════════════════════╝"
echo ""

# ==========================================
# 1. API Tests
# ==========================================
echo "━━━ 1. REST API Tests ━━━"

echo "→ GET /vehicles"
curl -s "http://${EMULATOR_HOST}/vehicles" | jq . || echo "  (API response)"

echo ""
echo "→ POST /select (switch to Toyota)"
curl -s -X POST "http://${EMULATOR_HOST}/select" \
  -H "Content-Type: application/json" \
  -d '{"manufacturer": "toyota", "model": "Camry"}' | jq .

echo ""
echo "→ GET /current"
curl -s "http://${EMULATOR_HOST}/current" | jq .

echo ""
echo "→ POST /set_parameter (set RPM to 2500)"
curl -s -X POST "http://${EMULATOR_HOST}/set_parameter" \
  -H "Content-Type: application/json" \
  -d '{"name": "engine_rpm", "value": 2500}' | jq .

echo ""
echo "→ POST /inject_fault (inject P0300)"
curl -s -X POST "http://${EMULATOR_HOST}/inject_fault" \
  -H "Content-Type: application/json" \
  -d '{"code": "P0300", "description": "Random Misfire", "severity": "major"}' | jq .

echo ""
echo "→ GET /diagnostics"
curl -s "http://${EMULATOR_HOST}/diagnostics" | jq .

echo ""
echo "→ POST /clear_faults"
curl -s -X POST "http://${EMULATOR_HOST}/clear_faults" | jq .

echo ""
echo "→ GET /metrics"
curl -s "http://${EMULATOR_HOST}/metrics"

echo ""
echo "→ GET /logs"
curl -s "http://${EMULATOR_HOST}/logs?count=5" | jq .

# ==========================================
# 2. CAN Commands (SocketCAN)
# ==========================================
echo ""
echo "━━━ 2. CAN Bus Tests (SocketCAN) ━━━"

# Check if CAN interface exists
if ip link show ${CAN_IFACE} &>/dev/null; then
    echo "→ CAN interface ${CAN_IFACE} is available"

    # Mode 01 PID 0C (RPM) request
    echo "→ Sending OBD2 Mode 01 PID 0C (RPM) request..."
    cansend ${CAN_IFACE} 7DF#02010C0000000000
    sleep 0.1

    # Mode 01 PID 0D (Speed) request
    echo "→ Sending OBD2 Mode 01 PID 0D (Speed) request..."
    cansend ${CAN_IFACE} 7DF#02010D0000000000
    sleep 0.1

    # Mode 03 (Read DTCs) request
    echo "→ Sending OBD2 Mode 03 (Read DTCs) request..."
    cansend ${CAN_IFACE} 7DF#0300000000000000
    sleep 0.1

    # Mode 09 VIN request
    echo "→ Sending OBD2 Mode 09 VIN request..."
    cansend ${CAN_IFACE} 7DF#0209020000000000
    sleep 0.1

    # UDS Diagnostic Session Control (Extended)
    echo "→ Sending UDS Diagnostic Session Control (Extended)..."
    cansend ${CAN_IFACE} 7E0#0210030000000000
    sleep 0.1

    # UDS Tester Present
    echo "→ Sending UDS Tester Present..."
    cansend ${CAN_IFACE} 7E0#013E000000000000
    sleep 0.1

    # GMLAN Mode 22 Read DID (VIN)
    echo "→ Sending GMLAN Mode 22 Read VIN DID..."
    cansend ${CAN_IFACE} 7E0#03221A0000000000
    sleep 0.1

    # GMLAN Mode 22 Read Engine RPM
    echo "→ Sending GMLAN Mode 22 Read Engine RPM..."
    cansend ${CAN_IFACE} 7E0#0322F18100000000
    sleep 0.1

    # UDS Security Access (request seed)
    echo "→ Sending UDS Security Access Level 1..."
    cansend ${CAN_IFACE} 7E0#0227010000000000
    sleep 0.1

    # Read data by identifier (ECU Serial)
    echo "→ Sending UDS Read Data By Identifier (ECU Serial)..."
    cansend ${CAN_IFACE} 7E0#0322F18C00000000
    sleep 0.1

    # ECU Reset
    echo "→ Sending UDS ECU Reset (Hard Reset)..."
    cansend ${CAN_IFACE} 7E0#0211010000000000
    sleep 0.1

else
    echo "→ CAN interface ${CAN_IFACE} not available."
    echo "  To create a virtual CAN interface for testing:"
    echo "    sudo modprobe can"
    echo "    sudo modprobe vcan"
    echo "    sudo ip link add vcan0 type vcan"
    echo "    sudo ip link set vcan0 up"
    echo "  Then configure config.json to use 'vcan0'"
fi

# ==========================================
# 3. Monitoring
# ==========================================
echo ""
echo "━━━ 3. Monitoring Commands ━━━"

echo "→ To monitor CAN traffic in a separate terminal:"
echo "    candump ${CAN_IFACE}"
echo ""
echo "→ To view emulator logs:"
echo "    tail -f /var/log/ecu_emulator.log"
echo ""
echo "→ To access Prometheus metrics:"
echo "    curl http://${EMULATOR_HOST}/metrics"
echo ""
echo "→ To open WebSocket monitor (requires websocat):"
echo "    websocat ws://${EMULATOR_HOST##*:}:8081"

echo ""
echo "━━━ All tests completed ━━━"
